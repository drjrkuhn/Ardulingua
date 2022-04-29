#pragma once

#include <cstdint>
#include <streambuf>
#include <stdio.h>
#include <string>
#include <limits>
#include <ios>
#include <sstream>
#include <type_traits>
#include <mutex>
#include "Print.h"

namespace arduino {

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


	/**
	 * Adapts a OSTREAM for printing.
	 *
	 * This version overrides both the single-byte 'write' and the
	 * multi-byte 'write' functions of Print to write to the OSTREAM.
	 */
	template <class OSTREAM>
	class Print_stdostream : public Print {
	public:
		Print_stdostream(OSTREAM& os) : _ostream(os) {}
		OSTREAM& ostream() { return _ostream; }


		virtual size_t write(const uint8_t c) override {
			std::lock_guard<std::mutex> _(_guard); 
			char cc = static_cast<char>(c);
			return _ostream.rdbuf()->sputc(cc) == cc ? 1 : 0;
		}

		virtual size_t write(const uint8_t* str, size_t n) override {
			std::lock_guard<std::mutex> _(_guard); 
			return _ostream.rdbuf()->sputn(reinterpret_cast<const char*>(str), n);
		}

		virtual int availableForWrite() override {
			std::lock_guard<std::mutex> _(_guard); 
			return std::numeric_limits<int>::max();
		}

		virtual void flush() override { 
			std::lock_guard<std::mutex> _(_guard); 
			return Print::flush();
		}

		template<typename T>
		OSTREAM& operator<< (T t) { return ostream() << t; }
	protected:
		OSTREAM& _ostream;
		mutable std::mutex _guard;
	};

	/**
	 * Adapts a std::ostringstream for printing to strings.
	 *
	 * This version keeps an internal stringstream/stringbuf
	 * as its stream. Access the current contents with @see str().
	 * Clear the buffer with @see clear().
	 */
	class Print_stdstring : public Print_stdostream<std::ostringstream>, public Printable {
	public:
		Print_stdstring(const std::string str)
			: Print_stdostream(_oss), _oss(str, std::ios_base::out | std::ios_base::app) {
			// NOTE: open in append mode so we don't overwrite the intiial value
		}
		Print_stdstring()
			: Print_stdostream(_oss), _oss(std::ios_base::out | std::ios_base::app) {
		}

		std::string str() const { 
			std::lock_guard<std::mutex> _(_guard); 
			return _oss.str(); 
		}
		void str(const std::string s) { 
			std::lock_guard<std::mutex> _(_guard); 
			_oss.str(s); 
		}
		void clear() { 
			std::lock_guard<std::mutex> _(_guard); 
			_oss.str(""); 
		}

		/// number of bytes available in write buffer. 
		/// For stringstream it will return 0 just before reallocating more buffer space
		virtual int availableForWrite() override {
			std::lock_guard<std::mutex> _(_guard); 
			return static_cast<int>(_oss.str().capacity() - _oss.str().length());
		}
		virtual size_t printTo(Print& p) const override {
			std::lock_guard<std::mutex> _(_guard); 
			return p.write(_oss.str());
		}
	protected:
		std::ostringstream _oss;
	};


} // namespace arduino