#pragma once
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

#ifndef __PRINT_MOCK_H__
#define __PRINT_MOCK_H__

#include "../sys_StringT.h"
#include "../std_type_traits.h"
#include "Print_MockHelp.h"
#include "Printable_Mock.h"

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

/**
 * Arduino-compatible Print class using STL strings
 */
namespace sys {

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
            size_t w = 0;
            while (size--) {
                if (write(*buffer++))
                    w++;
                else
                    break;
            }
            return w;
        }

        // default to zero, meaning "a single write may block"
        // should be overridden by subclasses with buffering
        virtual int availableForWrite() { return 0; }
        virtual void flush() {}

        int getWriteError() { return write_error; }
        void clearWriteError() { setWriteError(0); }

        size_t write(const char* str) {
            if (str == NULL) return 0;
            return write((const uint8_t*)str, strlen(str));
        }

        // iterator version of write
        size_t write_from(const char* begin, const char* end) {
            return write(&begin[0], end - begin);
        }
        size_t write(const sys::StringT& str) {
            return write(str.c_str(), str.length());
        }
        size_t write(const char* buffer, size_t size) {
            return write((const uint8_t*)buffer, size);
        }

        size_t print(const sys::StringT& s) { return write(s.c_str(), s.length()); }
        size_t print(const rdl::__FlashStringHelper* ifsh) { return print(reinterpret_cast<const char*>(ifsh)); }
        size_t print(const char str[]) { return write(str); }
        size_t print(char c) { return write(c); }
        size_t print(unsigned char n, int base = DEC) {
            if (base == 0)
                return write(n);
            else
                return write(to_string(n, base));
        }
        size_t print(short n, int base = DEC) { return write(to_string(n, base)); }
        size_t print(unsigned short n, int base = DEC) { return write(to_string(n, base)); }
        size_t print(int n, int base = DEC) { return write(to_string(n, base)); }
        size_t print(unsigned int n, int base = DEC) { return write(to_string(n, base)); }
        size_t print(long n, int base = DEC) { return write(to_string(n, base)); }
        size_t print(unsigned long n, int base = DEC) { return write(to_string(n, base)); }
        size_t print(long long n, int base = DEC) { return write(to_string(n, base)); }
        size_t print(unsigned long long n, int base = DEC) { return write(to_string(n, base)); }
        size_t print(float n, int digits = 2) { return write(to_string(n, digits)); }
        size_t print(double n, int digits = 2) { return write(to_string(n, digits)); }
        size_t print(const Printable& x) { return x.printTo(*this); }

        size_t println(const __FlashStringHelper* ifsh) {
            size_t w = print(ifsh) + println();
            w += println();
            return w;
        }
        size_t println(const sys::StringT& str) {
            size_t w = print(str);
            w += println();
            return w;
        }
        size_t println(const char c[]) {
            size_t w = print(c);
            w += println();
            return w;
        }
        size_t println(char c) {
            size_t w = print(c);
            w += println();
            return w;
        }
        size_t println(unsigned char n, int base = DEC) {
            size_t w = print(n, base);
            w += println();
            return w;
        }
        size_t println(short n, int base = DEC) {
            size_t w = print(n, base);
            w += println();
            return w;
        }
        size_t println(unsigned short n, int base = DEC) {
            size_t w = print(n, base);
            w += println();
            return w;
        }
        size_t println(int n, int base = DEC) {
            size_t w = print(n, base);
            w += println();
            return w;
        }
        size_t println(unsigned int n, int base = DEC) {
            size_t w = print(n, base);
            w += println();
            return w;
        }
        size_t println(long n, int base = DEC) {
            size_t w = print(n, base);
            w += println();
            return w;
        }
        size_t println(unsigned long n, int base = DEC) {
            size_t w = print(n, base);
            w += println();
            return w;
        }
        size_t println(long long n, int base = DEC) {
            size_t w = print(n, base);
            w += println();
            return w;
        }
        size_t println(unsigned long long n, int base = DEC) {
            size_t w = print(n, base);
            w += println();
            return w;
        }
        size_t println(float n, int digits = 2) {
            size_t w = print(n, digits);
            w += println();
            return w;
        }
        size_t println(double n, int digits = 2) {
            size_t w = print(n, digits);
            w += println();
            return w;
        }
        size_t println(const Printable& x) {
            size_t w = print(x);
            w += println();
            return w;
        }

        size_t println() {
            return write("\r\n");
        }
    };

} // namespace

#endif // __PRINT_MOCK_H__