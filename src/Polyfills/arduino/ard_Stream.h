/*
  Stream.h - base class for character-based streams.
  Copyright (c) 2010 David A. Mellis.  All right reserved.

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

  parsing functions based on TextFinder library by Michael Margolis
*/

#pragma once

#include "Common.h"
#include "Print.h"
#include <inttypes.h>

namespace arduino {

    const unsigned long PARSE_TIMEOUT = 1000; // default number of milli-seconds to wait

    // This enumeration provides the lookahead options for parseInt(), parseFloat()
    // The rules set out here are used until either the first valid character is found
    // or a time out occurs due to lack of input.
    enum LookaheadMode {
        SKIP_ALL,       // All invalid characters are ignored.
        SKIP_NONE,      // Nothing is skipped, and the stream is not touched unless the first waiting character is valid.
        SKIP_WHITESPACE // Only tabs, spaces, line feeds & carriage returns are skipped.
    };

#define NO_IGNORE_CHAR '\x01' // a char not found in a valid ASCII numeric field

    class Stream : public Print {
     protected:
        unsigned long _timeout;     // number of milliseconds to wait for the next char before aborting timed read
        unsigned long _startMillis; // used for timeout measurement
                                    // private method to read stream with timeout
        virtual int timedRead() {
            int c;
            _startMillis = millis();
            do {
                c = read();
                if (c >= 0) return c;
                yield();
                delay(1);
            } while (millis() - _startMillis < _timeout);
            return -1; // -1 indicates timeout
        }              
        
        // private method to peek stream with timeout
        virtual int timedPeek() {
            int c;
            _startMillis = millis();
            do {
                c = peek();
                if (c >= 0) return c;
                yield();
                delay(1);
            } while (millis() - _startMillis < _timeout);
            return -1; // -1 indicates timeout
        }              // private method to peek stream with timeout
                       // returns peek of the next digit in the stream or -1 if timeout
        // discards non-numeric characters
        int peekNextDigit(LookaheadMode lookahead, bool detectDecimal) {
            int c;
            while (1) {
                c = timedPeek();
                if (c < 0 || c == '-' || (c >= '0' && c <= '9') || (detectDecimal && c == '.'))
                    return c;

                switch (lookahead) {
                    case SKIP_NONE:
                        return -1; // Fail code.
                    case SKIP_WHITESPACE:
                        switch (c) {
                            case ' ':
                            case '\t':
                            case '\r':
                            case '\n':
                                break;
                            default:
                                return -1; // Fail code.
                        }
                    case SKIP_ALL:
                        break;
                }
                read(); // discard non-numeric
            }
        } // returns the next numeric digit in the stream or -1 if timeout

     public:
        Stream() { _timeout = 1000; }
        virtual ~Stream() = default;

        virtual int available() = 0;
        virtual int read()      = 0;
        virtual int peek()      = 0;

