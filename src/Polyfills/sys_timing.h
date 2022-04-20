#pragma once

#ifndef __SYS_TIMING_H__
#define __SYS_TIMING_H__

#include <ctype.h> // for uint32_t;

#ifdef __has_include
    #if __has_include(<Arduino.h>)
        #include <Arduino.h>
        inline void sys_yield(void) {  yield();}
        inline void sys_delay(uint32_t msec) {  delay(msec);}
        inline void sys_delayMicroseconds(uint32_t usec) {  delayMicroseconds(usec);}
        inline uint32_t sys_millis(void) { return millis();}
        inline uint32_t sys_micros(void) { return micros();}
    #else
        #include <thread>
        #include <chrono>
        inline void sys_yield(void) { std::this_thread::yield();}
        inline void sys_delay(uint32_t msec) { std::this_thread::sleep_for(std::chrono::milliseconds(msec));}
        inline void sys_delayMicroseconds(uint32_t usec) { std::this_thread::sleep_for(std::chrono::microseconds(msec));}
        inline uint32_t sys_millis(void) { 
            auto now = std::chrono::system_clock::now().time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();        
        }
        inline uint32_t sys_micros(void) { 
            auto now = std::chrono::system_clock::now().time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::microseconds>(now).count();        
        }
    #endif
#endif
    


#endif // __SYS_TIMING_H__