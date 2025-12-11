#ifndef LEXER_HPP
#define LEXER_HPP

#include <complex>
#include <string>
#include <regex>
#include <optional>
#include <unordered_map>

enum TokenType {
    RNumber,
    ImaginaryUnit,
    Identifier,
    BinOperator,
    OpeningParen,
    ClosingParen,
    Function,
    EOL
};

const std::string TokenTypeNames[] = {  // FIXME: for test only
    "RNumber",
    "ImaginaryUnit",
    "Variable",
    "BinaryOperator",
    "OpeningParenthesis",
    "ClosingParenthesis",
    "Function",
    "End"
};

const std::unordered_map<TokenType, std::regex> REGEX_PATTERN_MAP = {
    {RNumber, std::regex("(0|[1-9][0-9]*)(\\.[0-9]+)?")},
    {Identifier, std::regex("[a-zA-Z_]+")},
    {Function, std::regex("(sin|cos|ln|exp)\\(", std::regex_constants::icase)}
};

struct Token {
    TokenType type;
    std::string value;
};

template<typename T>
class Lexer {
public:
    explicit Lexer(const std::string& input, bool case_sensitive = false);

    Token next_token();

private:
    std::string str;
    std::string::const_iterator it;
    TokenType prev_token_type = EOL;
    std::optional<Token> discarded_token = std::nullopt;

    char peek() const;
    void advance();

    Token get_token_from_str();
    std::optional<std::string> get_str_by_pattern(const std::regex& regexp);
};

#endif  // LEXER_HPP