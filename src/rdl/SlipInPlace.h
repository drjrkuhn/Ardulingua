/*!
 *  @file SlipInPlace.h
 *
 *  @mainpage SlipInPlace
 *
 *  @section intro_sec Introduction
 *
 *  Library for in-place SLIP encoding and decoding.
 *  Optional SLIP+NUL codec allows encoding of buffers with NULL characters
 *  in the middle for string-based communication.
 *
 *  @section author Author
 *
 *  Written by Jeffrey Kuhn <jrkuhn@mit.edu>.
 *
 *  @section license License
 *
 *  MIT license, all text above must be included in any redistribution
 */

#pragma once

#ifndef __SLIPINPLACE_H
    #define __SLIPINPLACE_H__

/**
 * @brief Unroll codelet testing loop, defaults to true (1).
 *
 * To turn off codelet loop unrolling, set this macro to
 * false (0) before including the library header.
 *
 * ```c++
 * #define SLIP_UNROLL_LOOPS 0
 * #include <SlipInPlace.h>
 * ```
 */

    #if !defined(SLIP_UNROLL_LOOPS)
        #define SLIP_UNROLL_LOOPS 1
    #endif

    #include "std_type_traits.h" // for enable_if
    #include "Common.h"
    #include <ctype.h>  // for isprint
    #include <stdint.h> // for uint8_t
    #include <stdlib.h> // for itoa
    #include <string.h> // for memmove, strlen

namespace rdl {

    /**************************************************************************************
     * Standard and extended SLIP character codes
     **************************************************************************************/

    /* Standard SLIP codes: END=\300 ESC=\333 ESCEND=\334 ESCESC=\335. */
    struct slip_stdcodes {
        static constexpr uint8_t SLIP_END      = 0300; ///< 0xC0
        static constexpr uint8_t SLIP_ESCEND   = 0334; ///< 0xDC
        static constexpr uint8_t SLIP_ESC      = 0333; ///< 0xDB
        static constexpr uint8_t SLIP_ESCESC   = 0335; ///< 0xDD
        static constexpr uint8_t SLIPX_NULL    = 0;    ///< 0 (nonstandard)
        static constexpr uint8_t SLIPX_ESCNULL = 0336; ///< 0xDE (nonstandard)
    };

    namespace svc {

        /**************************************************************************************
     * Base for both encoders and decoders
     **************************************************************************************/

        /**
     * @brief Base container for custom SLIP codes.
     *
     * targets C++11 - no variable or static data member at struct/class scope
     * [(since C++14)](https://en.cppreference.com/w/cpp/language/variable_template)
     *
     * @tparam _CharT       unsigned char or char
     * @tparam _EndC        end character code \300
     * @tparam _EscEndC     escaped end character code \334
     * @tparam _EscC        escape character code \333
     * @tparam _EscEscC     escaped escape character code \334
     * @tparam _NullC       NULL character code \000
     * @tparam _EscNullC    escaped NULL character code. If set to anything other than zero, NULLs will be processed
     *
     */
        template <typename _CharT, uint8_t _EndC, uint8_t _EscEndC, uint8_t _EscC, uint8_t _EscEscC,
                  uint8_t _NullC = 0, uint8_t _EscNullC = 0>
        struct slip_base {
            using char_type = _CharT;
            static constexpr _CharT end_code() noexcept { return (_CharT)_EndC; }         ///< end code
            static constexpr _CharT escend_code() noexcept { return (_CharT)_EscEndC; }   ///< escaped end code
            static constexpr _CharT esc_code() noexcept { return (_CharT)_EscC; }         ///< escape code
            static constexpr _CharT escesc_code() noexcept { return (_CharT)_EscEscC; }   ///< escped escape code
            static constexpr _CharT null_code() noexcept { return (_CharT)_NullC; }       ///< NULL code
            static constexpr _CharT escnull_code() noexcept { return (_CharT)_EscNullC; } ///< escaped NULL code if present

            /** Maximum number of special characters to escape while encoding */
            static constexpr int max_specials = 3;

            /** doest this encoder/decoder encode for the NULL character? */
            static constexpr bool is_null_encoded = (_EscNullC != 0);

            /**
         *  Number of special characters in this codec.
         *  Standard SLIP uses two (end and esc). SLIP+NULL adds a third code.
         */
            static constexpr int num_specials = (is_null_encoded ? 3 : 2);

         protected:
            ///////////////////////////////////////////////////////////////
            // TODO: Try to implement static variable workaround similar to
            // [N3651](https://isocpp.org/files/papers/N3651.pdf)
            ///////////////////////////////////////////////////////////////

