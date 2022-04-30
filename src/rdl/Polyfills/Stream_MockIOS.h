#pragma once

#ifndef __STREAM_MOCKIOS_H__
    #define __STREAM_MOCKIOS_H__

    #include "../sys_StringT.h"
    #include "Stream_Mock.h"
    #include <iomanip>
    #include <ios>
    #include <mutex>
    #include <ostream>
    #include <sstream>

    #ifndef STRINGSTREAM_KEEP_SMALL
        #define STRINGSTREAM_KEEP_SMALL 1
    #endif

namespace std {

    /*
     * ## Rules-of-thumb for mutex locking
     *
     * If a method with a std::lock_guard calls another method with its own
     * std::lock_guard, this will create a nested mutex halt condition. To keep
     * from nested locks, we stick to a few set of rules:
     *
     *    - only public methods have lock guards
     *    - private/protected methods don't have lock guards
     *    - public methods don't call each other (no lock overlap)
     *    - public methods call private/protected unlocked _impl methods
     */

    template <class IOSTREAM>
    class Stream_iostream : public sys::Stream {
     public:
        Stream_iostream(IOSTREAM& ios) : _ios(ios) {
            std::lock_guard<std::mutex> _(_guard);
            init();
        }
        IOSTREAM& ios() {
            std::lock_guard<std::mutex> _(_guard);
            return _ios;
        }

        virtual size_t write(const uint8_t byte) override {
            std::lock_guard<std::mutex> _(_guard);
            char cc = static_cast<char>(byte);
            return _canput && _ios.rdbuf()->sputc(cc) == cc ? 1 : 0;
        }
        virtual size_t write(const uint8_t* str, size_t n) override {
            std::lock_guard<std::mutex> _(_guard);
            return _canput ? _ios.rdbuf()->sputn(reinterpret_cast<const char*>(str), n) : 0;
        }
        virtual int availableForWrite() override {
            std::lock_guard<std::mutex> _(_guard);
            return _canput ? std::numeric_limits<int>::max() : 0;
        }

        /**
         * How many characters are available for reading.
         *
         * **MECHANISM NOTES**
         * std::basic_streambuf::in_avail() returns an _estimate_
         * of the number of characters. In many implementations, in_avail()
         * does not call the protected underflow() to update the
         * input stream pointers. Calling sgetc() forces underflow()
         * and allows in_avail() to report at least one character
         * if the input stream has data.
         */
        virtual int available() override {
            std::lock_guard<std::mutex> _(_guard);
            if (!_canget)
                return 0;
            // force update of input buffer pointers
            _ios.rdbuf()->sgetc();
            return static_cast<int>(_ios.rdbuf()->in_avail());
        }

        virtual int read() override {
            std::lock_guard<std::mutex> _(_guard);
            int ret = static_cast<int>(_canget ? _ios.rdbuf()->sbumpc() : -1);
            update_buf();
            return ret;
        }
        virtual int peek() override {
            std::lock_guard<std::mutex> _(_guard);
            int ret = static_cast<int>(_canget ? _ios.rdbuf()->sgetc() : -1);
            update_buf();
            return ret;
        }
        virtual size_t readBytes(char* buffer, size_t length) override {
            std::lock_guard<std::mutex> _(_guard);
            size_t ret = _canget ? _ios.rdbuf()->sgetn(buffer, length) : 0;
            update_buf();
            return ret;
        }

     protected:
        virtual void update_buf() {}

        void init() {
            _canget = _ios.tellg() >= 0;
            _canput = _ios.tellp() >= 0;
        }

        // protected default constructor for derived
        struct no_init_tag {};
        Stream_iostream(IOSTREAM& ios, no_init_tag) : _ios(ios) {}

        IOSTREAM& _ios;
        bool _canget, _canput;
        mutable std::mutex _guard;
    };

    class Stream_StringT : public Stream_iostream<std::stringstream> {
     public:
        Stream_StringT(
            const sys::StringT str, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out | std::ios_base::app)
            : Stream_iostream(_ss, (no_init_tag())), _ss(str, which) {
            init();
            // NOTE: open in append mode so we don't overwrite the intiial value
        }
        Stream_StringT(std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
            : Stream_iostream(_ss, (no_init_tag())), _ss(which) {
            init();
            // NOTE: open in append mode so we don't overwrite the intiial value
        }
        sys::StringT str() const {
            std::lock_guard<std::mutex> _(_guard);
            return _ss.str();
        }
        void str(const sys::StringT s) {
            std::lock_guard<std::mutex> _(_guard);
            _ss.str(s);
        }
        void clear() {
            std::lock_guard<std::mutex> _(_guard);
            _ss.str("");
            std::swap(_ios, _ss);
        }

        /** Diagnostic version of str. Shows g and p pointers. */
        sys::StringT buffer_str() const {
            std::lock_guard<std::mutex> _(_guard);
            sys::StringT buf = _ss.str();
            // convert to signed positions
            long len = static_cast<long>(buf.length());
            long g   = static_cast<long>(_ios.tellg());
            long p   = static_cast<long>(_ios.tellp());
            if (p < 0)
                p = len;
            bool samegp = g == p;
            if (g < 0)
                g = 0;
            long headlen = g;
            long taillen = len - p;
            sys::StringT ptrs;
            if (headlen - 1 > 0)
                ptrs.append((headlen - 1), '.');
            ptrs.append(1, samegp ? '@' : '^');
            ptrs.append(buf.substr(g, p - g));
            if (!samegp)
                ptrs.append(1, 'v');
            return ptrs;
        }

     protected:

    #if STRINGSTREAM_KEEP_SMALL 1
        /** Check put and get pointers. If they are equal, clear the string for more IO. */
        virtual void update_buf() override {
            // force update of pointers
            _ios.rdbuf()->sgetc();
            // fetch current pointer positions            
            std::streampos g = _ios.tellg(), p = _ios.tellp();
            if (p < 0)
                p = _ss.str().length();
            if (g > 0 && g == p) {
                // clear _ss by swapping with a new one
                std::stringstream temp;
                temp.copyfmt(_ss);
                _ss.swap(temp);
                _ss.clear();
                _ss.copyfmt(temp);
                std::swap(_ios, _ss);
            }
        }
    #endif

        std::stringstream _ss;
    };

} // namespace arduino

#endif // __STREAM_MOCKIOS_H__