#ifndef AQ1_MAIN_HPP
#define AQ1_MAIN_HPP

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>

namespace mp = boost::multiprecision;
using MPInt = mp::cpp_int;
using MPRational = boost::rational<MPInt>;
using MPFloat = mp::cpp_dec_float_100;

enum class TOK {
    NUMLIT,  // Numeric literal
    OWARI,   // End of file
    PLUS,
    MINUS,
    STAR,
    SLASH,
    NEWLINE,
    LPAREN,
    RPAREN,
    IDENT,
    COMMA,
};

struct Token {
    TOK kind;
    std::variant<std::monostate, MPRational, std::string> data;

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
    std::vector<char> history_;

    int getch();
    void putback(int ch);
    Token next_token();

public:
    Lex(std::istream &is) : is_(is), pending_(std::nullopt)
    {
    }

    // Get the next non-NEWLINE token.
    Token get();
    // Expect the next non-NEWLINE token.
    Token expect(TOK kind);
    // Return if the next non-NEWLINE token's kind is `kind`.
    bool match(TOK kind);

    // Peek the next token. This function will NOT skip NEWLINE.
    Token peek_next();

    std::string clear_history();
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

enum class UNARYOP {
    PLUS,
    MINUS,
};

class UnaryOp : public ASTNode {
private:
    UNARYOP kind_;
    ASTNodePtr src_;

public:
    UnaryOp(UNARYOP kind, ASTNodePtr src) : kind_(kind), src_(std::move(src))
    {
    }

    MPRational eval() const override;
};

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
        : kind_(kind), lhs_(std::move(lhs)), rhs_(std::move(rhs))
    {
    }

    MPRational eval() const override;
};

class NumImm : public ASTNode {
private:
    MPRational val_;

public:
    NumImm(MPRational val) : val_(std::move(val))
    {
    }

    MPRational eval() const override
    {
        return val_;
    }
};

class FuncCall : public ASTNode {
private:
    std::string name_;
    std::vector<ASTNodePtr> args_;

public:
    FuncCall(std::string name, std::vector<ASTNodePtr> args)
        : name_(std::move(name)), args_(std::move(args))
    {
    }

    MPRational eval() const override;
};

class Parser {
private:
    Lex &lex_;

    ASTNodePtr parse_primary();
    ASTNodePtr parse_unary();
    ASTNodePtr parse_multiplicative();
    ASTNodePtr parse_additive();
    ASTNodePtr parse_expr();

public:
    Parser(Lex &lex) : lex_(lex)
    {
    }

    ASTNodePtr parse();
};

#endif
