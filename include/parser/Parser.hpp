#ifndef PARSER_HPP
#define PARSER_HPP

#include "Lexer.hpp"
#include "../expressions/expressions.hpp"

template<typename T = RealNumber>
class Parser {
public:
    explicit Parser(const std::string& expression_str, bool case_sensitive = false);
    Expression<T> parse();

private:
    Lexer<T> lexer;
    Token cur_token;

    Token advance();
    Token consume(TokenType expect);

    std::shared_ptr<BaseExpr<T>> parse_real_number();
    std::shared_ptr<BaseExpr<T>> parse_imaginary_unit();
    std::shared_ptr<BaseExpr<T>> parse_identifier();
    std::shared_ptr<BaseExpr<T>> parse_parentheses_expr();
    std::shared_ptr<BaseExpr<T>> parse_function();
    std::shared_ptr<BaseExpr<T>> parse_primary();
    std::shared_ptr<BaseExpr<T>> parse_bin_op_rhs(OpPrecedence expr_precedence, std::shared_ptr<BaseExpr<T>> lhs);
    std::shared_ptr<BaseExpr<T>> parse_expression();
};

#endif  // PARSER_HPP