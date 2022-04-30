#pragma once

#ifndef __LOGGER_H__
    #define __LOGGER_H__

    #include "sys_PrintT.h"
    #include "sys_StringT.h"
    #include <iomanip>
    #include <sstream>
    #include <ArduinoJson.h>

namespace rdl {

    #ifndef DEC
        #define DEC 10
    #endif

    class Null_Print : public sys::PrintT {
     public:
        virtual size_t write(const uint8_t) override { return 0; }
        virtual size_t write(const uint8_t*, size_t) override { return 0; }
        virtual int availableForWrite() override { return 0; }
        virtual void flush() override {}
    };

    template <class DeviceT>
    class DeviceLog_Print : public sys::PrintT {
     public:
        DeviceLog_Print() : device_(nullptr), debug_only_(true) {}
        DeviceLog_Print(DeviceT* device, bool debug_only) : device_(device), debug_only_(debug_only) {}

        virtual size_t write(const uint8_t uc) override {
            char c = static_cast<char>(c);
            if (c == '\n') {
                send_to_log(1);
                return 1;
            }
            size_t w = stream_.tellp();
            stream_.put(c);
            w = stream_.tellp() - w;
            return w;
        }
        virtual size_t write(const uint8_t* ucbuf, size_t size) override {
            const char* buf = reinterpret_cast<const char*>(ucbuf);
            if (strncmp(buf, "\r\n", 2)) {
                send_to_log(2);
                return 2;
            }
            size_t w = stream_.tellp();
            stream_.write(buf, size);
            w = stream_.tellp() - w;
            return w;
        }
        virtual int availableForWrite() override { return SIZE_MAX; }
        virtual void flush() override {}

     protected:
        void send_to_log(int endsize) {
            if (device_) {
                sys::StringT str = stream_.str();
                size_t len  = str.length();
                device_->LogMessage(str, len - endsize, debug_only_);
            }
            stream_.str("");
        }

        DeviceT* device_;
        std::ostringstream stream_;
        bool debug_only_;
    };

    size_t print(sys::PrintT& printer, JsonDocument& doc) {
        sys::StringT strdoc;
        ArduinoJson::serializeJson(doc, strdoc);
        return printer.print("JSON:") + printer.print(strdoc);
    }
    size_t println(sys::PrintT& printer, JsonDocument& doc) {
        return print(printer, doc) + printer.println();
    }

    inline size_t print_escaped(sys::PrintT& printer, const char* buf, size_t size, const char* brackets = "\"") {
        size_t written = 0;
        size_t br_len  = brackets ? strlen(brackets) : 0;
        if (br_len > 0)
            written += printer.print(brackets[0]);
        static char c_escapes[] = {
            '\0', '0', '\'', '\'', '\"', '"', '\?', '?', '\\', '\\', '\a', 'a',
            '\b', 'b', '\f', 'f', '\n', 'n', '\r', 'r', '\t', 't', '\v', 'v'};
        const char* buf_end = buf + size;
        for (; buf != buf_end; buf++) {
            unsigned char uc = static_cast<unsigned char>(buf[0]);
            size_t c_esc;
            for (c_esc = 0; c_esc < sizeof(c_escapes); c_esc += 2) {
                if (uc == c_escapes[c_esc])
                    break;
            }
            if (c_esc < sizeof(c_escapes)) {
                char sc = c_escapes[c_esc + 1];
                written += printer.print('\\') + printer.print(sc);
            } else if (isprint(uc)) {
                written += printer.print(buf[0]);
            } else {
                written += printer.print("\\x") + printer.print(uc, 16);
            }
        }
        if (br_len > 0) {
            if (br_len > 1)
                written += printer.print(brackets[1]);
            else
                written += printer.print(brackets[0]);
        }
        return written;
    }

    inline size_t print_escaped(sys::PrintT& printer, const unsigned char* buf, size_t size, const char* brackets = "\"") {
        return print_escaped(printer, reinterpret_cast<const char*>(buf), size, brackets);
    }

}; // namespace rdl

#endif // __LOGGER_H__
