#include "main.hpp"

#include <cassert>

#define AQ1_ASSERT(cond, msg)    \
    do {                         \
        assert((cond) && (msg)); \
    } while (0);

[[noreturn]] void error(const char *msg)
{
    std::cerr << "[ERROR]\t" << msg << std::endl;
    std::terminate();
}

[[noreturn]] void unreachable(const char *msg)
{
    AQ1_ASSERT(false, msg);
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
    if (r.denominator() == 1) {  // When Integer
        os << r.numerator();
    }
    else {  // Convert fractions to floating points
        // TODO: Much smarter way?
        mp::cpp_dec_float_100 den{r.denominator().str()},
            num{r.numerator().str()};
        mp::cpp_dec_float_100 t = num / den;
        os << t.str();
    }

    return os;
}

int main(int argc, char **argv)
{
    Lex lex(std::cin);
    Parser parser(lex);
    std::cout << parser.parse()->eval() << std::endl;
}
