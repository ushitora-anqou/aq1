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

Token Lex::get()
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

Token Lex::get_skipping_lfcr()
{
    while (true) {
        Token tok = get();
        if (tok.kind != TOK::LFCR) return tok;
    }
}

bool Lex::is(TOK kind)
{
    if (!pending_) pending_ = get();
    return pending_->kind == kind;
}

MPRational Evaluator::eval()
{
    Token tok = lex_.get_skipping_lfcr();
    if (tok.kind == TOK::NUMLIT) {
        MPRational lhs = std::get<MPRational>(tok.data);

        while (lex_.is(TOK::PLUS) || lex_.is(TOK::MINUS)) {
            bool isPlus = lex_.get().kind == TOK::PLUS;
            tok = lex_.get_skipping_lfcr();
            if (tok.kind != TOK::NUMLIT) error("Invalid addition");
            const MPRational &rhs = std::get<MPRational>(tok.data);

            if (isPlus)
                lhs += rhs;
            else
                lhs -= rhs;
        }

        return lhs;
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
