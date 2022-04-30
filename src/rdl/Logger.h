#pragma once

#ifndef __LOGGER_H__
    #define __LOGGER_H__

    #include "sys_StringT.h"
    #include <ArduinoJson.h>

class Printable;

    #include <iomanip>
    #include <sstream>

namespace rdl {

    #ifndef DEC
        #define DEC 10
    #endif

    class NULL_Printer  {
     public:
        inline size_t write(const unsigned char*, size_t) { return 0; }
        inline size_t write(const char*) { return 0; }
        inline size_t write(uint8_t) { return 0; }
        inline size_t print(const StringT&) { return 0; }
        inline size_t print(const char[]) { return 0; }
        inline size_t print(char) { return 0; }
        inline size_t print(unsigned char, int = DEC) { return 0; }
        inline size_t print(short, int = DEC) { return 0; }
        inline size_t print(unsigned short, int = DEC) { return 0; }
        inline size_t print(int, int = DEC) { return 0; }
        inline size_t print(unsigned int, int = DEC) { return 0; }
        inline size_t print(long, int = DEC) { return 0; }
        inline size_t print(unsigned long, int = DEC) { return 0; }
        inline size_t print(long long, int = DEC) { return 0; }
        inline size_t print(unsigned long long, int = DEC) { return 0; }
        inline size_t print(float, int = 2) { return 0; }
        inline size_t print(double, int = 2) { return 0; }
        inline size_t print(const Printable&) { return 0; }

        inline size_t println(const StringT&) { return 0; }
        inline size_t println(const char[]) { return 0; }
        inline size_t println(char) { return 0; }
        inline size_t println(unsigned char, int = DEC) { return 0; }
        inline size_t println(short, int = DEC) { return 0; }
        inline size_t println(unsigned short, int = DEC) { return 0; }
        inline size_t println(int, int = DEC) { return 0; }
        inline size_t println(unsigned int, int = DEC) { return 0; }
        inline size_t println(long, int = DEC) { return 0; }
        inline size_t println(unsigned long, int = DEC) { return 0; }
        inline size_t println(long long, int = DEC) { return 0; }
        inline size_t println(unsigned long long, int = DEC) { return 0; }
        inline size_t println(float, int = 2) { return 0; }
        inline size_t println(double, int = 2) { return 0; }
        inline size_t println(const Printable&) { return 0; }
        inline size_t println(void) { return 0; }
    };

    template <class DeviceT>
    class DeviceLog_Printer {
     public:
        DeviceLog_Printer() : device_(nullptr), debug_only_(debug_only) {}
        DeviceLog_Printer(DeviceT* device, bool debug_only) : device_(device), debug_only_(debug_only) {}

        inline size_t write(const unsigned char* buf, size_t size) {
            size_t s = stream_.tellp();
            stream_.write(reinterpret_cast<const char*>(buf), size);
            return stream_.tellp() - s;
        }
        inline size_t write(const char* c_str) {
            size_t s = stream_.tellp();
            stream_ << c_str;
            return stream_.tellp() - s;
        }
        inline size_t write(uint8_t c) {
            size_t s = stream_.tellp();
            stream_ << c;
            return stream_.tellp() - s;
        }
        inline size_t print(const StringT& s) { return write(s); }
        inline size_t print(const char s[]) { return write(s); }
        inline size_t print(char) { return write(c); }
        inline size_t print(unsigned char n, int base = DEC) { return print_unsigned(n, base); }
        inline size_t print(short n, int base = DEC) { return print_signed(n, base); }
        inline size_t print(unsigned short n, int base = DEC) { return print_unsigned(n, base); }
        inline size_t print(int n, int base = DEC) { return print_signed(n, base); }
        inline size_t print(unsigned int n, int base = DEC) { return print_unsigned(n, base); }
        inline size_t print(long n, int base = DEC) { return print_signed(n, base); }
        inline size_t print(unsigned long n, int base = DEC) { return print_unsigned(n, base); }
        inline size_t print(long long n, int base = DEC) { return print_signed(n, base); }
        inline size_t print(unsigned long long n, int base = DEC) { return print_unsigned(n, base); }
        inline size_t print(float n, int prec = 2) { return print(static_cast<double>(n), prec); }
        inline size_t print(double n, int prec = 2) {
            size_t s = stream_.tellp();
            stream_ << std::setprecision(prec) << std::fixed << n;
            return stream_.tellp() - s;
        }
        inline size_t print(const Printable& x) { return x.printTo(*this); }

