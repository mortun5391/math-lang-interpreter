#ifndef LONGNUM_HPP
#define LONGNUM_HPP

#include <iostream>
#include <vector>
#include <string>
#include <cinttypes>
#include <climits>
#include <optional>

namespace longnum {
    class LongNum {
        bool is_negative = false;
        unsigned exp = 0;
        std::vector<uint32_t> limbs;

        LongNum(bool _is_negative, unsigned _exp, std::vector<uint32_t>& _limbs);

        void remove_leading_zeros();

    public:
        LongNum() = default;
        LongNum(const LongNum& other) = default;
        explicit LongNum(unsigned long long x);
        LongNum(long long x);

        ~LongNum() = default;

        LongNum& operator=(const LongNum& other) = default;

        friend bool operator==(const LongNum& lhs, const LongNum& rhs);
        friend std::strong_ordering operator<=>(const LongNum& lhs, const LongNum& rhs);

        LongNum operator+() const;
        LongNum operator-() const;

        LongNum& operator<<=(unsigned shift);
        friend LongNum operator<<(const LongNum& number, unsigned shift);

        LongNum& operator>>=(unsigned shift);
        friend LongNum operator>>(const LongNum& number, unsigned shift);

        LongNum& operator+=(const LongNum& rhs);
        friend LongNum operator+(LongNum lhs, const LongNum& rhs);

        LongNum& operator-=(const LongNum& rhs);
        friend LongNum operator-(LongNum lhs, const LongNum& rhs);

        void set_precision(unsigned precision);
        LongNum with_precision(unsigned precision) const;

        std::string to_binary_string() const;
        static LongNum from_binary_string(std::string str);
    };

    std::istream& operator>>(std::istream& stream, LongNum& number);
    std::ostream& operator<<(std::ostream& stream, const LongNum& number);
}

#endif
