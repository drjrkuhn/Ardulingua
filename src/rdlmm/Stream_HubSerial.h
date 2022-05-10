#pragma once

#ifndef __STREAM_HUBSERIAL_H__
    #define __STREAM_HUBSERIAL_H__

    #include "../rdl/sys_StreamT.h"
    #include "../rdl/sys_StringT.h"
    #include "../rdl/sys_timing.h"
    #include <cstddef> // for size_t
    #include <deque>
    #include <mutex>

namespace rdlmm {

    namespace svc {
        /** Our read deque buffers a single character so we can `peek` at the first character. */
        constexpr size_t DEQUE_SIZE = 1;
    }

    /**
     * Adapter for an MM device hub to make it look like an arduino-compatible
     * Serial port stream.
     *
     * In other words, this class emulates an Arduino Serial or other read/write Stream
     * using serial-port read/write methods available to an MM HubBase device.
     * 
     * > NOTE: the hub DeviceT MUST have a public `port()` method that returns the
     * > current serial port name.
     *
     * This adapter also exposes some of the Hub's protected methods to make
     * generic DeviceT accessing a little easier. @see protected accessor struct
     * for the mechanism.
     * 
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
     *    - private/protected methods don't call public methods
     *
     * @tparam DeviceT  MM CDeviceHub type. MUST HAVE `port()` method that 
     *                  returns a string with the current serial port name
     */
    template <class DeviceT>
    class Stream_HubSerial : public sys::StreamT {
     public:
        Stream_HubSerial(DeviceT* hub) : hub_(hub) {}

        /** Get the current serial port name from the Hub device */
        sys::StringT port() const {
            std::lock_guard<std::mutex> _(guard_);
            return port_impl();
        }

        virtual size_t write(const uint8_t byte) override {
            std::lock_guard<std::mutex> _(guard_);
            return write_impl(&byte, 1);
        }

        virtual size_t write(const uint8_t* str, size_t n) override {
            std::lock_guard<std::mutex> _(guard_);
            return write_impl(str, n);
        }

        int availableForWrite() override {
            std::lock_guard<std::mutex> _(guard_);
            // boost::AsioClient doesn't have a write limit
            return std::numeric_limits<int>::max();
        }

        virtual int available() override {
            std::lock_guard<std::mutex> _(guard_);
            getNextChar();
            return static_cast<int>(rdbuf_.size());
        }

        virtual int read() override {
            std::lock_guard<std::mutex> _(guard_);
            return read_impl();
        }

        virtual int peek() override {
            std::lock_guard<std::mutex> _(guard_);
            getNextChar();
            return rdbuf_.empty() ? -1 : rdbuf_.front();
        }

        virtual void clear() {
            std::lock_guard<std::mutex> _(guard_);
            accessor::PurgeComPort(hub_, port_impl().c_str());
        }

        size_t readBytes(char* buffer, size_t length) {
            std::lock_guard<std::mutex> _(guard_);
            return readBytes_impl(buffer, length);
        }

        sys::StringT readStdStringUntil(char terminator) {
            std::lock_guard<std::mutex> _(guard_);
            return readStdStringUntil_impl(terminator);
        }

        size_t readBytesUntil(char terminator, char* buffer, size_t length) {
            std::lock_guard<std::mutex> _(guard_);
            return readBytesUntil_impl(terminator, buffer, length);
        }

        size_t readBytesUntil(char terminator, uint8_t* buffer, size_t length) {
            std::lock_guard<std::mutex> _(guard_);
            return readBytesUntil_impl(terminator, reinterpret_cast<char*>(buffer), length);
        }

        /** Expose the hub's LogMessage method */
        int LogMessage(sys::StringT msg, bool debug_only = false) {
            std::lock_guard<std::mutex> _(guard_);
            return accessor::LogMessage(hub_, msg, debug_only);
        }

        /** Expose the hub's LogMessage method */
        int LogMessage(const char* msg, bool debug_only = false) {
            std::lock_guard<std::mutex> _(guard_);
            return accessor::LogMessage(hub_, msg, debug_only);
        }
        /** Expose the hub's LogMessageCode method */
        int LogMessageCode(const int errorCode, bool debug_only = false) const {
            std::lock_guard<std::mutex> _(guard_);
            return accessor::LogMessageCode(hub_, errorCode, debug_only);
        }


