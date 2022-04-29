#pragma once

#ifndef __ARDUINO_STRING_HELPERS__
#define __ARDUINO_STRING_HELPERS__

#include <string>
#include <type_traits>
#include <algorithm>
#include <cmath>

#if _MSC_VER < 1400

	#include <cerrno>
	errno_t strncpy_s(char* dest, size_t dsize, const char* src, size_t count) {
		if (dest==NULL || dsize==0) {
			return EINVAL;
		}
		if (src==NULL) {
			dest[0] = 0;
			return EINVAL;
		}
		if (dsize < count) {
			dest[0] = 0;
			return ERANGE;
		}
		strncpy(dest, src, count);
		return 0;
	}
#endif

namespace arduino {

	template <typename T>
	using is_char_iterator = std::is_same<typename std::iterator_traits<T>::value_type, char>;
	// constexpr bool is_char_iterator_v = std::is_same<typename std::iterator_traits<T>::value_type, char>::value;

	/**
	 * Append the value of a generic **unsigned** integer to a generic string with a generic base.
	 *
	 * Output should be the similar to the Arduino's Print.printNumber() function.
	 */
	template<typename numT, typename std::enable_if<std::is_integral<numT>::value&& std::is_unsigned<numT>::value, bool>::type = true>
	std::string& appendNumber(std::string& dest, numT number, int base, bool reversed = false)
	{
		if (base < 2) base = 2;
		// create the number string in reverse order by appending digits
		size_t startlen = dest.length();
		do {
			numT digit = number % base;
			number = number / base;
			dest.push_back(static_cast<char>(digit < 10 ? digit + '0' : digit + 'A' - 10));
		} while (number != 0);
		if (!reversed)
			std::reverse(dest.begin() + startlen, dest.end());
		return dest;
	}

	/**
	 * Append the value of a generic **signed** integer to a generic string with a generic base.
	 *
	 * Output should be the similar to the Arduino's Print.printNumber() function.
	 * Negative numbers are only represented for base=10. For other bases, negative
	 * numbers are represented as their two's complement.
	 */
	template<typename numT, typename std::enable_if<std::is_integral<numT>::value&& std::is_signed<numT>::value, bool>::type = true>
	std::string& appendNumber(std::string& dest, numT number, int base, bool reversed = false)
	{
		using unsignedN = typename std::make_unsigned<numT>::type;
		constexpr bool KEEP_REVERSED = true;
		size_t startlen = dest.length();

		if (base == 10) {
			// treat as signed
			bool isneg = number < 0;
			if (isneg) number = -number;
			appendNumber<unsignedN>(dest, static_cast<unsignedN>(number), base, KEEP_REVERSED);
			if (isneg)
				dest.push_back('-');
			if (!reversed)
				std::reverse(dest.begin() + startlen, dest.end());
			return dest;
		}
		else {
			// treat as unsigned
			return appendNumber<unsignedN>(dest, static_cast<unsignedN>(number), base, reversed);
		}
	}

	/**
	 * Append the value of a generic floating point number to a generic string.
	 *
	 * Output should be the similar to the Arduino's Print.printFloat(). It does not
	 * print exponential notation.
	 */
	template<typename numT, typename std::enable_if<std::is_floating_point<numT>::value, bool>::type = true>
	std::string& appendNumber(std::string& dest, numT number, int decimalPlaces, bool dummy = false)
	{
		if (decimalPlaces < 0)
			decimalPlaces = 2;

		if (std::isnan(number)) return dest.append("nan");
		if (std::isinf(number)) return dest.append("inf");
		if (number > 4294967040.0) return dest.append("ovf");  // constant determined empirically
		if (number < -4294967040.0) return dest.append("ovf");  // constant determined empirically
														 // Handle negative numbers
		if (number < 0.0) {
			dest.append(1, '-');
			number = -number;
		}

		// Round correctly so that print(1.999, 2) prints as "2.00"
		numT rounding = 0.5;
		for (int i = 0; i < decimalPlaces; ++i)
			rounding /= 10.0;

		number += rounding;

		// Extract the integer part of the number and print it
		unsigned long int_part = static_cast<unsigned long>(number);
		numT remainder = number - static_cast<numT>(int_part);
		appendNumber<unsigned int>(dest, int_part, 10, false);

		// Print the decimal point, but only if there are digits beyond
		if (decimalPlaces > 0) {
			dest.append(1, '.');
		}

		// Extract digits from the remainder one at a time
		while (decimalPlaces-- > 0) {
			remainder *= 10.0;
			unsigned int toPrint = static_cast<unsigned int>(remainder);
			appendNumber<unsigned int>(dest, toPrint, 10, false);
			remainder -= toPrint;
		}

		return dest;
	}

