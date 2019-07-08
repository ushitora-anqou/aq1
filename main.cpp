#include <cassert>
#include <iostream>
#include <variant>

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>

namespace mp = boost::multiprecision;
using MPRational = boost::rational<mp::cpp_int>;

enum class TOK {
    NUMLIT,  // Numeric literal
    OWARI,   // End of file
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

public:
    Lex(std::istream &is) : is_(is)
    {
    }

    Token next();
};

Token Lex::next()
{
    assert(is_.good() && "Invalid input stream.");
    int ch = is_.get();
    if (ch == EOF) return Token::owari();  // TOKEN NO OWARI

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

    // No valid token
    is_.putback(ch);
    return Token::owari();
}

// Pretty print
std::ostream &operator<<(std::ostream &os, const MPRational &r)
{
    if (r.denominator() == 1) {  // When Integer
        os << r.numerator();
    }
    else {  // Convert fractions to floating points
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
    Token tok = lex.next();
    std::cout << std::get<MPRational>(tok.data) << std::endl;
}
