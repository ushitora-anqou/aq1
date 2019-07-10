#ifndef AQ1_MAIN_HPP
#define AQ1_MAIN_HPP

#include <iostream>
#include <memory>
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
    STAR,
    SLASH,
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

class ASTNode {
public:
    ASTNode() = default;

    // Thanks to:
    // https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rc-copy-virtual
    // Thanks to:
    // https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rc-five
    virtual ~ASTNode() = default;
    ASTNode(const ASTNode &) = delete;
    ASTNode &operator=(const ASTNode &) = delete;
    ASTNode(ASTNode &&) = delete;
    ASTNode &operator=(ASTNode &&) = delete;

    virtual MPRational eval() const = 0;
};

using ASTNodePtr = std::shared_ptr<ASTNode>;

enum class BINOP {
    ADD,
    SUB,
    MUL,
    DIV,
};

class BinOp : public ASTNode {
private:
    BINOP kind_;
    ASTNodePtr lhs_, rhs_;

public:
    BinOp(BINOP kind, ASTNodePtr lhs, ASTNodePtr rhs)
        : kind_(kind), lhs_(lhs), rhs_(rhs)
    {
    }

    MPRational eval() const override;
};

class NumImm : public ASTNode {
private:
    MPRational val_;

public:
    NumImm(MPRational val) : val_(val)
    {
    }

    MPRational eval() const override
    {
        return val_;
    }
};

class Parser {
private:
    Lex &lex_;

    ASTNodePtr parse_primary();
    ASTNodePtr parse_multiplicative();
    ASTNodePtr parse_additive();

public:
    Parser(Lex &lex) : lex_(lex)
    {
    }

    ASTNodePtr parse();
};

#endif