	/**
	 * Convert generic integers (signed or unsigned) or floating point to a std::string.
	 * Integers take a base as the second parameter (HEX, DEC, etc). Floating points
	 * take a decimal places as the second parameter (number of digits after the decimal).
	 *
	 * Output should be the similar to the Arduino's Print.printNumber() function.
	 */
	template<typename numT, typename std::enable_if<std::is_integral<numT>::value || std::is_floating_point<numT>::value, bool>::type = true>
	std::string to_string(numT number, int baseOrPlaces)
	{
		std::string res;
		return appendNumber(res, number, baseOrPlaces);
	}

/*********************************************/
/*  Static Member Initialisation             */
/*********************************************/

size_t const String::FLT_MAX_DECIMAL_PLACES;
size_t const String::DBL_MAX_DECIMAL_PLACES;

/*********************************************/
/*  Constructors                             */
/*********************************************/

String::String(const char *cstr) 
{
	if (cstr) buffer.assign(cstr);
}

String::String(const char *cstr, unsigned int length)
{
	if (cstr) buffer.assign(cstr, length);
}

String::String(const String &value)
	: buffer(value.buffer)
{
}

String::String(const std::string& value)
	: buffer(value)
{
}

String::String(const __FlashStringHelper *pstr)
{
	if (pstr) buffer.assign(reinterpret_cast<const char*>(pstr));
}

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__) || _MSVC_LANG >= 201402L
String::String(String &&rval)
	: buffer(std::move(rval.buffer))
{
	if (this != &rval)
		rval.buffer.clear();
}
#endif

String::String(char c)
	: buffer(1, c)
{
}

String::String(unsigned char value, unsigned char base)
	: buffer(to_string(value, base))
{
}

String::String(int value, unsigned char base)
	: buffer(to_string(value, base))
{
}

String::String(unsigned int value, unsigned char base)
	: buffer(to_string(value, base))
{
}

String::String(long value, unsigned char base)
	: buffer(to_string(value, base))
{
}

String::String(unsigned long value, unsigned char base)
	: buffer(to_string(value, base))
{
}

String::String(float value, unsigned char decimalPlaces)
{
	std::ostringstream out;
	out.precision(std::min(static_cast<size_t>(decimalPlaces), FLT_MAX_DECIMAL_PLACES));
	out << std::fixed << value;
	buffer = out.str();
}

String::String(double value, unsigned char decimalPlaces)
{
	std::ostringstream out;
	out.precision(std::min(static_cast<size_t>(decimalPlaces), DBL_MAX_DECIMAL_PLACES));
	out << std::fixed << value;
	buffer = out.str();
}

String::~String()
{
}

/*********************************************/
/*  Memory Management                        */
/*********************************************/

void String::invalidate(void)
{
	buffer.clear();
}

bool String::reserve(unsigned int size)
{
	buffer.reserve(size);
	return true;
}

/*********************************************/
/*  Copy and Move                            */
/*********************************************/

String & String::copy(const char *cstr, unsigned int length)
{
	buffer.assign(cstr, length);
	return *this;
}

String & String::copy(const __FlashStringHelper *pstr, unsigned int length)
{
	buffer.assign(reinterpret_cast<const char*>(pstr), length);
	return *this;
}