            /** An array of special characters to escape */
            static __ALWAYS_INLINE__ const _CharT* special_codes() noexcept {
                // Work around no-statics in header-only libraries under C++11. Should stay valid until program exit
                static constexpr _CharT specials[] = {end_code(), esc_code(), null_code()};
                return specials;
            }
            /** An array of special character escapes in the same order as special_codes(). */
            static __ALWAYS_INLINE__ const _CharT* escaped_codes() noexcept {
                // Work around no-statics in header-only libraries under C++11. Should stay valid until program exit
                static constexpr _CharT escapes[] = {escend_code(), escesc_code(), escnull_code()};
                return escapes;
            }

    #ifdef SLIP_UNROLL_LOOPS
            static __ALWAYS_INLINE__ int test_codes(const _CharT c, const _CharT* codes) {
                static_assert(max_specials == 3, "too many codecs to unroll. Recompile with -DSLIP_UNROLL_LOOPS=0");
                // a good compiler will notice the short-circuit constexpr evaluation
                // in the first term and eliminate the entire test if false
                if (num_specials > 2 && c == codes[2]) return 2;
                if (num_specials > 1 && c == codes[1]) return 1;
                if (num_specials > 0 && c == codes[0]) return 0;
                return -1;
            }
        };
    #else
            static __ALWAYS_INLINE__ int test_codes(const _CharT c, const _CharT* codes) {
                int i = num_specials;
                while (--i >= 0) {
                    if (c == codes[i])
                        break;
                };
                return i;
            }
        };
    #endif

        /**************************************************************************************
     * Base encoder
     **************************************************************************************/

        /**
     * @brief Basic SLIP encoder.
     *
     * Automatically handles out-of-place encoding via copy or in-place encoding given
     * a buffer of sufficient size.
     *
     * @tparam _CharT       unsigned char or char
     * @tparam _EndC        end character code \300
     * @tparam _EscEndC     escaped end character code \334
     * @tparam _EscC        escape character code \333
     * @tparam _EscEscC     escaped escape character code \334
     * @tparam _NullC       NULL character code \000
     * @tparam _EscNullC    escaped NULL character code \335 (non-standard)
     */
        template <typename _CharT, uint8_t _EndC, uint8_t _EscEndC, uint8_t _EscC, uint8_t _EscEscC,
                  uint8_t _NullC = 0, uint8_t _EscNullC = 0>
        struct encoder_base : public slip_base<_CharT, _EndC, _EscEndC, _EscC, _EscEscC, _NullC, _EscNullC> {
            using BASE = slip_base<_CharT, _EndC, _EscEndC, _EscC, _EscEscC, _NullC, _EscNullC>;
            using BASE::end_code;
            using BASE::escend_code;
            using BASE::esc_code;
            using BASE::escesc_code;
            using BASE::null_code;
            using BASE::escnull_code;
            using BASE::max_specials;
            using BASE::num_specials;
            using BASE::is_null_encoded;
            using BASE::special_codes;
            using BASE::escaped_codes;

            /**
         * @brief Pre-calculate the size after SLIP encoding.
         *
         * @param src       pointer to source buffer
         * @param srcsize   size of source buffer to parse
         * @return size_t   size needed to encode this buffer
         */
            static inline size_t encoded_size(const _CharT* src, size_t srcsize) noexcept {
                static const _CharT* specials = special_codes();
                const _CharT* buf_end         = src + srcsize;
                size_t nspecial               = 0;
                int isp;
                for (; src < buf_end; src++) {
                    isp = BASE::test_codes(src[0], specials);
                    if (isp >= 0) nspecial++;
                }
                return srcsize + nspecial + 1;
            }

