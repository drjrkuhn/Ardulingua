#pragma once

#ifndef __RDL_COMMON_H__
    #define __RDL_COMMON_H__

    #if !defined(__ALWAYS_INLINE__)
        #if (defined(__GNUC__) && __GNUC__ > 3) || defined(__clang__)
            #define __ALWAYS_INLINE__ inline __attribute__((__always_inline__))
            #define __DEPRECATED__ __attribute__((deprecated))
        #elif defined(_MSC_VER)
            // Does nothing in Debug mode (with standard option /Ob0)
            #define __ALWAYS_INLINE__ inline __forceinline
            #define __DEPRECATED__ __declspec(deprecated)
        #else
            #define __ALWAYS_INLINE__ inline
            #define __DEPRECATED__ [[deprecated]]
        #endif
    #endif

namespace rdl {

    // template <typename T>
    // struct __DEPRECATED__ debug_type;

    // template <typename T>
    // inline void debug_type(const T&) {}

    // template <typename T>
    // __DEPRECATED__ inline void debug_type();

    // template <typename T>
    // inline void debug_type() {}

    ///// Test compile with
    // typedef .... ComplicatedT
    // CompicatedT instance;
    //
    // debug_type<ComplicatedT>();
    // debug_type(instance);

}

#endif //__RDL_COMMON_H__