String& String::copy(const std::string& str)
{
	buffer.assign(str);
	return *this;
}

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__) || _MSVC_LANG >= 201402L
void String::move(String &rhs)
{
	if (this == &rhs) return;
	buffer = std::move(rhs.buffer);
	rhs.buffer.clear();
}
#endif

String & String::operator = (const String &rhs)
{
	if (this == &rhs) return *this;
	buffer.assign(rhs.buffer);
	return *this;
}

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__) || _MSVC_LANG >= 201402L
String & String::operator = (String &&rval)
{
	move(rval);
	return *this;
}
#endif

String & String::operator = (const char *cstr)
{
	if (cstr) 
		copy(cstr, static_cast<unsigned int>(strlen(cstr)));
	else 
		buffer.clear();
	return *this;
}

String & String::operator = (const __FlashStringHelper *pstr)
{
	const char* cstr = reinterpret_cast<const char*>(pstr);
	if (cstr)
		copy(cstr, static_cast<unsigned int>(strlen(cstr)));
	else
		buffer.clear();
	return *this;
}

String& String::operator = (const std::string& str)
{
	copy(str);
	return *this;
}

/*********************************************/
/*  concat                                   */
/*********************************************/

bool String::concat(const String &s)
{
	return concat(s.buffer);
}

bool String::concat(const std::string& str)
{
	buffer.append(str);
	return true;
}

bool String::concat(const char *cstr, unsigned int length)
{
	if (!cstr) return false;
	buffer.append(cstr, length);
	return true;
}

bool String::concat(const char *cstr)
{
	if (!cstr) return false;
	return concat(cstr, static_cast<unsigned int>(strlen(cstr)));
}

bool String::concat(char c)
{
	buffer.append(1, c);
	return true;
}

bool String::concat(unsigned char num)
{
	appendNumber(buffer, num, 10);
	return true;
}

bool String::concat(int num)
{
	appendNumber(buffer, num, 10);
	return true;
}

bool String::concat(unsigned int num)
{
	appendNumber(buffer, num, 10);
	return true;
}

bool String::concat(long num)
{
	appendNumber(buffer, num, 10);
	return true;
}

bool String::concat(unsigned long num)
{
	appendNumber(buffer, num, 10);
	return true;
}

bool String::concat(float num)
{
	appendNumber(buffer, num, 2);
	return true;
}

bool String::concat(double num)
{
	appendNumber(buffer, num, 2);
	return true;
}

bool String::concat(const __FlashStringHelper * str)
{
	return concat(reinterpret_cast<const char*>(str));
}

/*********************************************/
/*  Concatenate                              */
/*********************************************/

StringSumHelper& operator + (const StringSumHelper& lhs, const String& rhs)
{
	StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
	if (!a.concat(rhs.buffer)) a.invalidate();
	return a;
}

StringSumHelper& operator + (const StringSumHelper& lhs, const std::string& rhs)
{
	StringSumHelper& a = const_cast<StringSumHelper&>(lhs);
	if (!a.concat(rhs)) a.invalidate();
	return a;
}

StringSumHelper & operator + (const StringSumHelper &lhs, const char *cstr)
{
	StringSumHelper &a = const_cast<StringSumHelper&>(lhs);
	if (!cstr || !a.concat(cstr)) a.invalidate();
	return a;
}

StringSumHelper & operator + (const StringSumHelper &lhs, char c)
{
	StringSumHelper &a = const_cast<StringSumHelper&>(lhs);
	if (!a.concat(c)) a.invalidate();
	return a;
}

StringSumHelper & operator + (const StringSumHelper &lhs, unsigned char num)
{
	StringSumHelper &a = const_cast<StringSumHelper&>(lhs);
	if (!a.concat(num)) a.invalidate();
	return a;
}

StringSumHelper & operator + (const StringSumHelper &lhs, int num)
{
	StringSumHelper &a = const_cast<StringSumHelper&>(lhs);
	if (!a.concat(num)) a.invalidate();
	return a;
}