            /**
         * @brief Encode a buffer using a SLIP protocol.
         *
         * Automatically handles out-of-place encoding via copy or in-place encoding given
         * a buffer of sufficient size.
         *
         * Encoding always expands the size of the source packet.
         * For in-place encoding, the algorithm first copies the string to the end of
         * the destination buffer, then begins encoding left-to-right.
         *
         * > :warning: Encode in-place clobbers the end of the destiation buffer past
         * >            the returned size!
         *
         * @param dest      destination buffer
         * @param destsize  dest buffer size - must be sufficiently large
         * @param src       source buffer
         * @param srcsize   size of source to encode
         * @return size_t   final encoded size or 0 if there was an error while encoding
         */
            static inline size_t encode(_CharT* dest, size_t destsize, const _CharT* src, size_t srcsize) noexcept {
                static constexpr size_t BAD_DECODE = 0;
                static const _CharT* specials      = special_codes();
                static const _CharT* escapes       = escaped_codes();
                const _CharT* send                 = src + srcsize;
                _CharT* dstart                     = dest;
                _CharT* dend                       = dest + destsize;
                if (!dest || !src || destsize < srcsize + 1)
                    return BAD_DECODE;
                if (dest <= src && src <= dend) { // sbuf somewhere in dbuf. So in-place
                    // copy source to end of the dest_buf. memmove copies overlaps in reverse. Use
                    // std::copy_backward if converting this function to iterators
                    src  = (_CharT*)memmove(dest + destsize - srcsize, src, srcsize);
                    send = src + srcsize;
                }
                int isp;

                while (src < send) {
                    isp = BASE::test_codes(src[0], specials);
                    if (isp < 0) { // regular character
                        if (dest >= dend) return BAD_DECODE;
                        *(dest++) = *(src++); // copy it
                    } else {
                        if (dest + 1 >= dend) return BAD_DECODE;
                        *(dest++) = esc_code();
                        *(dest++) = escapes[isp];
                        src++;
                    }
                }

                if (dest >= dend) {
                    return BAD_DECODE;
                }
                *(dest++) = end_code();
                return dest - dstart;
            }

            /**
         * @copydoc encoded_size
         * @tparam _FromT must have same element size as _CharT
         */
            template <typename _FromT,
                      typename std::enable_if<sizeof(_FromT) == sizeof(_CharT), bool>::type = true>
            static inline size_t encoded_size(const _FromT* src, size_t srcsize) noexcept {
                return encoded_size(reinterpret_cast<const _CharT*>(src), srcsize);
            }

            /**
         * @copydoc encode
         * @tparam _FromT must have same element size as _CharT
         */
            template <typename _FromT,
                      typename std::enable_if<sizeof(_FromT) == sizeof(_CharT), bool>::type = true>
            static inline size_t encode(_FromT* dest, size_t destsize, const _FromT* src, size_t srcsize) noexcept {
                return encode(reinterpret_cast<_CharT*>(dest), destsize, reinterpret_cast<const _CharT*>(src), srcsize);
            }
        };

        /**************************************************************************************
     * Base decoder
     **************************************************************************************/

        /**
     * @brief Basic SLIP decoder.
     *
     * Automatically handles both out-of-place and in-place decoding.
     *
     * @tparam _CharT       unsigned char or char
     * @tparam _EndC        end character code \300
     * @tparam _EscEndC     escaped end character code \334
     * @tparam _EscC        escape character code \333
     * @tparam _EscEscC     escaped escape character code \334
     * @tparam _NullC       NULL character code \000
     * @tparam _EscNullC    escaped NULL character code \335 (non-standard)
     */
        template <typename _CharT, uint8_t _EndC, uint8_t _EscEndC, uint8_t _EscC, uint8_t _EscEscC,
                  uint8_t _NullC = 0, uint8_t _EscNullC = 0>
        struct decoder_base : public slip_base<_CharT, _EndC, _EscEndC, _EscC, _EscEscC, _NullC, _EscNullC> {
            using BASE = slip_base<_CharT, _EndC, _EscEndC, _EscC, _EscEscC, _NullC, _EscNullC>;
            using BASE::end_code;
            using BASE::escend_code;
            using BASE::esc_code;
            using BASE::escesc_code;
            using BASE::null_code;
            using BASE::escnull_code;
            using BASE::max_specials;
            using BASE::num_specials;
            using BASE::is_null_encoded;
            using BASE::special_codes;
            using BASE::escaped_codes;

            /**
         * @brief Pre-calculate the size after SLIP decoding.
         *
         * Does not check the validity of two-byte escape sequences, just their presence.
         *
         * @param src       pointer to source buffer
         * @param srcsize   size of source buffer to parse
         * @return size_t   size needed to decode this buffer
         */
            static inline size_t decoded_size(const _CharT* src, size_t srcsize) noexcept {
                const _CharT* bufend = src + srcsize;
                size_t nescapes      = 0;
                for (; src < bufend; src++) {
                    if (src[0] == esc_code()) {
                        nescapes++;
                        src++;
                    } else if (src[0] == end_code()) {
                        srcsize--;
                        src = bufend;
                    }
                }
                return srcsize - nescapes;
            }

