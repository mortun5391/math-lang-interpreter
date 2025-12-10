#include "../include/longnum.hpp"
#include <algorithm>
#include <utility>
#include <stdexcept>

constexpr unsigned BASE = 32;
constexpr unsigned DEFAULT_PRECISION = 64;

namespace longnum {
    LongNum::LongNum(const bool _is_negative, const unsigned _exp, std::vector<uint32_t>& _limbs)
            : is_negative(_is_negative),
              exp(_exp),
              limbs(std::move(_limbs)) {
    }

    LongNum::LongNum(unsigned long long x) {
        if (x == 0) {
            return;
        }
        while (x) {
            limbs.push_back(x & UINT32_MAX);
            x >>= BASE;
        }
    }

    LongNum::LongNum(long long x) {
        if (x == 0) {
            return;
        }
        is_negative = x < 0;
        x = std::abs(x);
        while (x) {
            limbs.push_back(x & UINT32_MAX);
            x >>= BASE;
        }
    }

    bool operator==(const LongNum& lhs, const LongNum& rhs) {
        if (lhs.limbs.empty() || rhs.limbs.empty()) {  // when lhs = 0 or rhs = 0
            return lhs.limbs.empty() && rhs.limbs.empty();
        }
        if (lhs.is_negative != rhs.is_negative) {
            return false;
        }
        if (lhs.exp < rhs.exp) {
            return lhs.with_precision(rhs.exp) == rhs;
        }
        if (lhs.exp > rhs.exp) {
            return lhs == rhs.with_precision(lhs.exp);
        }
        return lhs.limbs == rhs.limbs;
    }

    std::strong_ordering operator<=>(const LongNum& lhs, const LongNum& rhs) {
        if (lhs.limbs.empty() && rhs.limbs.empty()) {  // lhs = rhs = 0
            return std::strong_ordering::equal;
        }
        if (lhs.limbs.empty()) {  // lhs = 0
            return rhs.is_negative ? std::strong_ordering::greater : std::strong_ordering::less;
        }
        if (rhs.limbs.empty()) {  // rhs = 0
            return lhs.is_negative ? std::strong_ordering::less : std::strong_ordering::greater;
        }
        if (lhs.is_negative && !rhs.is_negative) {
            return std::strong_ordering::less;
        }
        if (!lhs.is_negative && rhs.is_negative) {
            return std::strong_ordering::greater;
        }
        if (lhs.exp < rhs.exp) {
            return lhs.with_precision(rhs.exp) <=> rhs;
        }
        if (lhs.exp > rhs.exp) {
            return lhs <=> rhs.with_precision(lhs.exp);
        }
        if (lhs.limbs.size() < rhs.limbs.size()) {
            return lhs.is_negative ? std::strong_ordering::greater : std::strong_ordering::less;
        }
        if (lhs.limbs.size() > rhs.limbs.size()) {
            return lhs.is_negative ? std::strong_ordering::less : std::strong_ordering::greater;
        }
        for (size_t i = 0; i < lhs.limbs.size(); i++) {
            if (lhs.limbs[i] < rhs.limbs[i]) {
                return lhs.is_negative ? std::strong_ordering::greater : std::strong_ordering::less;
            }
            if (lhs.limbs[i] > rhs.limbs[i]) {
                return lhs.is_negative ? std::strong_ordering::less : std::strong_ordering::greater;
            }
        }
        return std::strong_ordering::equal;
    }

    LongNum LongNum::operator+() const {
        return *this;
    }

    LongNum LongNum::operator-() const {
        LongNum res(*this);
        if (*this != 0) {  // -0 must be +0
            res.is_negative ^= 1;
        }
        return res;
    }

    LongNum& LongNum::operator<<=(const unsigned shift) {
        *this = *this << shift;
        return *this;
    }

    LongNum operator<<(const LongNum& number, const unsigned shift) {
        if (!shift || number == 0) {
            return number;
        }
        const unsigned zeros_cnt = (shift + BASE - 1) / BASE;
        const unsigned r = shift % BASE;
        std::vector<uint32_t> limbs(number.limbs.size() + zeros_cnt, 0);
        for (size_t i = zeros_cnt; i < limbs.size(); i++) {
            limbs[i] = number.limbs[i - zeros_cnt];
        }
        if (r) {
            for (size_t i = zeros_cnt - 1; i < limbs.size() - 1; i++) {
                limbs[i] = (limbs[i] >> (BASE - r)) | (limbs[i + 1] << r);
            }
            limbs.back() >>= BASE - r;
        }
        LongNum res(number.is_negative, number.exp, limbs);
        res.remove_leading_zeros();
        return res;
    }

    LongNum& LongNum::operator>>=(const unsigned shift) {
        *this = *this >> shift;
        return *this;
    }

    LongNum operator>>(const LongNum& number, const unsigned shift) {
        if (!shift || number == 0) {
            return number;
        }
        const unsigned to_erase = shift / BASE;
        if (to_erase >= number.limbs.size()) {
            return (0_longnum).with_precision(number.exp);
        }
        std::vector<uint32_t> limbs(number.limbs.size() - to_erase);
        const unsigned r = shift % BASE;
        for (size_t i = 0; i < limbs.size(); i++) {
            limbs[i] = number.limbs[i + to_erase];
        }
        if (r) {
            for (size_t i = 0; i < limbs.size() - 1; i++) {
                limbs[i] = (limbs[i] >> r) | (limbs[i + 1] << (BASE - r));
            }
            limbs.back() >>= r;
        }
        LongNum res(number.is_negative, number.exp, limbs);
        res.remove_leading_zeros();
        return res;
    }

