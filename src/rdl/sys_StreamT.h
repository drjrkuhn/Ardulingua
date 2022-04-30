#pragma once

#ifndef __SYS_STREAMT_H__
    #define __SYS_STREAMT_H__

#include "Common.h"

    #ifdef ARDUINO
        #ifdef __has_include
            #if __has_include(<Stream.h>)
                #include <Stream.h>
            #else
                #error Could not find Arduino Stream definition
            #endif
        #endif
namespace sys {
    using StreamT = ::Stream;
}

    #else // NOT ARDUINO
        #include "Polyfills/Stream_Mock.h"
        #include "Polyfills/Stream_MockIOS.h"
namespace sys {
    using StreamT = sys::Stream;
    // don't define Stream_xxxxT type aliases for other print mocks
    // test code should use directly
}
    #endif // NOT ARDUINO

#endif // __SYS_STREAMT_H__