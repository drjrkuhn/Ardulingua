#pragma once

#ifndef __POLYFILLS_STD_TYPE_TRAITS_H__
    #define __POLYFILLS_STD_TYPE_TRAITS_H__

/**
 * Define necessary type_traits if STL is not available
 *
 * std::enable_if
 */

    #ifdef __has_include
        #if __has_include(<type_traits>)
            #include <type_traits>
        #else

namespace std {
    template <bool B, typename T = void>
    struct enable_if {};

    template <typename T>
    struct enable_if<true, T> { typedef T type; };
}; // namespace std

        #endif // #if __has_include(<type_traits>)
    #endif     // #ifdef __has_include

#endif // __POLYFILLS_STD_TYPE_TRAITS_H__
