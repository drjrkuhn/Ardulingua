/*
  Copyright (c) 2016 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include "Printable.h"
#include "StringHelpers.h"
#include "WString.h"
#include <inttypes.h>
#include <iterator>
#include <stdio.h> // for size_t
#include <string>
#include <type_traits>

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

namespace arduino {

    class Print {
     private:
        int write_error;

     protected:
        void setWriteError(int err = 1) { write_error = err; }

     public:
        Print() : write_error(0) {}
        virtual ~Print() = default;

        virtual size_t write(const uint8_t) = 0;
        virtual size_t write(const uint8_t* buffer, size_t size) {
            size_t n = 0;
            while (size--) {
                if (write(*buffer++))
                    n++;
                else
                    break;
            }
            return n;
        }
        // default to zero, meaning "a single write may block"
        // should be overridden by subclasses with buffering
        virtual int availableForWrite() { return 0; }
        virtual void flush() { /* Empty implementation for backward compatibility */
        }

        int getWriteError() { return write_error; }
        void clearWriteError() { setWriteError(0); }

        size_t write(const char* str) {
            if (str == NULL) return 0;
            return write((const uint8_t*)str, strlen(str));
        }

        // iterator version of write
        template <typename CharIT, typename std::enable_if<is_char_iterator<CharIT>::value, bool>::type = false>
        size_t write_from(CharIT begin, CharIT end) {
            return write(&begin[0], end - begin);
        }
        size_t write(const std::string& str) {
            return write_from(str.begin(), str.end());
        }
        size_t write(const char* buffer, size_t size) {
            return write((const uint8_t*)buffer, size);
        }

        size_t print(const __FlashStringHelper*) {
            return print(reinterpret_cast<const char*>(ifsh));
        }
        size_t print(const String&) {
            return write(s.c_str(), s.length());
        }
        template <typename CharIT, typename std::enable_if<is_char_iterator<CharIT>::value, bool>::type = false>
        size_t print_from(CharIT begin, CharIT end) { return write_from(begin, end); }
        size_t print(const std::string& str) {
            return write_from(str.begin(), str.end());
        }
        size_t print(const char[]) {
            return write(str);
        }
        size_t print(char) {
            return write(c);
        }
        size_t print(unsigned char, int = DEC) {
            if (base == 0)
                return write(n);
            else
                return write(to_string(n, base));
        }
        size_t print(short, int = DEC) {
            return write(to_string(n, base));
        }
        size_t print(unsigned short, int = DEC) {
            return write(to_string(n, base));
        }
        size_t print(int, int = DEC) {
            return write(to_string(n, base));
        }
        size_t print(unsigned int, int = DEC) {
            return write(to_string(n, base));
        }
        size_t print(long, int = DEC) {
            return write(to_string(n, base));
        }
        size_t print(unsigned long, int = DEC) {
            return write(to_string(n, base));
        }
        size_t print(long long, int = DEC) {
            return write(to_string(n, base));
        }
        size_t print(unsigned long long, int = DEC) {
            return write(to_string(n, base));
        }
        size_t print(float, int = 2) {
            return write(to_string(n, digits));
        }
        size_t print(double, int = 2) {
            return write(to_string(n, digits));
        }
        size_t print(const Printable&) {
            return x.printTo(*this);
        }

        size_t println(const __FlashStringHelper*) {
            size_t n = print(ifsh);
            n += println();
            return n;
        }
        size_t println(const String& s) {
            size_t n = print(s);
            n += println();
            return n;
        }
        template <typename CharIT, typename std::enable_if<is_char_iterator<CharIT>::value, bool>::type = false>
        size_t println_from(CharIT begin, CharIT end) { return write_from(begin, end) + println(); }
        size_t println(const std::string& str) {
            size_t n = print(str);
            n += println();
            return n;
        }
        size_t println(const char[]) {
            size_t n = print(c);
            n += println();
            return n;
        }
        size_t println(char) {
            size_t n = print(c);
            n += println();
            return n;
        }
        size_t println(unsigned char, int = DEC) {
            size_t n = print(b, base);
            n += println();
            return n;
        }
        size_t println(short, int = DEC) {
            size_t n = print(num, base);
            n += println();
            return n;
        }
        size_t println(unsigned short, int = DEC) {
            size_t n = print(num, base);
            n += println();
            return n;
        }
        size_t println(int, int = DEC) {
            size_t n = print(num, base);
            n += println();
            return n;
        }
        size_t println(unsigned int, int = DEC) {
            size_t n = print(num, base);
            n += println();
            return n;
        }
        size_t println(long, int = DEC) {
            size_t n = print(num, base);
            n += println();
            return n;
        }
        size_t println(unsigned long, int = DEC) {
            size_t n = print(num, base);
            n += println();
            return n;
        }
        size_t println(long long, int = DEC) {
            size_t n = print(num, base);
            n += println();
            return n;
        }
        size_t println(unsigned long long, int = DEC) {
            size_t n = print(num, base);
            n += println();
            return n;
        }
        size_t println(float, int = 2) {
            size_t n = print(num, digits);
            n += println();
            return n;
        }
        size_t println(double, int = 2) {
            size_t n = print(num, digits);
            n += println();
            return n;
        }
        size_t println(const Printable&) {
            size_t n = print(x);
            n += println();
            return n;
        }
        size_t println(void) {
            return write("\r\n");
        }
    };

} // namespace arduino

using arduino::Print;