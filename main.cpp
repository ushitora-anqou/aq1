#include <cassert>
#include <iostream>
#include <optional>
#include <variant>

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>

namespace mp = boost::multiprecision;
using MPRational = boost::rational<mp::cpp_int>;

#define AQ1_ASSERT(cond, msg)    \
    do {                         \
        assert((cond) && (msg)); \
    } while (0);

[[noreturn]] void error(const char *msg)
{
    std::cerr << "[ERROR]\t" << msg;
    std::terminate();
}

enum class TOK {
    NUMLIT,  // Numeric literal
    OWARI,   // End of file
    PLUS,
};

struct Token {
    TOK kind;
    std::variant<std::monostate, MPRational> data;

    static const Token &owari()
    {
        static const Token tok{TOK::OWARI, std::monostate{}};
        return tok;
    }
};

class Lex {
private:
    std::istream &is_;
    std::optional<Token> pending_;

public:
    Lex(std::istream &is) : is_(is), pending_(std::nullopt)
    {
    }

    Token next();
    bool is(TOK kind);
};

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
    } while (isspace(ch));

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
    }

    // No valid token
    is_.putback(ch);
    return Token::owari();
}

bool Lex::is(TOK kind)
{
    if (!pending_) pending_ = next();
    return pending_->kind == kind;
}

class Evaluator {
private:
    Lex &lex_;

public:
    Evaluator(Lex &lex) : lex_(lex)
    {
    }

    MPRational eval();
};

MPRational Evaluator::eval()
{
    Token tok = lex_.next();
    if (tok.kind == TOK::NUMLIT) {
        MPRational val = std::get<MPRational>(tok.data);
        while (lex_.is(TOK::PLUS)) {
            lex_.next();  // Eat TOK::PLUS
            tok = lex_.next();
            if (tok.kind != TOK::NUMLIT) error("Invalid addition");
            val += std::get<MPRational>(tok.data);
        }
        return val;
    }

    error("Can't parse");
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
    Evaluator evalor(lex);
    std::cout << evalor.eval() << std::endl;
}