        /** Expose the hub's GetErrorText method */
        bool GetErrorText(int errorCode, char* text) const {
            std::lock_guard<std::mutex> _(guard_);
            return accessor::GetErrorText(hub_, errorCode, text);
        }

        /** Expose the hub's GetCoreCallback method */
        MM::Core* GetCoreCallback() const {
            std::lock_guard<std::mutex> _(guard_);
            return accessor::GetCoreCallback(hub_);
        }

     protected:
        // accessor for protected CDeviceBase methods
        struct accessor : DeviceT {
            static int PurgeComPort(DeviceT* target, const char* portLabel) {
                int (DeviceT::*fn)(const char*) = &DeviceT::PurgeComPort;
                return (target->*fn)(portLabel);
            }
            static int WriteToComPort(DeviceT* target, const char* portLabel, const unsigned char* buf, unsigned bufLength) {
                int (DeviceT::*fn)(const char*, const unsigned char*, unsigned) = &DeviceT::WriteToComPort;
                return (target->*fn)(portLabel, buf, bufLength);
            }
            static int ReadFromComPort(DeviceT* target, const char* portLabel, unsigned char* buf, unsigned bufLength, unsigned long& read) {
                int (DeviceT::*fn)(const char*, unsigned char*, unsigned, unsigned long&) = &DeviceT::ReadFromComPort;
                return (target->*fn)(portLabel, buf, bufLength, read);
            }
            static int GetSerialAnswer(DeviceT* target, const char* portName, const char* term, std::string& ans) {
                int (DeviceT::*fn)(const char*, const char*, std::string&) = &DeviceT::GetSerialAnswer;
                return (target->*fn)(portName, term, ans);
            }
            static int LogMessage(DeviceT* target, sys::StringT msg, bool debug_only = false) {
                int (DeviceT::*fn)(const sys::StringT&, bool) const = &DeviceT::LogMessage;
                return (target->*fn)(msg, debug_only);
            }
            static int LogMessage(DeviceT* target, const char* msg, bool debug_only = false) {
                int (DeviceT::*fn)(const char*, bool) const = &DeviceT::LogMessage;
                return (target->*fn)(msg, debug_only);
            }
            static int LogMessageCode(DeviceT* target, const int errorCode, bool debugOnly = false) {
                int (DeviceT::*fn)(const int, bool) const = &DeviceT::LogMessageCode;
                return (target->*fn)(errorCode, debugOnly);
            }

            static bool GetErrorText(DeviceT* target, int errorCode, char* text) {
                bool (DeviceT::*fn)(int, char*) const = &DeviceT::GetErrorText;
                return (target->*fn)(errorCode, text);
            }
            static MM::Core* GetCoreCallback(DeviceT* target) {
                MM::Core* (DeviceT::*fn)() const = &DeviceT::GetCoreCallback;
                return (target->*fn)();
            }

        };

        sys::StringT port_impl() const {
            return hub_->port();
        }

        size_t write_impl(const uint8_t* str, size_t n) {
            int err = accessor::WriteToComPort(hub_, port_impl().c_str(), str, static_cast<unsigned int>(n));
            return (err == DEVICE_OK) ? n : 0;
        }

        int read_impl() {
            getNextChar();
            if (rdbuf_.empty()) return -1;
            int front = rdbuf_.front();
            rdbuf_.pop_front();
            return front;
        }

        // readBytes_impl(buffer,length) calls
        //    int err =
        //    DeviceBase::ReadFromComPort(portLabel,buf,bufLength,[OUT]bytesRead);
        //        which calls
        //        int err =
        //        CallBack::ReadFromSerial(this,portLabel,buf,bufLength,[OUT]bytesRead);
        //            which calls
        //            int err =
        //            SerialInstance::Read(buf, bufLength, [OUT] bytesRead);
        //                which skips MMCore and just directly calls
        //                int err =
        //                GetImpl()->Read(buf, bufLen, [OUT]charsRead);
        //
        // So this is a somewhat direct read operation compred to GetSerialAnswer
        size_t readBytes_impl(char* buffer, size_t length) {
            std::lock_guard<std::mutex> _(guard_);

            int lastc = -1;
            if (!rdbuf_.empty() && (lastc = read_impl()) >= 0) {
                *(buffer++) = (char)lastc;
                length--;
            }
            unsigned long bytesRead;
            int err = accessor::ReadFromComPort(hub_, port_impl().c_str(), reinterpret_cast<unsigned char*>(buffer), static_cast<unsigned int>(length), bytesRead);
            if (err == DEVICE_OK) {
                // GetSerialAnswer discards the terminator
                if (lastc >= 0) bytesRead++;
                return bytesRead;
            } else {
                accessor::LogMessage(hub_, "HubStreamAdapter::readBytes(buffer,length) failed: ");
                char text[MM::MaxStrLength];
                accessor::GetErrorText(hub_, err, text);
                accessor::LogMessage(hub_, text);
                if (lastc >= 0) bytesRead++;
                return bytesRead;
            }
        }

