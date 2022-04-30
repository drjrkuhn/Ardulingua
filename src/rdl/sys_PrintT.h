#pragma once

#ifndef __SYS_PRINTT_H__
    #define __SYS_PRINTT_H__

#include "Common.h"

    #ifdef ARDUINO
        #ifdef __has_include
            #if __has_include(<Printable.h>)
                #include <Printable.h>
            #else
                #error Could not find Arduino Printable definition
            #endif
            #if __has_include(<Print.h>)
                #include <Print.h>
            #else
                #error Could not find Arduino Print definition
            #endif
        #endif
namespace sys {
    using PrintT = ::Print;
    using PrintableT = ::Printable;
}

    #else // NOT ARDUINO
        #include "Polyfills/Printable_Mock.h"
        #include "Polyfills/Print_Mock.h"
        #include "Polyfills/Print_MockIOS.h"
namespace sys {
    using PrintableT = sys::Printable;
    using PrintT = sys::Print;
    // don't define Print_xxxxT type aliases for other print mocks
    // test code should use directly
}
    #endif // NOT ARDUINO

#endif // __SYS_PRINTT_H__