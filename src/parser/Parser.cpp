#include "../../include/parser/Parser.hpp"
#include "../../include/parser/Lexer.hpp"
#include "../../include/expressions/expressions.hpp"

#include <string>
#include <stdexcept>

template<typename T>
Token Parser<T>::consume(const TokenType expect) {
    if (cur_token.type != expect) {
        throw std::runtime_error(std::format("Unexpected token: \"{}\"", cur_token.value));
    }
    return cur_token = lexer.next_token();
}

template<typename T>
Token Parser<T>::advance() {
    return cur_token = lexer.next_token();
}

template<>
std::shared_ptr<BaseExpr<RealNumber>> Parser<RealNumber>::parse_real_number() {
    auto res = std::make_shared<Constant<RealNumber>>(std::stold(cur_token.value));
    advance();
    return res;
}

template<>
std::shared_ptr<BaseExpr<ComplexNumber>> Parser<ComplexNumber>::parse_real_number() {
    auto real_part = ComplexNumber(std::stold(cur_token.value), 0);
    auto res = std::make_shared<Constant<ComplexNumber>>(real_part);
    advance();
    return res;
}

template<>
std::shared_ptr<BaseExpr<RealNumber>> Parser<RealNumber>::parse_imaginary_unit() {
    throw std::runtime_error("Can not parse imaginary unit in Parser<RealNumber>");
}

template<>
std::shared_ptr<BaseExpr<ComplexNumber>> Parser<ComplexNumber>::parse_imaginary_unit() {
    constexpr auto imaginary_unit = ComplexNumber(0, 1);
    auto res = std::make_shared<Constant<ComplexNumber>>(imaginary_unit);
    advance();
    return res;
}

template<typename T>
std::shared_ptr<BaseExpr<T>> Parser<T>::parse_identifier() {
    auto res = std::make_shared<Variable<T>>(cur_token.value);
    advance();
    return res;
}

/// parentheses_expr
///   ::= '(' expression ')'
template<typename T>
std::shared_ptr<BaseExpr<T>> Parser<T>::parse_parentheses_expr() {
    consume(OpeningParen);
    auto expr = parse_expression();
    consume(ClosingParen);
    return expr;
}

/// function
///   ::= <func_name>( expression ')'
template<typename T>
std::shared_ptr<BaseExpr<T>> Parser<T>::parse_function() {
    const std::string func_name = cur_token.value.substr(0, cur_token.value.length() - 1);
    consume(Function);
    auto expr = parse_expression();
    consume(ClosingParen);
    return Func<T>::from_name(func_name, expr);
}

/// primary
///   ::= identifier
///   ::= real_number
///   ::= imaginary_unit
///   ::= parentheses_expr
///   ::= function
template<typename T>
std::shared_ptr<BaseExpr<T>> Parser<T>::parse_primary() {
    switch (cur_token.type) {
    case OpeningParen:
        return parse_parentheses_expr();
    case Function:
        return parse_function();
    case RNumber:
        return parse_real_number();
    case ImaginaryUnit:
        return parse_imaginary_unit();
    case Identifier:
        return parse_identifier();
    default:
        throw std::runtime_error(std::format("Unexpected token: \"{}\"", cur_token.value));
    }
}

/// bin_op_rhs
///   ::= ('+' primary)*
template<typename T>
std::shared_ptr<BaseExpr<T>> Parser<T>::parse_bin_op_rhs(
    const OpPrecedence expr_precedence, std::shared_ptr<BaseExpr<T>> lhs
) {
    while (cur_token.type != EOL && cur_token.type != ClosingParen) {
        if (cur_token.type != BinOperator) {
            throw std::runtime_error(std::format("Expected binary operator, got: \"{}\"", cur_token.value));
        }
        const std::string bin_op = cur_token.value;
        const auto bin_op_precedence = BinOp<T>::get_precedence_by_name(bin_op);
        if (bin_op_precedence < expr_precedence) {
            break;
        }
        advance();
        auto rhs = parse_primary();
        if (cur_token.type == BinOperator) {
            const std::string next_bin_op = cur_token.value;
            const auto next_bin_op_precedence = BinOp<T>::get_precedence_by_name(next_bin_op);
            if (bin_op_precedence < next_bin_op_precedence) {
                rhs = parse_bin_op_rhs(OpPrecedence(int(bin_op_precedence) + 1), rhs);
            }
        }
        lhs = BinOp<T>::from_name(bin_op, lhs, rhs);
    }
    return lhs;
}

/// expression
///   ::= primary bin_op_rhs
template<typename T>
std::shared_ptr<BaseExpr<T>> Parser<T>::parse_expression() {
    auto lhs = parse_primary();
    return parse_bin_op_rhs(OpPrecedence::AddSub, lhs);
}

template<typename T>
Parser<T>::Parser(
    const std::string& expression_str, const bool case_sensitive
) : lexer(expression_str, case_sensitive) {
    cur_token = lexer.next_token();
}

template<typename T>
Expression<T> Parser<T>::parse() {
    return Expression<T>(parse_expression());
}

template class Parser<RealNumber>;
template class Parser<ComplexNumber>;