#pragma once

#ifndef __JSONCLIENT_H__
    #define __JSONCLIENT_H__

    #include "JsonDelegate.h"
    #include "JsonError.h"
    #include "JsonProtocol.h"
    #include "Logger.h"
    #include "SlipInPlace.h"
    #include "std_utility.h"
    #include "sys_PrintT.h"
    #include "sys_StreamT.h"
    #include "sys_StringT.h"
    #include "sys_timing.h"
    #include <ArduinoJson.h>

namespace rdl {

    /************************************************************************
     * CLIENT
     ***********************************************************************/
    template <class KeysT, size_t BUFSIZE>
    class json_client : protocol_base<KeysT> {
     public:
        using BaseT = protocol_base<KeysT>;
        using BaseT::logger;

        json_client(sys::StreamT& istream, sys::StreamT& ostream, unsigned long timeout_ms = JSONRPC_DEFAULT_TIMEOUT,
                    unsigned long retry_delay_ms = JSONRPC_DEFAULT_RETRY_DELAY)
            : BaseT(istream, ostream, buffer_data_, BUFSIZE, timeout_ms, retry_delay_ms),
              nextid_(1) {
        }

        template <typename... PARAMS>
        int call(const char* method, PARAMS... args) {
            unsigned long starttime = sys::millis();
            unsigned long endtime   = starttime + timeout_ms_;
            unsigned long time;
            int last_err = ERROR_OK;
            size_t msgsize;
            long msg_id = nextid_++;
            last_err    = call_impl<PARAMS...>(method, msg_id, args...);
            int attempt = 0;
            while ((time = sys::millis()) < endtime) {
                attempt++;
                // give some time for the reply
                if (retry_delay_ms_ > 0) {
                    sys::delay(retry_delay_ms_);
                } else {
                    sys::yield();
                }
                DCS_BLK(logger_->print("CLIENT call read_reply attempt "); logger_->println(attempt));
                // get reply
                StaticJsonDocument<svc::JDOC_SIZE> msg;
                last_err = read_reply(msgsize);
                if (last_err == ERROR_OK) {
                    last_err = BaseT::deserialize_reply(msg, msgsize, msg_id);
                }
                if (last_err != ERROR_OK) {
                    if (last_err == ERROR_JSON_NO_REPLY) {
                        DCS_BLK(logger_->println("CLIENT no reply yet"));
                    } else {
                        DCS_BLK(logger_->print("CLIENT bad reply ERROR "); logger_->println(last_err));
                    }
                    continue; // try again
                }
                DCS_BLK(logger_->print("CLIENT call success"));
                DCS_BLK(logger_->print("\ttime ("); logger_->print(sys::millis() - starttime); logger_->println(" ms)"));

                // all good
                return ERROR_OK;
            }
            return last_err;
        }

        /** Call with no return and tuple of parameters */
        template <typename TUPLE>
        inline int call_tuple(const char* method, TUPLE args) {
            return call_tuple_impl(method, args, std::make_index_sequence<std::tuple_size<TUPLE>{}>{});
        }

        template <typename RTYPE, typename... PARAMS>
        int call_get(const char* method, RTYPE& ret, PARAMS... args) {
            unsigned long starttime = sys::millis();
            unsigned long endtime   = starttime + timeout_ms_;
            unsigned long time;
            int last_err = ERROR_OK;
            size_t msgsize;
            long msg_id = nextid_++;
            last_err    = call_impl<PARAMS...>(method, msg_id, args...);
            int attempt = 0;
            while ((time = sys::millis()) < endtime) {
                attempt++;
                // give some time for the reply
                if (retry_delay_ms_ > 0) {
                    sys::delay(retry_delay_ms_);
                } else {
                    sys::yield();
                }
                DCS_BLK(logger_->print("CLIENT call read_reply attempt "); logger_->println(attempt));
                // get reply
                StaticJsonDocument<svc::JDOC_SIZE> msg;
                last_err = read_reply(msgsize);
                if (last_err == ERROR_OK) {
                    last_err = BaseT::deserialize_reply(msg, msgsize, msg_id, ret);
                }
                if (last_err != ERROR_OK) {
                    if (last_err == ERROR_JSON_NO_REPLY) {
                        DCS_BLK(logger_->println("CLIENT no reply yet"));
                    } else {
                        DCS_BLK(logger_->print("CLIENT bad reply ERROR "); logger_->println(last_err));
                    }
                    continue; // try again
                }
                DCS_BLK(logger_->print("CLIENT call_get success"));
                DCS_BLK(logger_->print("\ttime ("); logger_->print(sys::millis() - starttime); logger_->println(" ms)"));
                // all good
                return ERROR_OK;
            }
            return last_err;
        }

