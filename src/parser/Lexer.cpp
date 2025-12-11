#include "../../include/parser/Lexer.hpp"

#include <vector>
#include <utility>
#include <ranges>
#include <algorithm>
#include <stdexcept>
#include <type_traits>

#include "../../include/expressions/expressions.hpp"

template<typename T>
Lexer<T>::Lexer(const std::string& input, const bool case_sensitive) {
    str = input;
    if (!case_sensitive) {
        std::ranges::transform(str, str.begin(), tolower);
    }
    it = str.begin();
}

template<typename T>
char Lexer<T>::peek() const {
    return *it;
}

template<typename T>
void Lexer<T>::advance() {
    ++it;
}

template<typename T>
std::optional<std::string> Lexer<T>::get_str_by_pattern(const std::regex& regexp) {
    std::smatch match_result;
    const bool match_status = std::regex_search(
        it, str.cend(), match_result, regexp, std::regex_constants::match_continuous
    );
    if (!match_status) {
        return std::nullopt;
    }
    it = match_result[0].second;
    return std::string(match_result[0].first, match_result[0].second);
}

template<typename T>
Token Lexer<T>::get_token_from_str() {
    while (std::isspace(peek())) {
        advance();
    }
    if (peek() == '\0') {
        return Token(EOL, "EOL");
    }

    for (TokenType token_type : {
        RNumber,
        Function,
        Identifier
    }) {
        const auto str = get_str_by_pattern(REGEX_PATTERN_MAP.at(token_type));
        if (str.has_value()) {
            if constexpr (std::is_same_v<T, std::complex<long double>>) {
                if (token_type == Identifier && str.value() == "i") {
                    return Token(ImaginaryUnit);
                }
            }
            return Token(token_type, str.value());
        }
    }

    const char chr = peek();
    advance();
    switch (chr) {
    case '+':
    case '-':
    case '*':
    case '/':
    case '^':
        return Token(BinOperator, std::string(1, chr));
    case '(':
        return Token(OpeningParen, "(");
    case ')':
        return Token(ClosingParen, ")");
    default:
        throw std::runtime_error(std::format("Unexpected character: \"{}\"", std::string(1, chr)));
    }
}

template<typename T>
Token Lexer<T>::next_token() {
    if (discarded_token.has_value()) {
        Token token = discarded_token.value();
        discarded_token.reset();
        return token;
    }

    Token token = get_token_from_str();
    if (token.type == EOL) {
        return token;
    }

    // Adding implicit multiplication
    static const std::vector<std::pair<TokenType, TokenType>> pairs = {
        {RNumber,       Identifier},
        {RNumber,       Function},
        {RNumber,       OpeningParen},
        {RNumber,       ImaginaryUnit},
        {Identifier,    OpeningParen},
        {ImaginaryUnit, OpeningParen},
        {ClosingParen,  OpeningParen},
        {ClosingParen,  RNumber},
        {ClosingParen,  ImaginaryUnit},
        {ClosingParen,  Identifier},
        {ClosingParen,  Function}
    };
    if (std::ranges::find(pairs, std::pair{prev_token_type, token.type}) != pairs.end()) {
        discarded_token = token;
        prev_token_type = token.type;
        return Token(BinOperator, "*");
    }
    prev_token_type = token.type;
    return token;
}

template class Lexer<RealNumber>;
template class Lexer<ComplexNumber>;