            /**
         * @brief Encode a buffer using a SLIP protocol.
         *
         * Automatically handles both out-of-place and in-place decoding.
         *
         * Since the decoded size is always smaller, in-place decoding works
         * witout copying.
         *
         * > :warning: The end of destiation buffer past the returned size is not cleared.
         *
         * @param dest      destination buffer
         * @param destsize  dest buffer size - must be sufficiently large
         * @param src       source buffer
         * @param srcsize   size of source to decode
         * @return size_t   final decoded size or 0 if there was an error while decoding
         */
            static inline size_t decode(_CharT* dest, size_t destsize, const _CharT* src, size_t srcsize) noexcept {
                static constexpr size_t BAD_DECODE = 0;
                static const _CharT* specials      = special_codes();
                static const _CharT* escapes       = escaped_codes();
                const _CharT* send                 = src + srcsize;
                _CharT* dstart                     = dest;
                _CharT* dend                       = dest + destsize;
                if (!dest || !src || srcsize < 1 || destsize < 1) return BAD_DECODE;
                int isp;

                while (src < send) {
                    if (src[0] == end_code()) return dest - dstart;
                    if (src[0] != esc_code()) {              // regular character
                        if (dest >= dend) return BAD_DECODE; // not enough room for results
                        *(dest++) = *(src++);
                    } else {
                        // check char after escape
                        src++;
                        if (src >= send || dest >= dend) return BAD_DECODE;
                        isp = BASE::test_codes(src[0], escapes);
                        if (isp < 0) return BAD_DECODE; // invalid escape code
                        *(dest++) = specials[isp];
                        src++;
                    }
                }
                return dest - dstart;
            }

            /**
         * @copydoc decoded_size
         * @tparam _FromT must have same element size as _CharT
         */
            template <typename _FromT,
                      typename std::enable_if<sizeof(_FromT) == sizeof(_CharT), bool>::type = true>
            static inline size_t decoded_size(const _FromT* src, size_t srcsize) noexcept {
                return decoded_size(reinterpret_cast<const _CharT*>(src), srcsize);
            }

