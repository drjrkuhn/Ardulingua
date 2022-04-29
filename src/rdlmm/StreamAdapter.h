#pragma once

#ifndef __STREAM_ADAPTER_H__
    #define __STREAM_ADAPTER_H__

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
     * > NOTE: Including the Arduino StreamT as a template parameters means we don't have
     * > to include our emulated "ArduinoCore-host" api here.
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
     *
     * @tparam HubT         MM HubDevice
     * @tparam StreamT      Arduino Stream base class to derive from.
     */
    template <class HubT, class StreamT>
    class HubStreamAdapter : public StreamT {
     public:
        HubStreamAdapter(HubT* hub) : hub_(hub) {}

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
            hub_->PurgeComPort(hub_->port().c_str());
        }

        size_t readBytes(char* buffer, size_t length) {
            std::lock_guard<std::mutex> _(guard_);
            return readBytes_impl(buffer, length);
        }

        std::string readStdStringUntil(char terminator) {
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

     protected:
        size_t write_impl(const uint8_t* str, size_t n) {
            int err = hub_->WriteToComPort(hub_->port().c_str(), str, static_cast<unsigned int>(n));
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
            int err = hub_->ReadFromComPort(hub_->port().c_str(), reinterpret_cast<unsigned char*>(buffer), static_cast<unsigned int>(length), bytesRead);
            if (err == DEVICE_OK) {
                // GetSerialAnswer discards the terminator
                if (lastc >= 0) bytesRead++;
                return bytesRead;
            } else {
                hub_->LogMessage("HubStreamAdapter::readBytes(buffer,length) failed: ");
                char text[MM::MaxStrLength];
                hub_->GetErrorText(err, text);
                hub_->LogMessage(text);
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
        //            std::string answer =
        //            MMCore::getSerialPortAnswer(portName,term);
        //                which checks that the term is not NULL and doesn't start
        //                with '\0' (NO NULL TERMINATORS) then creates another
        //                internal `char bufferB[1024]` and calls
        //                int ret =
        //                SerialInstance::GetAnswer(bufferB,1024,term);
        //                    which calls
        //                    GetImpl()->GetAnswer(bufferB, 1024, term);
        //            MMCore then creates a std::string from the NULL term bufferB
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
        std::string readStdStringUntil_impl(char terminator) {
            std::string compose;
            // deal with any character already in the read buffer
            int lastc;
            if (!rdbuf_.empty() && (lastc = read_impl()) >= 0) {
                compose.append(1, (char)lastc);
                if (lastc == terminator) return compose;
            }
            std::string answerString;
            const char termstr[]{terminator, '\0'};
            int err = hub_->GetSerialAnswer(hub_->port().c_str(), termstr, answerString);
            if (err == DEVICE_OK) {
                // GetSerialAnswer discards the terminator
                compose.append(answerString);
                compose.append(1, terminator);
            } else {
                hub_->LogMessage(
                    "HubStreamAdapter::readStdStringUntil(terminator) failed: ");
                char text[MM::MaxStrLength];
                hub_->GetErrorText(err, text);
                hub_->LogMessage(text);
            }
            return compose;
        }

        size_t readBytesUntil_impl(char terminator, char* buffer, size_t length) {
            std::string answer = readStdStringUntil_impl(terminator);
            size_t ngood       = std::min(answer.length(), length);
            std::strncpy(buffer, answer.c_str(), ngood);
            if (answer.length() > length) {
                hub_->LogMessage(
                    "HubStreamAdapter::readBytesUntil(terminator,buffer,length) "
                    "failed: ");
                char text[MM::MaxStrLength];
                hub_->GetErrorText(DEVICE_BUFFER_OVERFLOW, text);
                hub_->LogMessage(text);
            }
            return ngood;
        }

        /** buffer the next character because MMCore doesn't have a peek 
         * function for serial */
        void getNextChar() {
            if (rdbuf_.empty()) {
                unsigned long timeout = arduino::millis() + getTimeout();
                do {
                    unsigned char buf;
                    unsigned long read;
                    int err = hub_->ReadFromComPort(hub_->port().c_str(), &buf, 1, read);
                    if (err == DEVICE_OK && read > 0) {
                        rdbuf_.push_back(buf);
                        return;
                    }
                } while (arduino::millis() < timeout);
            }
        }

        std::deque<unsigned char> rdbuf_;
        HubT* hub_;
        mutable std::mutex guard_;
    };

} // namespace rdl

#endif // __STREAM_ADAPTER_H__