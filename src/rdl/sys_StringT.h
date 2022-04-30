/**
 * @file sys_StringT.h
 * @author Jeffrey Kuhn (jrkuhn@mit.edu)
 * @brief Common Strings for Arduino and Host
 * @version 0.1
 * @date 2022-04-29
 * 
 * @copyright Copyright (c) 2022
 * 
 * 
 * Arduino and host have different string types. The compiler should
 * prefer arduino::String on microcontrollers and std::string on hosts
 * with a full OS. Microntrollers should prefer arduino::String for its
 * smaller heap footprint even if the library supports the C++ STL
 * (ie some ARM, STM32, and other boards).
 * 
 * Both string implementations share enough in common that Ardulingua \algorithms 
 * should be able to work with either type transparently. Constructors from 
 * const char*, copy, move, assignment, several operators, c_str, and length.
 * Where they differ, the rdl namespace provides a common set of accessors.
 * 
 * We define platform-dependent type alias to sys::StringT below, and all
 * rdl algorithms and classes should use this common sys::StringT and accessors
 * methods defined here. The name StringT is chosen to make it more obvious
 * that this is a type alias to an underlying implementation.
 */

#pragma once

#include "Common.h"

#ifndef __SYS_STRINGT_H__
    #define __SYS_STRINGT_H__

    #ifdef ARDUINO
        #ifdef __has_include
            #if __has_include(<String.h>)
                #include <String.h>
            #elif __has_include(<WString.h>)
                #include <WString.h>
            #else
                #error Could not find Arduino String definition
            #endif
        #endif
namespace sys {
    using StringT = ::String;

    __ALWAYS_INLINE__ StringT& append(StringT& str, const char c) {
        str.concat(c);
        return str;
    }

    __ALWAYS_INLINE__ StringT StringT_from(const char c) { return StringT(c); }
}

    #else // NOT ARDUINO
        #include <string>
namespace sys {
    using StringT = std::string;
    class __FlashStringHelper;

    // No single-argument char to string constructor or append. It is always (count, char)
    __ALWAYS_INLINE__ StringT& append(StringT& str, const char c) { return str.append(1, c); }
    __ALWAYS_INLINE__ StringT StringT_from(const char c) { return StringT(1, c); }
}
    #endif // NOT ARDUINO

namespace sys {
    /**
     * Fast unordered_map hash function for strings
     * 
     * The STL doesn't define hash functions for arduino String. And 
     * some Arduino STL implementations like Andy's Workshop STL (based
     * on the SGI STL) has a string hash function that is just too simple.
     * 
     * Jenkins one-at-a-time 32-bits hash has great coverage
     * (little overlap) and fast speeds for short strings
     * 
     * see https://stackoverflow.com/questions/7666509/hash-function-for-string
     * 
     * to use in a STL hashmap with arduino String keys, declare the map as
     * @code{.cpp}
     *      using MapT = std::unordered_map<StringT, rdl::json_stub, string_hash>;
     *      MapT dispatch_map;
     * @endcode
     */
    class string_hash {
     public:
        size_t operator()(const StringT& s) const {
            size_t len      = s.length();
            const char* key = s.c_str();
            size_t hash, i;
            for (hash = i = 0; i < len; ++i) {
                hash += key[i];
                hash += (hash << 10);
                hash ^= (hash >> 6);
            }
            hash += (hash << 3);
            hash ^= (hash >> 11);
            hash += (hash << 15);
            return hash;
        }
    };

}; // namespace sys

#endif // __SYS_STRINGT_H__