            /**
         * @copydoc decode
         * @tparam _FromT must have same element size as _CharT
         */
            template <typename _FromT,
                      typename std::enable_if<sizeof(_FromT) == sizeof(_CharT), bool>::type = true>
            static inline size_t decode(_FromT* dest, size_t destsize, const _FromT* src, size_t srcsize) noexcept {
                return decode(reinterpret_cast<_CharT*>(dest), destsize, reinterpret_cast<const _CharT*>(src), srcsize);
            }
        };

    }; // namespace svc

    /**************************************************************************************
     * Base for standard and extended SLIP encoders and decoders
     **************************************************************************************/

    /** standard SLIP encoder template */
    template <typename _CharT>
    using slip_encoder_base = svc::encoder_base<_CharT, slip_stdcodes::SLIP_END, slip_stdcodes::SLIP_ESCEND, slip_stdcodes::SLIP_ESC, slip_stdcodes::SLIP_ESCESC>;
    /** standard SLIP decoder template */
    template <typename _CharT>
    using slip_decoder_base = svc::decoder_base<_CharT, slip_stdcodes::SLIP_END, slip_stdcodes::SLIP_ESCEND, slip_stdcodes::SLIP_ESC, slip_stdcodes::SLIP_ESCESC>;
    /** SLIP+NULL encoder template */
    template <typename _CharT>
    using slipnull_encoder_base = svc::encoder_base<_CharT, slip_stdcodes::SLIP_END, slip_stdcodes::SLIP_ESCEND, slip_stdcodes::SLIP_ESC, slip_stdcodes::SLIP_ESCESC, slip_stdcodes::SLIPX_NULL, slip_stdcodes::SLIPX_ESCNULL>;
    /** SLIP+NULL decoder template */
    template <typename _CharT>
    using slipnull_decoder_base = svc::decoder_base<_CharT, slip_stdcodes::SLIP_END, slip_stdcodes::SLIP_ESCEND, slip_stdcodes::SLIP_ESC, slip_stdcodes::SLIP_ESCESC, slip_stdcodes::SLIPX_NULL, slip_stdcodes::SLIPX_ESCNULL>;

    /**************************************************************************************
     * Final byte-oriented (uint8_t) standard encoder and decoder
     **************************************************************************************/

    /** byte-oriented standard SLIP encoder */
    using slip_encoder = slip_encoder_base<uint8_t>;
    /** byte-oriented standard SLIP decoder */
    using slip_decoder = slip_decoder_base<uint8_t>;
    /** byte-oriented SLIP+NULL encoder */
    using slip_null_encoder = slipnull_encoder_base<uint8_t>;
    /** byte-oriented SLIP+NULL decoder */
    using slip_null_decoder = slipnull_decoder_base<uint8_t>;

    /**************************************************************************************
     * Escape code printing
     **************************************************************************************/

    template <typename _CharT>
    struct escape_encoder_base {

        static inline size_t escape(_CharT* dest, size_t destsize, const _CharT* src, size_t srcsize,
                                    const _CharT* brackets = "\"\"") noexcept {
            static constexpr size_t BAD_ENCODE = 0;
            const _CharT* send                 = src + srcsize;
            _CharT* dstart                     = dest;
            _CharT* dend                       = dest + destsize;
            if (!dest || !src || destsize < srcsize + 1)
                return BAD_ENCODE;
            if (dest <= src && src <= dend) { // sbuf somewhere in dbuf. So in-place
                // copy source to end of the dest_buf. memmove copies overlaps in reverse. Use
                // std::copy_backward if converting this function to iterators
                src  = (_CharT*)memmove(dest + destsize - srcsize, src, srcsize);
                send = src + srcsize;
            }
            if (src >= send) return BAD_ENCODE;
            size_t br_len             = brackets ? strlen(reinterpret_cast<const char*>(brackets)) : 0;
            static _CharT c_escapes[] = {
                '\0', '0',  /* NULL */
                '\'', '\'', /* single quote */
                '\"', '"',  /* double quote */
                '\?', '?',  /* question mark */
                '\\', '\\', /* backslash */
                '\a', 'a',  /* audible bell */
                '\b', 'b',  /* backspace */
                '\f', 'f',  /* formfeed */
                '\n', 'n',  /* line feed */
                '\r', 'r',  /* carriage return */
                '\t', 't',  /* horizontal tab */
                '\v', 'v'   /* vertical tab */
            };

            static constexpr size_t hbuf_size = 5;
            _CharT hbuf[hbuf_size];

            if (br_len > 0) {
                *(dest++) = brackets[0];
            }

            while (src < send && dest < dend) {
                unsigned char c = static_cast<unsigned char>(*src);
                size_t c_esc;
                for (c_esc = 0; c_esc < sizeof(c_escapes); c_esc += 2) {
                    if (c == c_escapes[c_esc])
                        break;
                }
                if (c_esc < sizeof(c_escapes)) {
                    _CharT sc = c_escapes[c_esc + 1];
                    if (dest >= dend - 2) return BAD_ENCODE;
                    *(dest++) = '\\';
                    *(dest++) = sc;
                } else if (isprint(static_cast<int>(c))) {
                    *(dest++) = c;
                } else {
                    if (dest >= dend - 4) return BAD_ENCODE;
                    char* hex = ctohex(static_cast<unsigned short>(c), hbuf, hbuf_size);
                    if (!hex) {
                        src++;
                        continue;
                    }
                    size_t hexlen = strlen(hex);
                    if (hexlen > 2) return BAD_ENCODE;
                    *(dest++) = '\\';
                    *(dest++) = 'x';
                    *(dest++) = hexlen > 1 ? *(hex++) : '0';
                    *(dest++) = *(hex++);
                }
                src++;
            }
            if (br_len > 0) {
                if (dest >= dend) return BAD_ENCODE;
                if (br_len > 1)
                    *(dest++) = brackets[1];
                else
                    *(dest++) = brackets[0];
            }
            if (dest < dend)
                *(dest) = '\0';
            return dest - dstart;
        }

        /**
         * @copydoc encode
         * @tparam _FromT must have same element size as _CharT
         */
        template <typename _FromT,
                  typename std::enable_if<sizeof(_FromT) == sizeof(_CharT), bool>::type = true>
        static inline size_t escape(_FromT* dest, size_t destsize, const _FromT* src, size_t srcsize, const _CharT* brackets = "\"\"") noexcept {
            return escape(reinterpret_cast<_CharT*>(dest), destsize, reinterpret_cast<const _CharT*>(src), srcsize, reinterpret_cast<const _CharT*>(brackets));
        }

     protected:
        static _CharT* ctohex(unsigned short c, _CharT* buf, size_t buflen) {
            if (buflen < 2) return nullptr;
            _CharT* bend = buf + buflen;
            *(--bend)    = '\0';
            unsigned short number   = c;
            do {
                int digit = number % 16;
                number    = number / 16;
                *(--bend) = digit < 10 ? digit + '0' : digit - 10 + 'A';
            } while (number != 0 && bend > buf);
            return bend;
        }
    };

    /** byte-oriented standard SLIP encoder */
    using escape_encoder = escape_encoder_base<char>;
}

#endif // __SLIPINPLACE_H__