/**
 * @file ard_PrintMockHelp.h
 * @author Jeffrey Kuhn (jrkuhn@mit.edu)
 * @brief Helpers for Print mock class on host
 * @version 0.1
 * @date 2022-04-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#ifndef __ARD_PRINTMOCKHELP_H__
#define __ARD_PRINTMOCKHELP_H__

#ifdef ARDUINO
#error Use Print and String classes from Arduino.h instead
#endif

#include "../StringT.h"
#include <type_traits>
#include <algorithm>
#include <cmath>

namespace rdl {

	/**
	 * Append the value of a generic **unsigned** integer to a generic string with a generic base.
	 *
	 * Output should be the similar to the Arduino's Print.printNumber() function.
	 */
	template<typename numT, typename std::enable_if<std::is_integral<numT>::value&& std::is_unsigned<numT>::value, bool>::type = true>
	rdl::StringT& appendNumber(rdl::StringT& dest, numT number, int base, bool reversed = false)
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
	rdl::StringT& appendNumber(rdl::StringT& dest, numT number, int base, bool reversed = false)
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
	rdl::StringT& appendNumber(rdl::StringT& dest, numT number, int decimalPlaces, bool dummy = false)
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
	 * Convert generic integers (signed or unsigned) or floating point to a rdl::StringT.
	 * Integers take a base as the second parameter (HEX, DEC, etc). Floating points
	 * take a decimal places as the second parameter (number of digits after the decimal).
	 *
	 * Output should be the similar to the Arduino's Print.printNumber() function.
	 */
	template<typename numT, typename std::enable_if<std::is_integral<numT>::value || std::is_floating_point<numT>::value, bool>::type = true>
	rdl::StringT to_string(numT number, int baseOrPlaces)
	{
		rdl::StringT res;
		return appendNumber(res, number, baseOrPlaces);
	}

} // namespace arduino


#endif // __ARD_PRINTMOCKHELP_H__