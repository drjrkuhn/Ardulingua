#pragma once

#ifndef __POLYFILLS_STD_UTILITY_H__
    #define __POLYFILLS_STD_UTILITY_H__

// TODO: Substitute Embedded Template Library for Arduino without
// https://www.etlcpp.com/

// from https://gist.github.com/ntessore/dc17769676fb3c6daa1f

    #ifdef __has_include
        #if __has_include(<utility>)
            #include <utility>
            #if (__cplusplus >= 201402L || _MSVC_LANG >= 201402L) // for integer_sequence
                #define HAS_INTEGER_SEQUENCE 1
            #else
                #define HAS_INTEGER_SEQUENCE 0
            #endif
        #endif
    #endif // __has_include

    #if !(HAS_INTEGER_SEQUENCE)

    // C++11 version of integer_sequence
        #include <cstddef> // for size_t
namespace std {
    template <typename T, T... Ints>
    struct integer_sequence {
        typedef T value_type;
        static constexpr std::size_t size() { return sizeof...(Ints); }
    };

    template <std::size_t... Ints>
    using index_sequence = integer_sequence<std::size_t, Ints...>;

    template <typename T, std::size_t N, T... Is>
    struct make_integer_sequence : make_integer_sequence<T, N - 1, N - 1, Is...> {
    };

    template <typename T, T... Is>
    struct make_integer_sequence<T, 0, Is...> : integer_sequence<T, Is...> {
    };

    template <std::size_t N>
    using make_index_sequence = make_integer_sequence<std::size_t, N>;

    template <typename... T>
    using index_sequence_for = make_index_sequence<sizeof...(T)>;
} // namespace
    #endif // !(HAS_INTEGER_SEQUENCE)

#endif // __POLYFILLS_STD_UTILITY_H__
