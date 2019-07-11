#include "main.hpp"

#include <cassert>
#include <functional>
#include <sstream>
#include <unordered_map>

#include <boost/format.hpp>

#define AQ1_ASSERT(cond, msg)    \
    do {                         \
        assert((cond) && (msg)); \
    } while (0);

template <class T>
[[noreturn]] void error(T msg)
{
    std::cerr << "[ERROR]\t" << msg << std::endl;
    std::terminate();
}

[[noreturn]] void unreachable(const char *msg)
{
    AQ1_ASSERT(false, msg);
}

// Convert fractions to floating points
// TODO: Much smarter way?
MPFloat r2f(const MPRational &r)
{
    return MPFloat{r.numerator().str()} / MPFloat{r.denominator().str()};
}

MPInt f2i(MPFloat f)
{
    return MPInt{MPFloat{mp::floor(f)}.str()};
}

MPFloat i2f(MPInt i)
{
    return MPFloat{i.str()};
}

MPRational f2r(const MPFloat &f)
{
    // TODO: Is that correct?
    MPInt base{10};
    base = mp::pow(base, 100);  // 10^100
    return MPRational{f2i(f * i2f(base)), base};
}

Token Lex::next()
{
    if (pending_) {
        Token ret = *pending_;
        pending_.reset();
        return ret;
    }

    int ch;
    do {
        AQ1_ASSERT(is_.good(), "Invalid input stream.");
        ch = is_.get();
        if (ch == EOF) return Token::owari();  // TOKEN NO OWARI
    } while (isspace(ch) && ch != '\r' && ch != '\n');

    // When numeric literal
    if (isdigit(ch)) {
        MPRational n = ch - '0';
        while (isdigit(ch = is_.get())) n = n * 10 + ch - '0';
        if (ch == '.') {
            MPRational digit = 1;
            while (isdigit(ch = is_.get())) {
                digit /= 10;
                n += digit * (ch - '0');
            }
        }
        is_.putback(ch);
        return {TOK::NUMLIT, n};
    }

    // When identifier
    if (isalpha(ch)) {
        std::stringstream ss;
        ss << (char)ch;
        while (isalnum(ch = is_.get())) ss << (char)ch;
        is_.putback(ch);
        return {TOK::IDENT, ss.str()};
    }

    switch (ch) {
    case '+':
        return {TOK::PLUS};
    case '-':
        return {TOK::MINUS};
    case '*':
        return {TOK::STAR};
    case '/':
        return {TOK::SLASH};
    case '(':
        return {TOK::LPAREN};
    case ')':
        return {TOK::RPAREN};
    case ',':
        return {TOK::COMMA};
    case '\n':
        return {TOK::LFCR};
    case '\r': {
        ch = is_.get();
        if (ch != '\n') is_.putback(ch);
        return {TOK::LFCR};
    }
    }

    // No valid token
    is_.putback(ch);
    return Token::owari();
}

Token Lex::get()
{
    while (true) {
        Token tok = next();
        if (tok.kind != TOK::LFCR) return tok;
    }
}

Token Lex::expect(TOK kind)
{
    Token tok = get();
    if (tok.kind != kind)
        error("Unexpected token: got \"%1%\", but expect \"%2%\"");
    return tok;
}

bool Lex::is(TOK kind)
{
    if (!pending_) pending_ = next();
    return pending_->kind == kind;
}

bool Lex::match(TOK kind)
{
    if (!pending_ || pending_->kind == TOK::LFCR) pending_ = get();
    return pending_->kind == kind;
}

MPRational BinOp::eval() const
{
    MPRational lhs = lhs_->eval(), rhs = rhs_->eval();

    switch (kind_) {
    case BINOP::ADD:
        return lhs + rhs;
    case BINOP::SUB:
        return lhs - rhs;
    case BINOP::MUL:
        return lhs * rhs;
    case BINOP::DIV:
        return lhs / rhs;
    }

    unreachable("Invalid binop's kind");
}

MPRational FuncCall::eval() const
{
    std::unordered_map<std::string,
                       std::pair<size_t, std::function<MPRational(
                                             const std::vector<MPRational> &)>>>
        functbl = {
            {"sqrt",
             {1, [](auto &&args) { return f2r(mp::sqrt(r2f(args[0]))); }}},
            {"floor",
             {1, [](auto &&args) { return f2r(mp::floor(r2f(args[0]))); }}},
            {"scale", {2, [](auto &&args) {
                           MPFloat base = mp::pow(10, r2f(args[0]));
                           return f2r(mp::floor(r2f(args[1]) * base) / base);
                       }}}};

    auto it = functbl.find(name_);
    if (it != functbl.end()) {
        if (args_.size() != it->second.first)
            error("Invalid number of arguments for \"%1%\"");

        std::vector<MPRational> args;
        for (auto &&arg : args_) args.push_back(arg->eval());

        return it->second.second(args);
    }

    error(boost::format("Invalid function name \"%1%\"") % name_);
}

ASTNodePtr Parser::parse_primary()
{
    Token tok = lex_.get();

    switch (tok.kind) {
    case TOK::NUMLIT:
        return std::make_shared<NumImm>(std::get<MPRational>(tok.data));
    case TOK::LPAREN: {
        ASTNodePtr ast = parse_expr();
        lex_.expect(TOK::RPAREN);
        return ast;
    }
    case TOK::IDENT: {  // function call
        std::string funcname = std::get<std::string>(tok.data);
        std::vector<ASTNodePtr> args;

        lex_.expect(TOK::LPAREN);
        if (!lex_.match(TOK::RPAREN)) {
            args.push_back(parse_expr());
            while (lex_.match(TOK::COMMA)) {
                lex_.get();  // Eat TOK::COMMA
                args.push_back(parse_expr());
            }
        }
        lex_.expect(TOK::RPAREN);

        ASTNodePtr ast = std::make_shared<FuncCall>(funcname, args);
        return ast;
    }
    default:
        error("Expect primary token");
    }
}

ASTNodePtr Parser::parse_multiplicative()
{
    ASTNodePtr lhs = parse_primary();

    while (lex_.is(TOK::STAR) || lex_.is(TOK::SLASH)) {
        bool isMul = lex_.get().kind == TOK::STAR;
        ASTNodePtr rhs = parse_primary();
        lhs =
            std::make_shared<BinOp>(isMul ? BINOP::MUL : BINOP::DIV, lhs, rhs);
    }

    return lhs;
}

ASTNodePtr Parser::parse_additive()
{
    ASTNodePtr lhs = parse_multiplicative();

    while (lex_.is(TOK::PLUS) || lex_.is(TOK::MINUS)) {
        bool isAdd = lex_.get().kind == TOK::PLUS;
        ASTNodePtr rhs = parse_multiplicative();
        lhs =
            std::make_shared<BinOp>(isAdd ? BINOP::ADD : BINOP::SUB, lhs, rhs);
    }

    return lhs;
}

ASTNodePtr Parser::parse_expr()
{
    return parse_additive();
}

ASTNodePtr Parser::parse()
{
    return parse_expr();
}

// Pretty print
std::ostream &operator<<(std::ostream &os, const MPRational &r)
{
    os << r2f(r).str();
    return os;
}

int main(int argc, char **argv)
{
    Lex lex(std::cin);
    Parser parser(lex);
    std::cout << parser.parse()->eval() << std::endl;
}
