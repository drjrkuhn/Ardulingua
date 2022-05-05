/** Human readable SLIP escapes for development */

#pragma once

#include <cassert>
#include <rdl/SlipInPlace.h>
#include <rdl/sys_StringT.h>

#ifndef __HRSLIP_H__
    #define __HRSLIP_H__

namespace rdl {
    /* Human readable SLIP codes */
    struct slip_hr_codes {
        static constexpr uint8_t SLIP_END      = '#'; ///< 0xC0
        static constexpr uint8_t SLIP_ESCEND   = 'D'; ///< 0xDC
        static constexpr uint8_t SLIP_ESC      = '^'; ///< 0xDB
        static constexpr uint8_t SLIP_ESCESC   = '['; ///< 0xDD
        static constexpr uint8_t SLIPX_NULL    = 0;   ///< 0 (nonstandard)
        static constexpr uint8_t SLIPX_ESCNULL = 0;   ///< tag for NO NULL encoding
    };

    /* Human readable SLIP+NULL codes */
    struct slip_hrnull_codes {
        static constexpr uint8_t SLIP_END      = '#'; ///< 0xC0
        static constexpr uint8_t SLIP_ESCEND   = 'D'; ///< 0xDC
        static constexpr uint8_t SLIP_ESC      = '^'; ///< 0xDB
        static constexpr uint8_t SLIP_ESCESC   = '['; ///< 0xDD
        static constexpr uint8_t SLIPX_NULL    = '0'; ///< 0 (nonstandard)
        static constexpr uint8_t SLIPX_ESCNULL = '@'; ///< tag for NO NULL encoding
    };

    typedef rdl::svc::encoder_base<char, slip_hr_codes> slip_encoder_hr;
    typedef rdl::svc::decoder_base<char, slip_hr_codes> slip_decoder_hr;

    typedef rdl::svc::encoder_base<char, slip_hrnull_codes> slip_encoder_hrnull;
    typedef rdl::svc::decoder_base<char, slip_hrnull_codes> slip_decoder_hrnull;

    template <class FROM, class TO>
    sys::StringT recode(const sys::StringT& src) {
        sys::StringT dest = src;
        using to_char_t   = typename TO::char_type;
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
    sys::StringT recode(const char* src, size_t size) {
        return recode<FROM, TO>(sys::StringT(src, size));
    }
};

#endif // __HRSLIP_H__
