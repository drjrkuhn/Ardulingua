/*
  String library for Wiring & Arduino
  ...mostly rewritten by Paul Stoffregen...
  Copyright (c) 2009-10 Hernando Barragan.  All right reserved.
  Copyright 2011, Paul Stoffregen, paul@pjrc.com

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifdef __cplusplus

    #ifndef __ARDUINO_STRINGS__
        #define __ARDUINO_STRINGS__

        #include "ard_StringHelp.h"
        #include "string.h"
        #include <cctype>
        #include <cstdlib>
        #include <iomanip>
        #include <sstream>
        #include <string>

namespace arduino {

    class __FlashStringHelper;
    inline const __FlashStringHelper* F(const char* string_literal) { return reinterpret_cast<const __FlashStringHelper*>(string_literal); }

    // An inherited class for holding the result of a concatenation.  These
    // result objects are assumed to be writable by subsequent concatenations.
    class StringSumHelper;

    /** The arduino String class - backed by a std::string */
    class String {
        friend class StringSumHelper;
        // use a function pointer to allow for "if (s)" without the
        // complications of an operator bool(). for more information, see:
        // http://www.artima.com/cppsource/safebool.html
        typedef void (String::*StringIfHelperType)() const;
        void StringIfHelper() const {}

        constexpr size_t const FLT_MAX_DECIMAL_PLACES = 10;
        constexpr size_t const DBL_MAX_DECIMAL_PLACES = FLT_MAX_DECIMAL_PLACES;

     public:
        using iterator       = std::string::iterator;
        using const_iterator = std::string::const_iterator;

        // constructors
        String(const char* cstr = "") {
            if (cstr) buffer.assign(cstr);
        }
        String(const char* cstr, unsigned int length) {
            if (cstr) buffer.assign(cstr, length);
        }
        String(const uint8_t* cstr, unsigned int length) : String(reinterpret_cast<const char*>(cstr), length) {}
        String(const String& other) : buffer(other.buffer) {}
        String(const std::string& other) : buffer(other) {}
        String(const __FlashStringHelper* pstr) {
            if (pstr) buffer.assign(reinterpret_cast<const char*>(pstr));
        }
        explicit String(char c)
            : buffer(1, c) {
        }
        explicit String(unsigned char value, unsigned char base = 10)
            : buffer(to_string(value, base)) {
        }
        explicit String(int value, unsigned char base = 10)
            : buffer(to_string(value, base)) {
        }
        explicit String(unsigned int value, unsigned char base = 10)
            : buffer(to_string(value, base)) {
        }
        explicit String(long value, unsigned char base = 10)
            : buffer(to_string(value, base)) {
        }
        explicit String(unsigned long value, unsigned char base = 10)
            : buffer(to_string(value, base)) {
        }
        explicit String(float value, unsigned char decimalPlaces = 2) {
            std::ostringstream out;
            out.precision(std::min(static_cast<size_t>(decimalPlaces), FLT_MAX_DECIMAL_PLACES));
            out << std::fixed << value;
            buffer = out.str();
        }
        explicit String(double value, unsigned char decimalPlaces = 2) {
            std::ostringstream out;
            out.precision(std::min(static_cast<size_t>(decimalPlaces), DBL_MAX_DECIMAL_PLACES));
            out << std::fixed << value;
            buffer = out.str();
        }
        ~String(void) {}

        // move constructor
        String(String&& rval) : buffer(std::move(rval.buffer)) {
            if (this != &rval) rval.buffer.clear();
        }
        // memory management
        // return true on success, false on failure (in which case, the string
        // is left unchanged).  reserve(0), if successful, will validate an
        // invalid string (i.e., "if (s)" will be true afterwards)
        bool reserve(unsigned int size) {
            buffer.reserve(size);
            return true;
        }

        unsigned int length(void) const { return static_cast<unsigned int>(buffer.length()); }

        // creates a copy of the assigned value.  if the value is null or
        // invalid, or if the memory allocation fails, the string will be
        // marked as invalid ("if (s)" will be false).
        String& operator=(const String& rhs) {
            if (this == &rhs) return *this;
            buffer.assign(rhs.buffer);
            return *this;
        }

        String& operator=(String&& rval) {
            move(rval);
            return *this;
        }

        String& operator=(const char* cstr) {
            if (cstr)
                copy(cstr, static_cast<unsigned int>(strlen(cstr)));
            else
                buffer.clear();
            return *this;
        }

        String& operator=(const __FlashStringHelper* pstr) {
            const char* cstr = reinterpret_cast<const char*>(pstr);
            if (cstr)
                copy(cstr, static_cast<unsigned int>(strlen(cstr)));
            else
                buffer.clear();
            return *this;
        }

        String& operator=(const std::string& str) {
            copy(str);
            return *this;
        }

        // concatenate (works w/ built-in types)

        // returns true on success, false on failure (in which case, the string
        // is left unchanged).  if the argument is null or invalid, the
        // concatenation is considered unsuccessful.
        bool concat(const String& s) {
            return concat(s.buffer);
        }

        bool concat(const std::string& str) {
            buffer.append(str);
            return true;
        }

        bool concat(const char* cstr, unsigned int length) {
            if (!cstr) return false;
            buffer.append(cstr, length);
            return true;
        }

        bool concat(const char* cstr) {
            if (!cstr) return false;
            return concat(cstr, static_cast<unsigned int>(strlen(cstr)));
        }

        bool concat(char c) {
            buffer.append(1, c);
            return true;
        }

        bool concat(unsigned char num) {
            appendNumber(buffer, num, 10);
            return true;
        }

        bool concat(int num) {
            appendNumber(buffer, num, 10);
            return true;
        }

        bool concat(unsigned int num) {
            appendNumber(buffer, num, 10);
            return true;
        }

        bool concat(long num) {
            appendNumber(buffer, num, 10);
            return true;
        }

        bool concat(unsigned long num) {
            appendNumber(buffer, num, 10);
            return true;
        }

        bool concat(float num) {
            appendNumber(buffer, num, 2);
            return true;
        }

        bool concat(double num) {
            appendNumber(buffer, num, 2);
            return true;
        }

        bool concat(const __FlashStringHelper* str) {
            return concat(reinterpret_cast<const char*>(str));
        }

        // if there's not enough memory for the concatenated value, the string
        // will be left unchanged (but this isn't signalled in any way)
        String& operator+=(const String& rhs) {
            concat(rhs);
            return (*this);
        }
        String& operator+=(const char* cstr) {
            concat(cstr);
            return (*this);
        }
        String& operator+=(const std::string& str) {
            concat(str);
            return (*this);
        }
        String& operator+=(char c) {
            concat(c);
            return (*this);
        }
        String& operator+=(unsigned char num) {
            concat(num);
            return (*this);
        }
        String& operator+=(int num) {
            concat(num);
            return (*this);
        }
        String& operator+=(unsigned int num) {
            concat(num);
            return (*this);
        }
        String& operator+=(long num) {
            concat(num);
            return (*this);
        }
        String& operator+=(unsigned long num) {
            concat(num);
            return (*this);
        }
        String& operator+=(float num) {
            concat(num);
            return (*this);
        }
        String& operator+=(double num) {
            concat(num);
            return (*this);
        }
        String& operator+=(const __FlashStringHelper* str) {
            concat(str);
            return (*this);
        }

        friend StringSumHelper& operator+(const StringSumHelper& lhs, const String& rhs);
        friend StringSumHelper& operator+(const StringSumHelper& lhs, const std::string& rhs);
        friend StringSumHelper& operator+(const StringSumHelper& lhs, const char* cstr);
        friend StringSumHelper& operator+(const StringSumHelper& lhs, char c);
        friend StringSumHelper& operator+(const StringSumHelper& lhs, unsigned char num);
        friend StringSumHelper& operator+(const StringSumHelper& lhs, int num);
        friend StringSumHelper& operator+(const StringSumHelper& lhs, unsigned int num);
        friend StringSumHelper& operator+(const StringSumHelper& lhs, long num);
        friend StringSumHelper& operator+(const StringSumHelper& lhs, unsigned long num);
        friend StringSumHelper& operator+(const StringSumHelper& lhs, float num);
        friend StringSumHelper& operator+(const StringSumHelper& lhs, double num);
        friend StringSumHelper& operator+(const StringSumHelper& lhs, const __FlashStringHelper* rhs);

        // comparison (only works w/ Strings and "strings")
        operator StringIfHelperType() const { return !buffer.empty() ? &String::StringIfHelper : 0; }
        int compareTo(const String& s) const;
        int compareTo(const char* cstr) const;
        int compareTo(const std::string& str) const;
        bool equals(const String& s) const;
        bool equals(const char* cstr) const;
        bool equals(const std::string& str) const;

        friend bool operator==(const String& a, const String& b) { return a.equals(b); }
        friend bool operator==(const String& a, const char* b) { return a.equals(b); }
        friend bool operator==(const char* a, const String& b) { return b == a; }
        friend bool operator==(const String& a, const std::string& b) { return a.equals(b); }
        friend bool operator==(const std::string& a, const String& b) { return b == a; }
        friend bool operator<(const String& a, const String& b) { return a.compareTo(b) < 0; }
        friend bool operator<(const String& a, const char* b) { return a.compareTo(b) < 0; }
        friend bool operator<(const char* a, const String& b) { return b.compareTo(a) > 0; }
        friend bool operator<(const String& a, const std::string& b) { return a.compareTo(b) < 0; }
        friend bool operator<(const std::string& a, const String& b) { return b.compareTo(a) > 0; }

        friend bool operator!=(const String& a, const String& b) { return !(a == b); }
        friend bool operator!=(const String& a, const char* b) { return !(a == b); }
        friend bool operator!=(const char* a, const String& b) { return !(a == b); }
        friend bool operator!=(const String& a, const std::string& b) { return !(a == b); }
        friend bool operator!=(const std::string& a, const String& b) { return !(a == b); }
        friend bool operator>(const String& a, const String& b) { return b < a; }
        friend bool operator>(const String& a, const char* b) { return b < a; }
        friend bool operator>(const char* a, const String& b) { return b < a; }
        friend bool operator>(const String& a, const std::string& b) { return b < a; }
        friend bool operator>(const std::string& a, const String& b) { return b < a; }
        friend bool operator<=(const String& a, const String& b) { return !(b < a); }
        friend bool operator<=(const String& a, const char* b) { return !(b < a); }
        friend bool operator<=(const char* a, const String& b) { return !(b < a); }
        friend bool operator<=(const String& a, const std::string& b) { return !(b < a); }
        friend bool operator<=(const std::string& a, const String& b) { return !(b < a); }
        friend bool operator>=(const String& a, const String& b) { return !(a < b); }
        friend bool operator>=(const String& a, const char* b) { return !(a < b); }
        friend bool operator>=(const char* a, const String& b) { return !(a < b); }
        friend bool operator>=(const String& a, const std::string& b) { return !(a < b); }
        friend bool operator>=(const std::string& a, const String& b) { return !(a < b); }

        bool equalsIgnoreCase(const String& s) const;
        bool startsWith(const String& prefix) const;
        bool startsWith(const String& prefix, unsigned int offset) const;
        bool endsWith(const String& suffix) const;
        bool equalsIgnoreCase(const std::string& s) const;
        bool startsWith(const std::string& prefix) const;
        bool startsWith(const std::string& prefix, unsigned int offset) const;
        bool endsWith(const std::string& suffix) const;

        // character access
        char charAt(unsigned int index) const;
        void setCharAt(unsigned int index, char c);
        char operator[](unsigned int index) const;
        char& operator[](unsigned int index);
        void getBytes(unsigned char* buf, unsigned int bufsize, unsigned int index = 0) const;
        void toCharArray(char* buf, unsigned int bufsize, unsigned int index = 0) const { getBytes((unsigned char*)buf, bufsize, index); }
        const char* c_str() const { return buffer.c_str(); }
        const std::string str() const { return buffer; }
        std::string::iterator /*char**/ begin() { return buffer.begin(); }
        std::string::iterator /*char**/ end() { return buffer.end(); }
        std::string::const_iterator /*const char**/ cbegin() const { return buffer.cbegin(); }
        std::string::const_iterator /*const char**/ cend() const { return buffer.cend(); }

        // search
        int indexOf(char ch) const;
        int indexOf(char ch, unsigned int fromIndex) const;
        int indexOf(const String& str) const;
        int indexOf(const String& str, unsigned int fromIndex) const;
        int indexOf(const std::string& str) const;
        int indexOf(const std::string& str, unsigned int fromIndex) const;
        int lastIndexOf(char ch) const;
        int lastIndexOf(char ch, unsigned int fromIndex) const;
        int lastIndexOf(const String& str) const;
        int lastIndexOf(const String& str, unsigned int fromIndex) const;
        int lastIndexOf(const std::string& str) const;
        int lastIndexOf(const std::string& str, unsigned int fromIndex) const;
        String substring(unsigned int beginIndex) const { return substring(beginIndex, static_cast<unsigned int>(buffer.length())); };
        String substring(unsigned int beginIndex, unsigned int endIndex) const;

        // modification
        void replace(char find, char replace);
        void replace(const String& find, const String& replace);
        void remove(unsigned int index);
        void remove(unsigned int index, unsigned int count);
        void toLowerCase(void);
        void toUpperCase(void);
        void trim(void);

        // parsing/conversion
        long toInt(void) const;
        float toFloat(void) const;
        double toDouble(void) const;

     protected:
        std::string buffer; // the actual char array
     protected:
        void invalidate(void) {
            buffer.clear();
        }

        // copy and move
        String& copy(const char* cstr, unsigned int length) {
            buffer.assign(cstr, length);
            return *this;
        }
        String& copy(const __FlashStringHelper* pstr, unsigned int length) {
            buffer.assign(reinterpret_cast<const char*>(pstr), length);
            return *this;
        }
        String& copy(const std::string& str) {
            buffer.assign(str);
            return *this;
        }
        #if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__) || _MSVC_LANG >= 201402L
        void move(String& rhs) {
            if (this == &rhs) return;
            buffer = std::move(rhs.buffer);
            rhs.buffer.clear();
        }
        #endif

        // std::string index helpers
        static int posToIndex(size_t pos);
        static size_t indexToPos(unsigned int index);
    };

    class StringSumHelper : public String {
     public:
        StringSumHelper(const String& s) : String(s) {}
        StringSumHelper(const char* p) : String(p) {}
        StringSumHelper(char c) : String(c) {}
        StringSumHelper(unsigned char num) : String(num) {}
        StringSumHelper(int num) : String(num) {}
        StringSumHelper(unsigned int num) : String(num) {}
        StringSumHelper(long num) : String(num) {}
        StringSumHelper(unsigned long num) : String(num) {}
        StringSumHelper(float num) : String(num) {}
        StringSumHelper(double num) : String(num) {}
    };




    /*********************************************/
    /*  Concatenate                              */
    /*********************************************/

    StringSumHelper& operator+(const StringSumHelper& lhs, const String& rhs) {
        StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
        if (!a.concat(rhs.buffer)) a.invalidate();
        return a;
    }

    StringSumHelper& operator+(const StringSumHelper& lhs, const std::string& rhs) {
        StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
        if (!a.concat(rhs)) a.invalidate();
        return a;
    }

    StringSumHelper& operator+(const StringSumHelper& lhs, const char* cstr) {
        StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
        if (!cstr || !a.concat(cstr)) a.invalidate();
        return a;
    }

    StringSumHelper& operator+(const StringSumHelper& lhs, char c) {
        StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
        if (!a.concat(c)) a.invalidate();
        return a;
    }

    StringSumHelper& operator+(const StringSumHelper& lhs, unsigned char num) {
        StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
        if (!a.concat(num)) a.invalidate();
        return a;
    }

    StringSumHelper& operator+(const StringSumHelper& lhs, int num) {
        StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
        if (!a.concat(num)) a.invalidate();
        return a;
    }

    StringSumHelper& operator+(const StringSumHelper& lhs, unsigned int num) {
        StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
        if (!a.concat(num)) a.invalidate();
        return a;
    }

    StringSumHelper& operator+(const StringSumHelper& lhs, long num) {
        StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
        if (!a.concat(num)) a.invalidate();
        return a;
    }

    StringSumHelper& operator+(const StringSumHelper& lhs, unsigned long num) {
        StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
        if (!a.concat(num)) a.invalidate();
        return a;
    }

    StringSumHelper& operator+(const StringSumHelper& lhs, float num) {
        StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
        if (!a.concat(num)) a.invalidate();
        return a;
    }

    StringSumHelper& operator+(const StringSumHelper& lhs, double num) {
        StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
        if (!a.concat(num)) a.invalidate();
        return a;
    }

    StringSumHelper& operator+(const StringSumHelper& lhs, const __FlashStringHelper* rhs) {
        StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
        if (!a.concat(rhs)) a.invalidate();
        return a;
    }

    /*********************************************/
    /*  Comparison                               */
    /*********************************************/

    int String::compareTo(const String& s) const {
        return buffer.compare(s.buffer);
    }

    int String::compareTo(const char* cstr) const {
        return buffer.compare(cstr ? cstr : "");
    }

    int String::compareTo(const std::string& s) const {
        return buffer.compare(s);
    }

    bool String::equals(const String& s2) const {
        return compareTo(s2) == 0;
    }

    bool String::equals(const char* cstr) const {
        return compareTo(cstr) == 0;
    }

    bool String::equals(const std::string& s2) const {
        return compareTo(s2) == 0;
    }

    bool String::equalsIgnoreCase(const String& s2) const {
        return equalsIgnoreCase(s2.buffer);
    }

    bool String::startsWith(const String& s2) const {
        return startsWith(s2.buffer);
    }

    bool String::startsWith(const String& s2, unsigned int offset) const {
        return startsWith(s2.buffer, offset);
    }

    bool String::endsWith(const String& s2) const {
        return endsWith(s2.buffer);
    }

    bool String::equalsIgnoreCase(const std::string& s2) const {
        if (length() != s2.length()) return false;
        auto it1 = buffer.begin();
        auto it2 = s2.begin();
        // lengths must be same to reach here, so we can just check for end of it1
        while (it1 != buffer.end()) {
            if (tolower(*it1++) != tolower(*it2++)) return false;
        }
        return true;
        // std::equal requires C++14
        // std::equal(buffer.begin(), buffer.end(), s2.begin(), s2.end(),
        // 		[](char a, char b) { return tolower(a) == tolower(b); }
        // );
    }

    bool String::startsWith(const std::string& s2) const {
        return 0 == buffer.compare(0, s2.length(), s2);
    }

    bool String::startsWith(const std::string& s2, unsigned int offset) const {
        return 0 == buffer.compare(offset, s2.length(), s2);
    }

    bool String::endsWith(const std::string& s2) const {
        size_t len = length(), slen = s2.length();
        if (len < slen || buffer.empty() || s2.empty()) return false;
        return 0 == buffer.compare(length() - slen, slen, s2);
    }

    /*********************************************/
    /*  Character Access                         */
    /*********************************************/

    char String::charAt(unsigned int loc) const {
        return operator[](loc);
    }

    void String::setCharAt(unsigned int loc, char c) {
        if (loc < length()) buffer[loc] = c;
    }

    char& String::operator[](unsigned int index) {
        static char dummy_writable_char;
        if (index >= length()) {
            dummy_writable_char = 0;
            return dummy_writable_char;
        }
        return buffer[index];
    }

    char String::operator[](unsigned int index) const {
        return index >= length() ? 0 : buffer[index];
    }

    void String::getBytes(unsigned char* buf, unsigned int bufsize, unsigned int index) const {
        size_t len = length();
        if (!bufsize || !buf) return;
        if (index >= len) {
            buf[0] = 0;
            return;
        }
        size_t n = bufsize - 1;
        if (n > len - index) n = len - index;
        strncpy_s(reinterpret_cast<char*>(buf), bufsize, buffer.c_str() + index, n);
        buf[n] = 0;
    }

    /*********************************************/
    /*  Search                                   */
    /*********************************************/

    int String::indexOf(char c) const {
        return indexOf(c, 0);
    }

    int String::indexOf(char ch, unsigned int fromIndex) const {
        return posToIndex(buffer.find(ch, fromIndex));
    }

    int String::indexOf(const String& s2) const {
        return indexOf(s2.buffer, 0);
    }

    int String::indexOf(const String& s2, unsigned int fromIndex) const {
        return indexOf(s2.buffer, fromIndex);
    }

    int String::indexOf(const std::string& s2) const {
        return indexOf(s2, 0);
    }

    int String::indexOf(const std::string& s2, unsigned int fromIndex) const {
        return posToIndex(buffer.find(s2, fromIndex));
    }

    int String::lastIndexOf(char theChar) const {
        return lastIndexOf(theChar, length() - 1);
    }

    int String::lastIndexOf(char ch, unsigned int fromIndex) const {
        if (fromIndex >= length()) return -1;
        return posToIndex(buffer.rfind(ch, indexToPos(fromIndex)));
    }

    int String::lastIndexOf(const String& s2) const {
        return lastIndexOf(s2, length() - s2.length());
    }

    int String::lastIndexOf(const String& s2, unsigned int fromIndex) const {
        return lastIndexOf(s2.buffer, fromIndex);
    }

    int String::lastIndexOf(const std::string& s2) const {
        return lastIndexOf(s2, static_cast<unsigned int>(length() - s2.length()));
    }

    int String::lastIndexOf(const std::string& s2, unsigned int fromIndex) const {
        size_t len = length(), s2len = s2.length();
        if (s2len == 0 || len == 0 || s2len > len) return -1;
        if (fromIndex >= len) fromIndex = static_cast<unsigned int>(len - 1);
        return posToIndex(buffer.rfind(s2, indexToPos(fromIndex)));
    }

    String String::substring(unsigned int left, unsigned int right) const {
        if (left > right) std::swap(left, right);
        size_t len   = length();
        size_t first = indexToPos(left);
        if (first > len) first = len;
        size_t last = indexToPos(right);
        if (last > len) last = len;
        return buffer.substr(first, last - first);
    }

    int String::posToIndex(size_t pos) {
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }
    size_t String::indexToPos(unsigned int index) {
        return index >= std::numeric_limits<unsigned int>::max() ? std::string::npos : static_cast<size_t>(index);
    }

    /*********************************************/
    /*  Modification                             */
    /*********************************************/

    void String::replace(char find, char replace) {
        size_t pos = 0;
        while ((pos = buffer.find(find, pos)) != std::string::npos) {
            buffer[pos++] = replace;
        }
    }

    void String::replace(const String& find, const String& replace) {
        size_t len = find.length();
        size_t pos = 0;
        while ((pos = buffer.find(find.buffer, pos)) != std::string::npos) {
            buffer.replace(pos, len, replace.buffer);
        }
    }

    void String::remove(unsigned int index) {
        // Pass the biggest integer as the count. The remove method
        // below will take care of truncating it at the end of the
        // string.
        remove(index, std::numeric_limits<unsigned int>::max());
    }

    void String::remove(unsigned int index, unsigned int count) {
        if (index < buffer.length())
            buffer.erase(index, count);
    }

    void String::toLowerCase(void) {
        std::transform(buffer.begin(), buffer.end(), buffer.begin(), [](char c) -> char { return std::tolower(c); });
    }

    void String::toUpperCase(void) {
        std::transform(buffer.begin(), buffer.end(), buffer.begin(), [](char c) -> char { return std::toupper(c); });
    }

    void String::trim(void) {
        auto left = std::find_if_not(buffer.begin(), buffer.end(), [](char c) { return std::isspace(c); });
        if (left != buffer.begin()) {
            buffer.erase(buffer.begin(), left);
        }
        auto right = std::find_if_not(buffer.rbegin(), buffer.rend(), [](char c) { return std::isspace(c); });
        if (right != buffer.rbegin()) {
            buffer.erase(right.base(), buffer.end());
        }
    }

    /*********************************************/
    /*  Parsing / Conversion                     */
    /*********************************************/

    long String::toInt(void) const {
        return ::atol(c_str());
    }

    float String::toFloat(void) const {
        return static_cast<float>(::atof(c_str()));
    }

    double String::toDouble(void) const {
        return ::atof(c_str());
    }

} // namespace arduino

using arduino::__FlashStringHelper;
using arduino::String;

    #endif // __cplusplus
#endif     // __ARDUINO_STRINGS__