        // reads data from the stream until the target string of given length is found
        // returns true if target string is found, false if timed out
        virtual bool find(const char* target, size_t length) {
            return findUntil(target, length, NULL, 0);
        }
        virtual bool find(char target) { return find(&target, 1); }
        // reads data from the stream until the target string of the given length is found
        // search terminated if the terminator string is found
        // returns true if target string is found, false if terminated or timed out
        virtual bool findUntil(const char* target, size_t targetLen, const char* terminate, size_t termLen) {
            if (terminator == NULL) {
                MultiTarget t[1] = {{target, targetLen, 0}};
                return findMulti(t, 1) == 0;
            } else {
                MultiTarget t[2] = {{target, targetLen, 0}, {terminator, termLen, 0}};
                return findMulti(t, 2) == 0;
            }
        }
        // returns the first valid (long) integer value from the current position.
        // lookahead determines how parseInt looks ahead in the stream.
        // See LookaheadMode enumeration at the top of the file.
        // Lookahead is terminated by the first character that is not a valid part of an integer.
        // Once parsing commences, 'ignore' will be skipped in the stream.
        virtual long parseInt(LookaheadMode lookahead = SKIP_ALL, char ignore = NO_IGNORE_CHAR) {
            bool isNegative = false;
            long value      = 0;
            int c;

            c = peekNextDigit(lookahead, false);
            // ignore non numeric leading characters
            if (c < 0)
                return 0; // zero returned if timeout

            do {
                if ((char)c == ignore)
                    ; // ignore this character
                else if (c == '-')
                    isNegative = true;
                else if (c >= '0' && c <= '9') // is c a digit?
                    value = value * 10 + c - '0';
                read(); // consume the character we got with peek
                c = timedPeek();
            } while ((c >= '0' && c <= '9') || (char)c == ignore);

            if (isNegative)
                value = -value;
            return value;
        }
        // as parseInt but returns a floating point value
        virtual float parseFloat(LookaheadMode lookahead = SKIP_ALL, char ignore = NO_IGNORE_CHAR) {
            bool isNegative = false;
            bool isFraction = false;
            double value    = 0.0;
            int c;
            double fraction = 1.0;

            c = peekNextDigit(lookahead, true);
            // ignore non numeric leading characters
            if (c < 0)
                return 0; // zero returned if timeout

            do {
                if ((char)c == ignore)
                    ; // ignore
                else if (c == '-')
                    isNegative = true;
                else if (c == '.')
                    isFraction = true;
                else if (c >= '0' && c <= '9') { // is c a digit?
                    if (isFraction) {
                        fraction *= 0.1;
                        value = value + fraction * (c - '0');
                    } else {
                        value = value * 10 + c - '0';
                    }
                }
                read(); // consume the character we got with peek
                c = timedPeek();
            } while ((c >= '0' && c <= '9') || (c == '.' && !isFraction) || (char)c == ignore);

            if (isNegative)
                value = -value;

            return float(value);
        }
        // read characters from stream into buffer
        // terminates if length characters have been read, or timeout (see setTimeout)
        // returns the number of characters placed in the buffer
        // the buffer is NOT null terminated.
        //
        virtual size_t readBytes(char* buffer, size_t length) {
            size_t count = 0;
            while (count < length) {
                int c = timedRead();
                if (c < 0) break;
                *buffer++ = (char)c;
                count++;
            }
            return count;
        }
        // as readBytes with terminator character
        // terminates if length characters have been read, timeout, or if the terminator character  detected
        // returns the number of characters placed in the buffer (0 means no valid data found)
        virtual size_t readBytesUntil(char terminator, char* buffer, size_t length) {
            size_t index = 0;
            while (index < length) {
                int c = timedRead();
                if (c < 0 || (char)c == terminator) break;
                *buffer++ = (char)c;
                index++;
            }
            return index; // return number of characters, not including null terminator
        }
        virtual std::string readStdString() {
            std::string ret;
            int c = timedRead();
            while (c >= 0) {
                ret.append(1, (char)c);
                c = timedRead();
            }
            return ret;
        }
        virtual std::string readStdStringUntil(char terminator) {
            std::string ret;
            int c = timedRead();
            while (c >= 0 && (char)c != terminator) {
                ret.append(1, (char)c);
                c = timedRead();
            }
            return ret;
        }

        // parsing methods

        void setTimeout(unsigned long timeout) // sets the maximum number of milliseconds to wait
        {
            _timeout = timeout;
        } // sets maximum milliseconds to wait for stream data, default is 1 second
        unsigned long getTimeout(void) { return _timeout; }

        // find returns true if the target string is found
        bool find(const char* target) {
            return findUntil(target, strlen(target), NULL, 0);
        } // reads data from the stream until the target string is found
        bool find(const uint8_t* target) { return find((const char*)target); }
        // returns true if target string is found, false if timed out (see setTimeout)

        bool find(const uint8_t* target, size_t length) { return find((const char*)target, length); }
        // returns true if target string is found, false if timed out
        bool find(const std::string target) { return find(target.c_str(), target.length()); }

