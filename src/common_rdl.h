#pragma once

#ifndef __RDL_COMMON_H__
    #define __RDL_COMMON_H__

    #if !defined(__ALWAYS_INLINE__)
        #if defined(__GNUC__) && __GNUC__ > 3
            #define __ALWAYS_INLINE__ inline __attribute__((__always_inline__))
        #elif defined(_MSC_VER)
            // Does nothing in Debug mode (with standard option /Ob0)
            #define __ALWAYS_INLINE__ inline __forceinline
        #else
            #define __ALWAYS_INLINE__ inline
        #endif
    #endif

#endif //__RDL_COMMON_H__