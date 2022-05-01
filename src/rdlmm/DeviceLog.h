#pragma once

#ifndef __DEVICELOG_H__
    #define __DEVICELOG_H__

    #include "../rdl/sys_PrintT.h"
    #include "../rdl/sys_StringT.h"
    #include <iomanip>
    #include <limits.h> // for INT_MAX
    #include <sstream>

namespace rdlmm {

    template <class DeviceT>
    class DeviceLog_Print : public sys::PrintT {
     public:
        DeviceLog_Print() : device_(nullptr), debug_only_(true) {}
        DeviceLog_Print(DeviceT* device, bool debug_only = true) : device_(device), debug_only_(debug_only) {}

        virtual size_t write(const uint8_t uc) override {
            char c = static_cast<char>(uc);
            if (c == '\n') {
                send_to_log();
                return 1;
            }
            size_t w = ss_.tellp();
            ss_.put(c);
            w = static_cast<size_t>(ss_.tellp()) - w;
            return w;
        }
        virtual size_t write(const uint8_t* ucbuf, size_t size) override {
            const char* buf = reinterpret_cast<const char*>(ucbuf);
            if (strncmp(buf, "\r\n", 2) == 0 /*match*/) {
                send_to_log();
                return 2;
            }
            size_t w = ss_.tellp();
            ss_.write(buf, size);
            w = static_cast<size_t>(ss_.tellp()) - w;
            return w;
        }
        virtual int availableForWrite() override { return INT_MAX; }
        virtual void flush() override {}

     protected:
        // accessor for protected CDeviceBase log method
        struct accessor : DeviceT {
            static int LogMessage(DeviceT* target, sys::StringT msg, bool debug_only = false) {
                int (DeviceT::*fn)(const sys::StringT&, bool) const = &DeviceT::LogMessage;
                return (target->*fn)(msg, debug_only);
            }
            static int LogMessage(DeviceT* target, const char* msg, bool debug_only = false) {
                int (DeviceT::*fn)(const char*, bool) const = &DeviceT::LogMessage;
                return (target->*fn)(msg, debug_only);
            }
        };

        void clear() {
            // clear the stringstream buffer
            ss_.str("");
            assert(ss_.str().length() == 0);
        }

        void send_to_log() {
            const static std::string WHITESPACE = " \n\r\t\f\v";
            if (device_) {
                sys::StringT sstr = ss_.str();
                if (sstr.length() == 0)
                    return;
                // left trim
                size_t start = sstr.find_first_not_of(WHITESPACE);
                if (start == std::string::npos)
                    start = 0;
                size_t end = sstr.find_last_not_of(WHITESPACE);
                if (end == std::string::npos) {
                    clear();
                    return;
                }
                sstr = sstr.substr(start, end + 1);
                if (sstr.length() == 0) {
                    clear();
                    return;
                }
                accessor::LogMessage(device_, sstr.c_str(), debug_only_);
                clear();
            }
        }

        DeviceT* device_;
        std::stringstream ss_;
        bool debug_only_;
    };

}; // namespace rdl

#endif // __DEVICELOG_H__
