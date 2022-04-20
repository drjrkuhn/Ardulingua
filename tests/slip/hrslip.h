/** Human readable SLIP escapes for development */

#pragma once

#include <SlipInPlace.h>
#include <cassert>
#include <string>

#ifndef __HRSLIP_H__
    #define __HRSLIP_H__

namespace rdl {
    typedef rdl::encoder_base<char, '#', 'D', '^', '['> encoder_hr;
    typedef rdl::decoder_base<char, '#', 'D', '^', '['> decoder_hr;

    typedef rdl::encoder_base<char, '#', 'D', '^', '[', '0', '@'> encoder_hrnull;
    typedef rdl::decoder_base<char, '#', 'D', '^', '[', '0', '@'> decoder_hrnull;

    template <class FROM, class TO>
    std::string recode(const std::string& src) {
        std::string dest = src;
        using to_char_t = typename TO::char_type;
        if (src.length() == 0)
            return dest;
        assert(FROM::num_specials == TO::num_specials);
        for (int i = 0; i < FROM::num_specials; i++) {
            std::replace(dest.begin(), dest.end(), static_cast<to_char_t>(FROM::special_codes()[i]), TO::special_codes()[i]);
            std::replace(dest.begin(), dest.end(), static_cast<to_char_t>(FROM::escaped_codes()[i]), TO::escaped_codes()[i]);
        }
        return dest;
    }

    template <class FROM, class TO>
    std::string recode(const char* src, size_t size) {
        return recode<FROM, TO>(std::string(src, size));
    }
};

#endif // __HRSLIP_H__