StringSumHelper & operator + (const StringSumHelper &lhs, unsigned int num)
{
	StringSumHelper &a = const_cast<StringSumHelper&>(lhs);
	if (!a.concat(num)) a.invalidate();
	return a;
}

StringSumHelper & operator + (const StringSumHelper &lhs, long num)
{
	StringSumHelper &a = const_cast<StringSumHelper&>(lhs);
	if (!a.concat(num)) a.invalidate();
	return a;
}

StringSumHelper & operator + (const StringSumHelper &lhs, unsigned long num)
{
	StringSumHelper &a = const_cast<StringSumHelper&>(lhs);
	if (!a.concat(num)) a.invalidate();
	return a;
}

StringSumHelper & operator + (const StringSumHelper &lhs, float num)
{
	StringSumHelper &a = const_cast<StringSumHelper&>(lhs);
	if (!a.concat(num)) a.invalidate();
	return a;
}

StringSumHelper & operator + (const StringSumHelper &lhs, double num)
{
	StringSumHelper &a = const_cast<StringSumHelper&>(lhs);
	if (!a.concat(num)) a.invalidate();
	return a;
}

StringSumHelper & operator + (const StringSumHelper &lhs, const __FlashStringHelper *rhs)
{
	StringSumHelper &a = const_cast<StringSumHelper&>(lhs);
	if (!a.concat(rhs))	a.invalidate();
	return a;
}

/*********************************************/
/*  Comparison                               */
/*********************************************/

int String::compareTo(const String &s) const
{
	return buffer.compare(s.buffer);
}

int String::compareTo(const char *cstr) const
{
	return buffer.compare(cstr ? cstr : "");
}

int String::compareTo(const std::string& s) const
{
	return buffer.compare(s);
}

bool String::equals(const String &s2) const
{
	return compareTo(s2) == 0;
}

bool String::equals(const char *cstr) const
{
	return compareTo(cstr) == 0;
}

bool String::equals(const std::string& s2) const
{
	return compareTo(s2) == 0;
}

bool String::equalsIgnoreCase( const String &s2 ) const
{
	return equalsIgnoreCase(s2.buffer);
}

bool String::startsWith( const String &s2 ) const
{
	return startsWith(s2.buffer);
}

bool String::startsWith( const String &s2, unsigned int offset ) const
{
	return startsWith(s2.buffer, offset);
}

bool String::endsWith( const String &s2 ) const
{
	return endsWith(s2.buffer);
}

bool String::equalsIgnoreCase(const std::string& s2) const
{
	if (length() != s2.length()) return false;
	auto it1 = buffer.begin();
	auto it2 = s2.begin();
	// lengths must be same to reach here, so we can just check for end of it1
	while (it1 != buffer.end()) {  
		if(tolower(*it1++)!= tolower(*it2++)) return false;
	}
	return true;
	// std::equal requires C++14
    // std::equal(buffer.begin(), buffer.end(), s2.begin(), s2.end(),
	// 		[](char a, char b) { return tolower(a) == tolower(b); }
	// );
}

bool String::startsWith(const std::string& s2) const
{
	return 0 == buffer.compare(0, s2.length(), s2);
}

bool String::startsWith(const std::string& s2, unsigned int offset) const
{
	return 0 == buffer.compare(offset, s2.length(), s2);
}

bool String::endsWith(const std::string& s2) const
{
	size_t len = length(), slen = s2.length();
	if (len < slen || buffer.empty() || s2.empty()) return false;
	return 0 == buffer.compare(length() - slen, slen, s2);
}


/*********************************************/
/*  Character Access                         */
/*********************************************/

char String::charAt(unsigned int loc) const
{
	return operator[](loc);
}

void String::setCharAt(unsigned int loc, char c) 
{
	if (loc < length()) buffer[loc] = c;
}

char & String::operator[](unsigned int index)
{
	static char dummy_writable_char;
	if (index >= length()) {
		dummy_writable_char = 0;
		return dummy_writable_char;
	}
	return buffer[index];
}

char String::operator[]( unsigned int index ) const
{
	return index >= length() ? 0 : buffer[index];
}

