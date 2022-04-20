#pragma once

#ifndef __LOGGER_H__
    #define __LOGGER_H__

    #include <ArduinoJson.h>

namespace rdl {

    template <class STR>
    class Print_null {
        public:
        size_t write(const char* buffer, size_t size) { return 0; }
        inline size_t print(const STR&) { return 0; }
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

        inline size_t println(const STR&) { return 0; }
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

    template <class PRINT, class STR>
    class logger_base {
     public:
        logger_base() : printer_(nullptr) {}
        logger_base(PRINT* printer) : printer_(printer) {}

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
        PRINT* printer_;
    };

}; // namespace rdl

#endif // __LOGGER_H__
