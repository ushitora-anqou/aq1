#include "main.hpp"

#include <cassert>
#include <deque>
#include <functional>
#include <sstream>
#include <unordered_map>

#include <readline/history.h>
#include <readline/readline.h>
// GNU readline defines NEWLINE macro, but this conflicts with TOK::NEWLINE
#undef NEWLINE

#include <boost/algorithm/string/trim.hpp>
#include <boost/format.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>

#define AQ1_RANGE(cont) std::begin(cont), std::end(cont)

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

int Lex::getch()
{
    int ch = is_.get();
    if (ch != EOF) history_.push_back(ch);
    return ch;
}

void Lex::putback(int ch)
{
    if (!history_.empty()) history_.pop_back();
    is_.putback(ch);
}

std::string Lex::clear_history()
{
    std::string ret{AQ1_RANGE(history_)};
    std::vector<char>{}.swap(history_);
    return ret;
}

Token Lex::next_token()
{
    if (pending_) {
        Token ret = *pending_;
        pending_.reset();
        return ret;
    }

    int ch;
    do {
        AQ1_ASSERT(is_.good(), "Invalid input stream.");
        ch = getch();
        if (ch == EOF) return Token::owari();  // TOKEN NO OWARI
    } while (isspace(ch) && ch != '\r' && ch != '\n');

    // When numeric literal
    if (isdigit(ch)) {
        MPRational n = ch - '0';
        while (isdigit(ch = getch())) n = n * 10 + ch - '0';
        if (ch == '.') {
            MPRational digit = 1;
            while (isdigit(ch = getch())) {
                digit /= 10;
                n += digit * (ch - '0');
            }
        }
        putback(ch);
        return {TOK::NUMLIT, n};
    }

    // When identifier
    if (isalpha(ch)) {
        std::stringstream ss;
        ss << (char)ch;
        while (isalnum(ch = getch())) ss << (char)ch;
        putback(ch);
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
        return {TOK::NEWLINE};
    case '\r': {
        ch = getch();
        if (ch != '\n') putback(ch);
        return {TOK::NEWLINE};
    }
    }

    // No valid token
    putback(ch);
    return Token::owari();
}

Token Lex::get()
{
    while (true) {
        Token tok = next_token();
        if (tok.kind != TOK::NEWLINE) return tok;
    }
}

Token Lex::expect(TOK kind)
{
    Token tok = get();
    if (tok.kind != kind)
        error("Unexpected token: got \"%1%\", but expect \"%2%\"");
    return tok;
}

Token Lex::peek_next()
{
    if (!pending_) pending_ = next_token();
    return *pending_;
}

bool Lex::match(TOK kind)
{
    if (!pending_ || pending_->kind == TOK::NEWLINE) pending_ = get();
    return pending_->kind == kind;
}

MPRational UnaryOp::eval() const
{
    MPRational src = src_->eval();

    switch (kind_) {
    case UNARYOP::PLUS:
        return src;
    case UNARYOP::MINUS:
        return -src;
    }

    unreachable("Invalid unaryop's kind");
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
             {1,
              [](auto &&args) {
                  return f2r(mp::sqrt(i2f(args[0].numerator()))) /
                         f2r(mp::sqrt(i2f(args[0].denominator())));
              }}},
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

ASTNodePtr Parser::parse_unary()
{
    if (lex_.match(TOK::PLUS)) {
        lex_.get();  // Eat TOK::PLUS
        return std::make_shared<UnaryOp>(UNARYOP::PLUS, parse_primary());
    }
    if (lex_.match(TOK::MINUS)) {
        lex_.get();  // Eat TOK::MINUS
        return std::make_shared<UnaryOp>(UNARYOP::MINUS, parse_primary());
    }
    return parse_primary();
}

ASTNodePtr Parser::parse_multiplicative()
{
    ASTNodePtr lhs = parse_unary();

    while (true) {
        switch (lex_.peek_next().kind) {
        case TOK::STAR: {
            lex_.get();  // Eat TOK::STAR
            ASTNodePtr rhs = parse_unary();
            lhs = std::make_shared<BinOp>(BINOP::MUL, lhs, rhs);
        }
            continue;

        case TOK::SLASH: {
            lex_.get();  // Eat TOK::SLASH
            ASTNodePtr rhs = parse_unary();
            lhs = std::make_shared<BinOp>(BINOP::DIV, lhs, rhs);
        }
            continue;

        default:
            break;
        }
        break;
    }

    return lhs;
}

ASTNodePtr Parser::parse_additive()
{
    ASTNodePtr lhs = parse_multiplicative();

    while (true) {
        switch (lex_.peek_next().kind) {
        case TOK::PLUS: {
            lex_.get();  // Eat TOK::PLUS
            ASTNodePtr rhs = parse_multiplicative();
            lhs = std::make_shared<BinOp>(BINOP::ADD, lhs, rhs);
        }
            continue;

        case TOK::MINUS: {
            lex_.get();  // Eat TOK::MINUS
            ASTNodePtr rhs = parse_multiplicative();
            lhs = std::make_shared<BinOp>(BINOP::SUB, lhs, rhs);
        }
            continue;

        default:
            break;
        }
        break;
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

class StreamLine : public boost::iostreams::source {
private:
    std::deque<char> buffer_;

public:
    StreamLine()
    {
    }

    std::streamsize read(char *s, std::streamsize n)
    {
        assert(n >= 1);

        if (!isatty()) {  // When non-interactive (batch mode)
            int ch = std::cin.get();
            if (ch == EOF) return -1;
            s[0] = ch;
            return 1;
        }

        if (buffer_.empty()) {
            std::shared_ptr<char> buf =
                std::shared_ptr<char>{::readline(">> ")};
            if (!buf || *buf == '\0') return -1;  // EOF
            std::string line{buf.get()};
            std::copy(AQ1_RANGE(line), std::back_inserter(buffer_));
            buffer_.push_back('\n');
        }

        assert(!buffer_.empty());

        s[0] = buffer_.front();
        buffer_.pop_front();
        return 1;
    }

    static void add_history(const std::string &src)
    {
        ::add_history(src.c_str());
    }

    static bool isatty()
    {
        return ::isatty(0);
    }
};

int main(int argc, char **argv)
{
    boost::iostreams::stream<StreamLine> in{StreamLine{}};
    Lex lex{in};

    if (!StreamLine::isatty()) {  // When non-interactive (batch mode)
        std::cout << Parser{lex}.parse()->eval() << std::endl;
        return 0;
    }

    while (true) {
        ASTNodePtr node = Parser{lex}.parse();
        StreamLine::add_history(
            boost::algorithm::trim_copy(lex.clear_history()));
        std::cout << node->eval() << std::endl;
    }
}