        // Oh, the tagled web we weave to search for messages with terminators.
        // readStringUntil calls
        //    int err =
        //    DeviceBase::GetSerialAnswer(portName,term,[OUT]answerString);
        //        which creates an internal `char bufferA[2000]` and then calls
        //        int err =
        //        CoreCallback::GetSerialAnswer(this,portName,2000,bufferA,term);
        //            which calls
        //            sys::StringT answer =
        //            MMCore::getSerialPortAnswer(portName,term);
        //                which checks that the term is not NULL and doesn't start
        //                with '\0' (NO NULL TERMINATORS) then creates another
        //                internal `char bufferB[1024]` and calls
        //                int ret =
        //                SerialInstance::GetAnswer(bufferB,1024,term);
        //                    which calls
        //                    GetImpl()->GetAnswer(bufferB, 1024, term);
        //            MMCore then creates a sys::StringT from the NULL term bufferB
        //            (so NO zeros in the middle of the buffer) and passes that
        //            back to CoreCallback
        //        CoreCallback does a strcpy of the answer into bufferA
        //        and passed that bufferA back to
        //    DeviceBase, which then copies it's own bufferA back into the
        //    `answerString` string and returns that to
        // readStringUntil
        //
        // So, `readStringUntil`, creates two additional buffers and one intermediate
        // string (with its own buffer)
        sys::StringT readStdStringUntil_impl(char terminator) {
            sys::StringT compose;
            // deal with any character already in the read buffer
            int lastc;
            if (!rdbuf_.empty() && (lastc = read_impl()) >= 0) {
                compose.append(1, (char)lastc);
                if (lastc == terminator) return compose;
            }
            sys::StringT answerString;
            const char termstr[]{terminator, '\0'};
            int err = accessor::GetSerialAnswer(hub_, port_impl().c_str(), termstr, answerString);
            if (err == DEVICE_OK) {
                // GetSerialAnswer discards the terminator
                compose.append(answerString);
                compose.append(1, terminator);
            } else {
                accessor::LogMessage(hub_, "HubStreamAdapter::readStdStringUntil(terminator) failed: ");
                char text[MM::MaxStrLength];
                accessor::GetErrorText(hub_, err, text);
                accessor::LogMessage(hub_, text);
            }
            return compose;
        }

        size_t readBytesUntil_impl(char terminator, char* buffer, size_t length) {
            sys::StringT answer = readStdStringUntil_impl(terminator);
            size_t ngood        = std::min(answer.length(), length);
            std::strncpy(buffer, answer.c_str(), ngood);
            if (answer.length() > length) {
                accessor::LogMessage(hub_, "HubStreamAdapter::readBytesUntil(terminator,buffer,length) failed: ");
                char text[MM::MaxStrLength];
                accessor::GetErrorText(hub_, DEVICE_BUFFER_OVERFLOW, text);
                accessor::LogMessage(hub_, text);
            }
            return ngood;
        }

        /** buffer the next character because MMCore doesn't have a peek 
         * function for serial */
        void getNextChar() {
            if (rdbuf_.empty()) {
                unsigned long timeout = sys::millis() + getTimeout();
                do {
                    unsigned char buf;
                    unsigned long read;
                    int err = accessor::ReadFromComPort(hub_, port_impl().c_str(), &buf, 1, read);
                    if (err == DEVICE_OK && read > 0) {
                        rdbuf_.push_back(buf);
                        return;
                    }
                } while (sys::millis() < timeout);
            }
        }

        std::deque<unsigned char> rdbuf_;
        DeviceT* hub_;
        mutable std::mutex guard_;
    };

} // namespace rdl

#endif // __STREAM_HUBSERIAL_H__