void String::getBytes(unsigned char *buf, unsigned int bufsize, unsigned int index) const
{
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

int String::indexOf(char c) const
{
	return indexOf(c, 0);
}

int String::indexOf( char ch, unsigned int fromIndex ) const
{
	return posToIndex(buffer.find(ch, fromIndex));
}

int String::indexOf(const String &s2) const
{
	return indexOf(s2.buffer, 0);
}

int String::indexOf(const String &s2, unsigned int fromIndex) const
{
	return indexOf(s2.buffer, fromIndex);
}

int String::indexOf(const std::string& s2) const
{
	return indexOf(s2, 0);
}

int String::indexOf(const std::string& s2, unsigned int fromIndex) const
{
	return posToIndex(buffer.find(s2, fromIndex));
}


int String::lastIndexOf( char theChar ) const
{
	return lastIndexOf(theChar, length() - 1);
}

int String::lastIndexOf(char ch, unsigned int fromIndex) const
{
	if (fromIndex >= length()) return -1;
	return posToIndex(buffer.rfind(ch, indexToPos(fromIndex)));
}

int String::lastIndexOf(const String &s2) const
{
	return lastIndexOf(s2, length() - s2.length());
}

int String::lastIndexOf(const String &s2, unsigned int fromIndex) const
{
	return lastIndexOf(s2.buffer, fromIndex);
}

int String::lastIndexOf(const std::string& s2) const
{
	return lastIndexOf(s2, static_cast<unsigned int>(length() - s2.length()));
}

int String::lastIndexOf(const std::string& s2, unsigned int fromIndex) const
{
	size_t len = length(), s2len = s2.length();
	if (s2len == 0 || len == 0 || s2len > len) return -1;
	if (fromIndex >= len) fromIndex = static_cast<unsigned int>(len - 1);
	return posToIndex(buffer.rfind(s2, indexToPos(fromIndex)));
}

String String::substring(unsigned int left, unsigned int right) const
{
	if (left > right) std::swap(left, right);
	size_t len = length();
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

void String::replace(char find, char replace)
{
	size_t pos = 0;
	while ((pos = buffer.find(find, pos)) != std::string::npos) {
		buffer[pos++] = replace;
	}
}

void String::replace(const String& find, const String& replace)
{
	size_t len = find.length();
	size_t pos = 0;
	while ((pos = buffer.find(find.buffer, pos)) != std::string::npos) {
		buffer.replace(pos, len, replace.buffer);
	}
}

void String::remove(unsigned int index)
{
	// Pass the biggest integer as the count. The remove method
	// below will take care of truncating it at the end of the
	// string.
	remove(index, std::numeric_limits<unsigned int>::max());
}

void String::remove(unsigned int index, unsigned int count)
{
	if (index < buffer.length())
		buffer.erase(index, count);
}

void String::toLowerCase(void)
{
	std::transform(buffer.begin(), buffer.end(), buffer.begin(), [](char c) -> char {return std::tolower(c); });
}

void String::toUpperCase(void)
{
	std::transform(buffer.begin(), buffer.end(), buffer.begin(), [](char c) -> char {return std::toupper(c); });
}

void String::trim(void)
{
	auto left = std::find_if_not(buffer.begin(), buffer.end(), [](char c) {return std::isspace(c); });
	if (left != buffer.begin()) {
		buffer.erase(buffer.begin(), left);
	}
	auto right = std::find_if_not(buffer.rbegin(), buffer.rend(), [](char c) {return std::isspace(c); });
	if (right != buffer.rbegin()) {
		buffer.erase(right.base(), buffer.end());
	}
}

/*********************************************/
/*  Parsing / Conversion                     */
/*********************************************/

long String::toInt(void) const
{
	return ::atol(c_str());
}

float String::toFloat(void) const
{
	return static_cast<float>(::atof(c_str()));
}

double String::toDouble(void) const
{
	return ::atof(c_str());
}
} // namespace arduino


#endif // __ARDUINO_STRING_HELPERS__