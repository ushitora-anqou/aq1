#include <cassert>
#include <iostream>
#include <variant>

enum class TOK {
    NUMLIT,  // Numeric literal
    OWARI,   // End of file
};

struct Token {
    TOK kind;
    std::variant<std::monostate, double> data;

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
        double n = ch - '0';
        while (isdigit(ch = is_.get())) n = n * 10. + ch - '0';
        is_.putback(ch);
        return {TOK::NUMLIT, n};
    }

    // No valid token
    is_.putback(ch);
    return Token::owari();
}

int main(int argc, char **argv)
{
    Lex lex(std::cin);
    Token tok = lex.next();
    std::cout << std::get<double>(tok.data) << std::endl;
}