    LongNum& LongNum::operator+=(const LongNum& rhs) {
        if (*this == 0) {
            *this = rhs.with_precision(std::max(exp, rhs.exp));
            return *this;
        }
        if (rhs == 0) {
            set_precision(std::max(exp, rhs.exp));
            return *this;
        }
        if (is_negative != rhs.is_negative) {
            *this -= -rhs;
            return *this;
        }
        if (exp < rhs.exp) {
            set_precision(rhs.exp);
        } else if (exp > rhs.exp) {
            *this += rhs.with_precision(exp);  // recursive call, this can only happen once
            return *this;
        }
        unsigned carry = 0;
        for (size_t i = 0; i < std::max(limbs.size(), rhs.limbs.size()) || carry; i++) {
            if (i == limbs.size()) {
                limbs.push_back(0);
            }
            const uint64_t sum = (uint64_t)limbs[i] + carry + (i < rhs.limbs.size() ? rhs.limbs[i] : 0);
            limbs[i] = sum;
            carry = sum >> BASE;
        }
        return *this;
    }

    LongNum operator+(LongNum lhs, const LongNum& rhs) {
        lhs += rhs;
        return lhs;
    }

    LongNum& LongNum::operator-=(const LongNum& rhs) {
        if (rhs == 0) {
            set_precision(std::max(exp, rhs.exp));
            return *this;
        }
        if (*this == rhs) {
            *this = (0_longnum).with_precision(std::max(exp, rhs.exp));
            return *this;
        }
        if (is_negative != rhs.is_negative) {
            *this += -rhs;
            return *this;
        }
        if ((*this < rhs) ^ is_negative) {  // handle cases like -1 - (-2) and 1 - 2
            *this = -(rhs - *this);
            return *this;
        }
        if (exp < rhs.exp) {
            set_precision(rhs.exp);
        } else if (exp > rhs.exp) {
            *this -= rhs.with_precision(exp);  // recursive call, this can only happen once
            return *this;
        }
        unsigned carry = 0;
        for (size_t i = 0; i < rhs.limbs.size() || carry; i++) {
            if (i < rhs.limbs.size()) {
                const unsigned new_carry = limbs[i] < rhs.limbs[i] || (limbs[i] == rhs.limbs[i] && carry);  // TODO: optimize carry calculation here
                limbs[i] -= rhs.limbs[i] + carry;
                carry = new_carry;
            } else {
                limbs[i] -= carry;
                carry = (limbs[i] == UINT32_MAX);
            }
        }
        remove_leading_zeros();
        return *this;
    }

    LongNum operator-(LongNum lhs, const LongNum& rhs) {
        lhs -= rhs;
        return lhs;
    }

    void LongNum::set_precision(const unsigned precision) {
        if (exp < precision) {
            *this <<= precision - exp;
        } else if (exp > precision) {
            *this >>= exp - precision;
        }
        exp = precision;
    }

    LongNum LongNum::with_precision(const unsigned precision) const {
        if (exp < precision) {
            LongNum res = *this << (precision - exp);
            res.exp = precision;
            return res;
        }
        if (exp > precision) {
            LongNum res = *this >> (exp - precision);
            res.exp = precision;
            return res;
        }
        return *this;
    }

    void LongNum::remove_leading_zeros() {
        while (!limbs.empty() && limbs.back() == 0) {
            limbs.pop_back();
        }
        if (limbs.empty()) {
            is_negative = false;
        }
    }

    std::string LongNum::to_binary_string() const {
        if (*this == 0) {
            return "0";
        }
        std::string res;
        if (is_negative) {
            res = "-";
        }
        for (uint32_t limb : limbs) {
            for (unsigned j = 0; j < BASE; j++) {
                res += std::to_string((limb >> j) & 1);
            }
        }
        while (res.back() == '0' && res.length() > exp + 1) {
            res.pop_back();
        }
        if (exp > 0) {
            res.insert(res.begin() + exp, '.');
        }
        std::ranges::reverse(res);
        return res;
    }

    LongNum LongNum::from_binary_string(std::string str) {
        if (str.empty()) {
            throw std::invalid_argument("Invalid string");
        }
        LongNum res;
        const size_t point_pos = str.find_first_of(".,");
        if (point_pos == std::string::npos) {
            res.exp = 0;
        } else {
            res.exp = str.size() - point_pos - 1;
            str.erase(str.begin() + point_pos);
        }
        if (str.find_first_of(".,") != std::string::npos) {
            throw std::invalid_argument("Invalid string");
        }
        for (const auto c: str) {
            if (c != '0' && c != '1') {
                throw std::invalid_argument("Invalid string");
            }
        }
        for (int i = (int)str.size() - 1; i >= 0; i -= BASE) {
            res.limbs.push_back(0);
            for (unsigned j = 0; j < std::min(BASE, (unsigned)i + 1); j++) {
                if (str[i - j] == '1') {
                    res.limbs.back() |= (1 << j);
                }
            }
        }
        if (res.exp < DEFAULT_PRECISION) {
            res.set_precision(DEFAULT_PRECISION);
        }
        return res;
    }

    std::istream& operator>>(std::istream& stream, LongNum& number) {
        std::string str;
        stream >> str;
        number = LongNum::from_binary_string(str);
        return stream;
    }

    std::ostream& operator<<(std::ostream& stream, const LongNum& number) {
        return stream << number.to_binary_string();
    }
}