        /** Call with return value and tuple of parameters */
        template <typename RTYPE, typename TUPLE>
        inline int call_get_tuple(const char* method, RTYPE& ret, TUPLE args) {
            return call_get_tuple_impl<RTYPE>(method, ret, args, std::make_index_sequence<std::tuple_size<TUPLE>{}>{});
        }

        template <typename... PARAMS>
        int notify(const char* method, PARAMS... args) {
            return call_impl(method, -1, args...);
        }

        /** Notify (no return) with tulple of parameters */
        template <typename TUPLE>
        inline int notify_tuple(const char* method, TUPLE args) {
            return notify_tuple_impl(method, args, std::make_index_sequence<std::tuple_size<TUPLE>{}>{});
        }

     protected:
        template <typename TUPLE, size_t... I>
        inline int call_tuple_impl(const char* method, TUPLE args, std::index_sequence<I...>) {
            return call(method, std::get<I>(args)...);
        }

        template <typename RTYPE, typename TUPLE, size_t... I>
        inline int call_get_tuple_impl(const char* method, RTYPE& ret, TUPLE args, std::index_sequence<I...>) {
            return call_get<RTYPE>(method, ret, std::get<I>(args)...);
        }

        template <typename TUPLE, size_t... I>
        inline int notify_tuple_impl(const char* method, TUPLE args, std::index_sequence<I...>) {
            return notify(method, std::get<I>(args)...);
        }

        template <typename... PARAMS>
        int call_impl(const char* method, long msg_id, PARAMS... args) {
            int last_err = ERROR_OK;
            size_t msgsize;
            StaticJsonDocument<svc::JDOC_SIZE> msg;
            last_err = this->template serialize_call<PARAMS...>(msg, msgsize, method, msg_id, args...);
            if (last_err != ERROR_OK)
                return last_err;
            size_t writesize = ostream_.write(buffer_.data(), msgsize);
            if (writesize < msgsize)
                return ERROR_JSON_SEND_ERROR;
            DCS_BLK(logger_->print("CLIENT >> "); print_escaped(*logger_, buffer_.data(), writesize, "'"); logger_->println());
            return ERROR_OK;
        }

    #if 0
        int read_reply(size_t& msgsize) {
            unsigned long starttime = sys::millis();
            unsigned long endtime = starttime + timeout_ms_;
            unsigned long time;
            msgsize        = 0;
            while ((time = sys::millis()) < endtime) {
                if (istream_.available() > 0) {
                    msgsize = istream_.readBytesUntil(slip_std_codes::SLIP_END, buffer_.data(), buffer_.size());
                    DCS_BLK(logger_->print("CLIENT << "); logger_->print_escaped(buffer_.data(), msgsize, "'"); logger_->println());
                    if (msgsize > 0) {
                        DCS_BLK(logger_->print("CLIENT read_reply found"));
                        DCS_BLK(logger_->print("\ttime ("); logger_->print(sys::millis() - starttime); logger_->println(" ms)"));
                        return ERROR_OK;
                    }
                }
                if (retry_delay_ms_ > 0) {
                    sys::delay(retry_delay_ms_);
                } else {
                    sys::yield();
                }
            }
            DCS_BLK(logger_->print("CLIENT read_reply TIMEOUT"));
            DCS_BLK(logger_->print("\ttime ("); logger_->print(sys::millis() - starttime); logger_->println(" ms)"));
            return ERROR_JSON_TIMEOUT;
        }
    #else
        int read_reply(size_t& msgsize) {
            if (istream_.available() == 0) {
                // Let the caller deal with timeouts
                return ERROR_JSON_NO_REPLY;
            }
            DCS(unsigned long starttime = sys::millis());

            msgsize = istream_.readBytesUntil(slip_std_codes::SLIP_END, buffer_.data(), buffer_.size());
            DCS_BLK(logger_->print("CLIENT << "); print_escaped(*logger_, buffer_.data(), msgsize, "'"); logger_->println());
            if (msgsize > 0) {
                DCS_BLK(logger_->print("CLIENT read_reply found"));
                DCS_BLK(logger_->print("\ttime ("); logger_->print(sys::millis() - starttime); logger_->println(" ms)"));
                return ERROR_OK;
            }
            DCS_BLK(logger_->print("CLIENT read_reply INVALID REPLY"));
            DCS_BLK(logger_->print("\ttime ("); logger_->print(sys::millis() - starttime); logger_->println(" ms)"));
            return ERROR_JSON_INVALID_REPLY;
        }

    #endif

        using BaseT::istream_;
        using BaseT::ostream_;
        using BaseT::buffer_;
        using BaseT::timeout_ms_;
        using BaseT::retry_delay_ms_;
        using BaseT::logger_;
        long nextid_;
        uint8_t buffer_data_[BUFSIZE];
    };

} // namespace

#endif // __JSONCLIENT_H__