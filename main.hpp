#ifndef AQ1_MAIN_HPP
#define AQ1_MAIN_HPP

#include <iostream>
#include <optional>
#include <variant>

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>

namespace mp = boost::multiprecision;
using MPRational = boost::rational<mp::cpp_int>;

enum class TOK {
    NUMLIT,  // Numeric literal
    OWARI,   // End of file
    PLUS,
    MINUS,
    LFCR,
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

    Token get();
    Token get_skipping_lfcr();
    bool is(TOK kind);
};

class Evaluator {
private:
    Lex &lex_;

public:
    Evaluator(Lex &lex) : lex_(lex)
    {
    }

    MPRational eval();
};

#endif