        inline size_t println(const StringT& s) { return print(s) + println(); }
        inline size_t println(const char s[]) { return print(s) + println(); }
        inline size_t println(char c) { return print(c) + println(); }
        inline size_t println(unsigned char n, int b = DEC) { return print(n, b) + println(); }
        inline size_t println(short n, int b = DEC) { return print(n, b) + println(); }
        inline size_t println(unsigned short n, int b = DEC) { return print(n, b) + println(); }
        inline size_t println(int n, int b = DEC) { return print(n, b) + println(); }
        inline size_t println(unsigned int n, int b = DEC) { return print(n, b) + println(); }
        inline size_t println(long n, int b = DEC) { return print(n, b) + println(); }
        inline size_t println(unsigned long n, int b = DEC) { return print(n, b) + println(); }
        inline size_t println(long long n, int b = DEC) { return print(n, b) + println(); }
        inline size_t println(unsigned long long n, int b = DEC) { return print(n, b) + println(); }
        inline size_t println(float n, int p = 2) { return print(n, p) + println(); }
        inline size_t println(double n, int p = 2) { return print(n, p) + println(); }
        inline size_t println(const Printable& x) { return print(x) + println(); }

        inline size_t println(void) {
            // only send on println;
            if (device_)
                device_->LogMessage(stream_.str(), debug_only_);
            stream_.str("");
        }

     protected:
        inline size_t print_unsigned(unsigned long long n, int base = DEC) {
            size_t s = stream_.tellp();
            stream_ << std::setbase(base) << n;
            return stream_.tellp() - s;
        }
        inline size_t print_signed(long long n, int base = DEC) {
            size_t s = stream_.tellp();
            stream_ << std::setbase(base) << n;
            return stream_.tellp() - s;
        }
        DeviceT* device_;
        std::ostringstream stream_;
        bool debug_only_;
    };

    template <class Print>
    class logger_base {
     public:
        logger_base() : printer_(nullptr) {}
        logger_base(Print* printer) : printer_(printer) {}

        Print& printer() { return *printer_; }

        template <typename T>
        size_t print(T t) {
            return printer_ ? printer_->print(t) : 0;
        }
        template <typename T>
        size_t print(T t, int base) {
            return printer_ ? printer_->print(t, base) : 0;
        }
        size_t print(const char* buf, size_t size) {
            return printer_ ? printer_->write(buf, size) : 0;
        }
        size_t print(JsonDocument& doc) {
            if (!printer_) return 0;
            return printer_->print("JSON:") + ArduinoJson::serializeJson(doc, *printer_);
        }

        template <typename T>
        size_t println(T t) {
            return printer_ ? printer_->println(t) : 0;
        }
        template <typename T>
        size_t println(T t, int base) {
            return printer_ ? printer_->println(t, base) : 0;
        }
        size_t println() {
            return printer_ ? printer_->println() : 0;
        }
        size_t println(const char* buf, size_t size) {
            if (!printer_) return 0;
            return printer_->write(buf, size) + printer_->println();
        }
        size_t println(JsonDocument& doc) {
            if (!printer_) return 0;
            return printer_->print("JSON:") + ArduinoJson::serializeJson(doc, *printer_) + printer_->println();
        }

        inline size_t print_escaped(const char* buf, size_t size, const char* brackets = "\"") {
            if (!printer_) return 0;
            size_t written = 0;
            size_t br_len  = brackets ? strlen(brackets) : 0;
            if (br_len > 0)
                written += printer_->print(brackets[0]);
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
                    written += printer_->print('\\') + printer_->print(sc);
                } else if (isprint(uc)) {
                    written += printer_->print(buf[0]);
                } else {
                    written += printer_->print("\\x") + printer_->print(uc, 16);
                }
            }
            if (br_len > 0) {
                if (br_len > 1)
                    written += printer_->print(brackets[1]);
                else
                    written += printer_->print(brackets[0]);
            }
            return written;
        }

        inline size_t print_escaped(const unsigned char* buf, size_t size, const char* brackets = "\"") {
            return print_escaped(reinterpret_cast<const char*>(buf), size, brackets);
        }

     protected:
        Print* printer_;
    };

}; // namespace rdl

#endif // __LOGGER_H__
