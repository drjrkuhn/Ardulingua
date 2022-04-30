#pragma once

#ifndef __DEVICELOG_H__
    #define __DEVICELOG_H__

    #include "../rdl/sys_PrintT.h"
    #include "../rdl/sys_StringT.h"
    #include <iomanip>
    #include <sstream>
    #include <limits.h> // for INT_MAX

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
            size_t w = stream_.tellp();
            stream_.put(c);
            w = static_cast<size_t>(stream_.tellp()) - w;
            return w;
        }
        virtual size_t write(const uint8_t* ucbuf, size_t size) override {
            const char* buf = reinterpret_cast<const char*>(ucbuf);
            if (strncmp(buf, "\r\n", 2)) {
                send_to_log();
                return 2;
            }
            size_t w = stream_.tellp();
            stream_.write(buf, size);
            w = static_cast<size_t>(stream_.tellp()) - w;
            return w;
        }
        virtual int availableForWrite() override { return INT_MAX; }
        virtual void flush() override {}

     protected:
        // accessor for protected CDeviceBase log method
        struct accessor : DeviceT {
            static int callLogMessage(DeviceT* target, sys::StringT msg, bool debug_only) {
                int (DeviceT::*fn)(const sys::StringT&, bool) const = &DeviceT::LogMessage;
                return (target->*fn)(msg, debug_only);
            }
            static int callLogMessage(DeviceT* target, const char* msg, bool debug_only) {
                int (DeviceT::*fn)(const char*, bool) const = &DeviceT::LogMessage;
                return (target->*fn)(msg, debug_only);
            }
        };
 
        //std::string ltrim(const std::string& s) {
        //    size_t start = s.find_first_not_of(WHITESPACE);
        //    return (start == std::string::npos) ? "" : s.substr(start);
        //}

        //std::string rtrim(const std::string& s) {
        //    size_t end = s.find_last_not_of(WHITESPACE);
        //    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
        //}

        void clear() {
            // clear _ss by swapping with a new one
            std::stringstream temp;
            temp.copyfmt(stream_);
            stream_.swap(temp);
            stream_.clear();
            stream_.copyfmt(temp);
            std::swap(stream_, temp);
        }

        void send_to_log() {
            const static std::string WHITESPACE = " \n\r\t\f\v";
            if (device_) {
                sys::StringT sstr = stream_.str();
                if (sstr.length() == 0)
                    return;
                // left trim
                size_t start = sstr.find_first_not_of(WHITESPACE);
                if (start == std::string::npos)
                    start = 0;
                size_t end   = sstr.find_last_not_of(WHITESPACE);
                if (end == std::string::npos) {
                    clear();
                    return;
                }
                sstr = sstr.substr(start, end);
                if (sstr.length() == 0) {
                    clear();
                    return;
                }
                accessor::callLogMessage(device_, sstr.c_str(), debug_only_);
                clear();
            }
        }

        DeviceT* device_;
        std::stringstream stream_;
        bool debug_only_;
    };

}; // namespace rdl

#endif // __DEVICELOG_H__