        // as find but search ends if the terminator string is found
        bool findUntil(const char* target, const char* terminator) {
            return findUntil(target, strlen(target), terminator, strlen(terminator));
        } // as find but search ends if the terminator string is found
        bool findUntil(const uint8_t* target, const char* terminator) { return findUntil((const char*)target, terminator); }

        bool findUntil(const uint8_t* target, size_t targetLen, const char* terminate, size_t termLen) { return findUntil((const char*)target, targetLen, terminate, termLen); }

        size_t readBytes(uint8_t* buffer, size_t length) { return readBytes((char*)buffer, length); }
        // terminates if length characters have been read or timeout (see setTimeout)
        // returns the number of characters placed in the buffer (0 means no valid data found)

        size_t readBytesUntil(char terminator, uint8_t* buffer, size_t length) { return readBytesUntil(terminator, (char*)buffer, length); }
        // terminates if length characters have been read, timeout, or if the terminator character  detected
        // returns the number of characters placed in the buffer (0 means no valid data found)

        template <typename CharIT, typename std::enable_if<is_char_iterator<CharIT>::value, bool>::type = false>
        size_t readBytes_to(CharIT begin, CharIT end) {
            return readBytes(&begin[0], end - begin);
        }

        // Arduino String functions to be added here
        String readString() { return String(readStdString()); }
        String readStringUntil(char terminator) { return String(readStdStringUntil(terminator)); }

     protected:
        long parseInt(char ignore) { return parseInt(SKIP_ALL, ignore); }
        float parseFloat(char ignore) { return parseFloat(SKIP_ALL, ignore); }
        // These overload exists for compatibility with any class that has derived
        // Stream and used parseFloat/Int with a custom ignore character. To keep
        // the public API simple, these overload remains protected.

        struct MultiTarget {
            const char* str; // string you're searching for
            size_t len;      // length of string you're searching for
            size_t index;    // index used by the search routine.
        };
        // This allows you to search for an arbitrary number of strings.
        // Returns index of the target that is found first or -1 if timeout occurs.
        virtual int findMulti(struct MultiTarget* targets, int tCount) {
            // any zero length target string automatically matches and would make
            // a mess of the rest of the algorithm.
            for (struct MultiTarget* t = targets; t < targets + tCount; ++t) {
                if (t->len <= 0)
                    return static_cast<int>(t - targets);
            }

            while (1) {
                int c = timedRead();
                if (c < 0)
                    return -1;

                for (struct MultiTarget* t = targets; t < targets + tCount; ++t) {
                    // the simple case is if we match, deal with that first.
                    if ((char)c == t->str[t->index]) {
                        if (++t->index == t->len)
                            return static_cast<int>(t - targets);
                        else
                            continue;
                    }

                    // if not we need to walk back and see if we could have matched further
                    // down the stream (ie '1112' doesn't match the first position in '11112'
                    // but it will match the second position so we can't just reset the current
                    // index to 0 when we find a mismatch.
                    if (t->index == 0)
                        continue;

                    size_t origIndex = t->index;
                    do {
                        --t->index;
                        // first check if current char works against the new current index
                        if ((char)c != t->str[t->index])
                            continue;

                        // if it's the only char then we're good, nothing more to check
                        if (t->index == 0) {
                            t->index++;
                            break;
                        }

                        // otherwise we need to check the rest of the found string
                        size_t diff = origIndex - t->index;
                        size_t i;
                        for (i = 0; i < t->index; ++i) {
                            if (t->str[i] != t->str[i + diff])
                                break;
                        }

                        // if we successfully got through the previous loop then our current
                        // index is good.
                        if (i == t->index) {
                            t->index++;
                            break;
                        }

                        // otherwise we just try the next index
                    } while (t->index);
                }
            }
            // unreachable
            return -1;
        }
    };

    // Public Methods
    //////////////////////////////////////////////////////////////

#undef NO_IGNORE_CHAR

}

using arduino::Stream;