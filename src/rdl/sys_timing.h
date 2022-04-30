#pragma once

#ifndef __SYS_TIMING_H__
    #define __SYS_TIMING_H__

    #include <ctype.h> // for uint32_t;

    #ifdef __has_include
        #if __has_include(<Arduino.h>)
            #include <Arduino.h>
namespace sys {
    // import from Arduino's global namespace
    // SEE devel/namespace_test for other examples of namespace alias definitions
    inline constexpr auto yield = ::yield;
    inline constexpr auto delay = ::delay;
    inline void yield(void) { ::yield(); }
    inline void delay(uint32_t msec) { ::delay(msec); }
    inline void delayMicroseconds(uint32_t usec) { ::delayMicroseconds(usec); }
    inline uint32_t millis(void) { return ::millis(); }
    inline uint32_t micros(void) { return ::micros(); }
} // namespace
        #else
            #include <chrono>
            #include <thread>
namespace sys {
    inline void yield(void) { std::this_thread::yield(); }
    inline void delay(uint32_t msec) { std::this_thread::sleep_for(std::chrono::milliseconds(msec)); }
    inline void delayMicroseconds(uint32_t usec) { std::this_thread::sleep_for(std::chrono::milliseconds(usec / 1000)); }
    inline uint32_t millis(void) {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
    }
    inline uint32_t micros(void) {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
    }
} // namespace
        #endif
    #endif

#endif // __SYS_TIMING_H__