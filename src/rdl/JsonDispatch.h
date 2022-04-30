#pragma once

#ifndef __JSONDISPATCH_H__
    #define __JSONDISPATCH_H__

    #include "sys_StringT.h"
    #include "sys_PrintT.h"
    #include "sys_StreamT.h"
    #include "JsonDelegate.h"
    #include "Logger.h"
    #include "std_utility.h"
    #include "sys_timing.h"
    #include "SlipInPlace.h"
    #include <ArduinoJson.h>
    #include <thread>
    #include <unordered_map>

    #define JSONRPC_USE_SHORT_KEYS 1
    // #define JSONRPC_USE_MSGPACK 1
    #define JSONRPC_DEBUG_CLIENTSERVER 1
    #define JSONRPC_DEBUG_SERVER_DISPATCH 1

    #define JSONRPC_DEFAULT_TIMEOUT 1000
    #define JSONRPC_DEFAULT_RETRY_DELAY 1
    #define JSONRCP_BUFFER_SIZE 256

// SWITCH TESTING

    #if defined(JSONRPC_USE_SHORT_KEYS) && (JSONRPC_USE_SHORT_KEYS != 0)
        #define JSONRPC_USE_SHORT_KEYS 1
    #else
        #define JSONRPC_USE_SHORT_KEYS 0
    #endif

    #if defined(JSONRPC_USE_MSGPACK) && (JSONRPC_USE_MSGPACK != 0)
        #define JSONRPC_USE_MSGPACK 1
    #else
        #define JSONRPC_USE_MSGPACK 0
    #endif

    #if defined(JSONRPC_DEBUG_CLIENTSERVER) && (JSONRPC_DEBUG_CLIENTSERVER != 0)
        #define JSONRPC_DEBUG_CLIENTSERVER 1
    #else
        #define JSONRPC_DEBUG_CLIENTSERVER 0
    #endif

    #if defined(JSONRPC_DEBUG_SERVER_DISPATCH) && (JSONRPC_DEBUG_SERVER_DISPATCH != 0)
        #define JSONRPC_DEBUG_SERVER_DISPATCH 1
    #else
        #define JSONRPC_DEBUG_SERVER_DISPATCH 0
    #endif

namespace rdl {

    /************************************************************************
     * # Json Dispatch
     *
     * Simplified JSON-RPC scheme designe for fast lookup/access on
     * microcontrollers. Similar to the [JSON-RPC 2.0 specification](https://www.jsonrpc.org)
     * but with the following changes
     * - NO `{"jsonrpc": "2.0"}` tag in the message
     * - NO named parameters, only positional parameters
     * - "m" short for "method"
     * - "p" short for "params"
     * - "i" short for "id"
     * - "r" short for "result"
     * - errors are not returned as a {code,message} named tuple,
     *   just the numeric code.
     * - instead of
     *      "error": {"code": -32600, "message": "..."}
     * - error returned as
     *      "e": -32600
     * - remote methods with void returns just return the id
     *
     * ## RPC call with positional parameters
     * --> {"m": "subtract", "p": [42, 23], "i": 1}
     * <-- {"r": 19, "i": 1}
     *
     * ## RPC call with 'void' return [SET]
     * --> {"m": "setfoo", "p": [42], "i": 2}
     * <-- {"i": 2}
     *
     * ## RPC call with return and no parameters [GET]
     * --> {"m": "getfoo", "i": 3}
     * <-- {"r": 42, "i": 3}
     *
     * ### RPC notification (no id means no reply requested)
     * --> {"m": "update", "p": [1,2,3,4,5]}
     * --> {"m": "foobar"}
     *
     * ### RPC call with error return
     * --> {"m": "subtract", "p": [42], "i": 3}
     * <-- {"e": -32600, "i": 3}
     *
     * ## RPC set-notify/get get pair [SETN-GET]
     * --> {"m": "setfoo", "p": [3.1999]}
     * --> {"m": "gettfoo", "i": 4}
     * <-- {"r": 3.2, "i": 4}
     *
     ***********************************************************************/

    #if JSONRPC_USE_SHORT_KEYS
    constexpr const char* RK_METHOD = "m";
    constexpr const char* RK_PARAMS = "p";
    constexpr const char* RK_ID     = "i";
    constexpr const char* RK_RESULT = "r";
    constexpr const char* RK_ERROR  = "e";
    #else
    constexpr const char* RK_METHOD = "method";
    constexpr const char* RK_PARAMS = "params";
    constexpr const char* RK_ID     = "id";
    constexpr const char* RK_RESULT = "result";
    constexpr const char* RK_ERROR  = "error";
    #endif

    #if JSONRPC_USE_MSGPACK
    template <typename TSource>
    size_t serializeMessage(const TSource& source, void* buffer, size_t bufferSize) {
        return serializeMsgPack(source, buffer, bufferSize);
    }

    template <typename TChar>
    DeserializationError deserializeMessage(JsonDocument& doc, TChar* input, size_t inputSize) {
        return deserializeMsgPack(doc, input, inputSize);
    }
    #else
    template <typename TSource>
    size_t serializeMessage(const TSource& source, void* buffer, size_t bufferSize) {
        return serializeJson(source, buffer, bufferSize);
    }

    template <typename TChar>
    DeserializationError deserializeMessage(JsonDocument& doc, TChar* input, size_t inputSize) {
        return deserializeJson(doc, input, inputSize);
    }
    #endif

    #if JSONRPC_DEBUG_CLIENTSERVER
        #define DCS(statement) statement
        #define DCS_BLK(block) \
            { block; }
    #else
        #define DCS(statement)
        #define DCS_BLK(block) \
            {}
    #endif
    #if JSONRPC_DEBUG_SERVER_DISPATCH
        #define DSRV(statement) statement
        #define DSRV_BLK(block) \
            { block; }
    #else
        #define DSRV(statement)
        #define DSRV_BLK(block) \
            {}
    #endif

    namespace svc {
        constexpr int MAX_PARAMETERS  = 6;
        constexpr size_t JDOC_SIZE    = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(MAX_PARAMETERS);
        constexpr size_t JRESULT_SIZE = JSON_OBJECT_SIZE(1);

        class buffer {
         public:
            buffer(uint8_t* data, size_t size) : data_(data), size_(size) {}
            uint8_t* data() const { return data_; }
            size_t size() const { return size_; }

         protected:
            uint8_t* data_;
            size_t size_;
        };

    };

    #define SERVER_COL "\t\t\t\t\t\t"

    /************************************************************************
     * PROTOCOL BASE
     ***********************************************************************/
    template <class IS, class OS>
    class protocol_base {
     public:
        protocol_base(IS& istream, OS& ostream, uint8_t* buffer_data, size_t buffer_size,
                      unsigned long timeout_ms     = JSONRPC_DEFAULT_TIMEOUT,
                      unsigned long retry_delay_ms = JSONRPC_DEFAULT_RETRY_DELAY)
            : istream_(istream), ostream_(ostream), buffer_(buffer_data, buffer_size), logger_(Null_Print()) {
            timeout_ms_     = timeout_ms;
            retry_delay_ms_ = retry_delay_ms;
        }

        sys::PrintT& logger() { return logger_; }
        
        sys::PrintT& logger(sys::PrintT& logger) {
            logger_ = logger;
            return logger_;
        }



        template <typename... PARAMS>
        int toJsonArray(JsonArray& params, PARAMS... args) {
            bool add_res[sizeof...(args)] = {params.add(args)...};
            for (bool b : add_res) {
                if (!b)
                    return ERROR_JSON_INVALID_PARAMS;
            }
            return ERROR_OK;
        }

        int toJsonArray(JsonArray&) {
            return ERROR_OK;
        }

        // SEVER_METHOD
        /** Reply with return value or possible error */
        int serialize_reply(JsonDocument& msgdoc, size_t& msgsize, const int id, JsonVariant result, int error_code) {
            // serialize the message
            if (error_code != ERROR_OK) {
                msgdoc[RK_ERROR] = error_code;
            } else if (!result.isNull()) {
                msgdoc[RK_RESULT] = result;
            }
            msgdoc[RK_ID] = id;
            // serialize the message
            msgsize = serializeMessage(msgdoc, buffer_.data(), buffer_.size());
            if (msgsize == 0)
                return ERROR_JSON_INTERNAL_ERROR;
            DCS_BLK(logger_.print(SERVER_COL "\tserialized"); println(logger_, msgdoc));
            msgsize = slip_null_encoder::encode(buffer_.data(), buffer_.size(), buffer_.data(), msgsize);
            if (msgsize == 0)
                return ERROR_SLIP_ENCODING_ERROR;
            return ERROR_OK;
        }

        // SERVER_METHOD
        /** Reply with no return (void) but possible error */
        int serialize_reply(JsonDocument& msgdoc, size_t& msgsize, const int id, int error_code) {
            // serialize the message
            if (error_code != ERROR_OK) {
                msgdoc[RK_ERROR] = error_code;
            }
            msgdoc[RK_ID] = id;
            // serialize the message
            msgsize = serializeMessage(msgdoc, buffer_.data(), buffer_.size());
            if (msgsize == 0)
                return ERROR_JSON_INTERNAL_ERROR;
            DCS_BLK(logger_.print(SERVER_COL "\tserialized"); println(logger_, msgdoc));
            msgsize = slip_null_encoder::encode(buffer_.data(), buffer_.size(), buffer_.data(), msgsize);
            if (msgsize == 0)
                return ERROR_SLIP_ENCODING_ERROR;
            return ERROR_OK;
        }

        // SERVER_METHOD
        int deserialize_call(JsonDocument& msgdoc, size_t msgsize, sys::StringT& method, int& id, JsonArray& args) {
            // slip decode the message
            msgsize = slip_null_decoder::decode(buffer_.data(), buffer_.size(), buffer_.data(), msgsize);
            if (msgsize == 0)
                return ERROR_SLIP_DECODING_ERROR;
            // deserialize the message
            DeserializationError derr = deserializeMessage(msgdoc, buffer_.data(), msgsize);
            if (derr != DeserializationError::Ok)
                return ERROR_JSON_DESER_ERROR_0 - derr.code();
            DCS_BLK(logger_.print(SERVER_COL "\tdeserialized"); println(logger_, msgdoc));
            if (!msgdoc.containsKey(RK_METHOD))
                return ERROR_JSON_INVALID_REQUEST;
            method = msgdoc[RK_METHOD].as<sys::StringT>();
            if (!msgdoc.containsKey(RK_PARAMS))
                return ERROR_JSON_INVALID_REQUEST;
            args = msgdoc[RK_PARAMS];
            if (msgdoc.containsKey(RK_ID))
                id = msgdoc[RK_ID];
            return ERROR_OK;
        }

        // CLIENT METHOD
        template <typename... PARAMS>
        int serialize_call(JsonDocument& msgdoc, size_t& msgsize, sys::StringT method, const int id, PARAMS... args) {
            // serialize the message
            msgdoc[RK_METHOD] = method;
            JsonArray params  = msgdoc.createNestedArray(RK_PARAMS);
            int err           = toJsonArray(params, args...);
            if (err != ERROR_OK)
                return err;
            if (id >= 0)
                msgdoc[RK_ID] = id; // request reply
            // slip-encode message
            msgsize = serializeMessage(msgdoc, buffer_.data(), buffer_.size());
            if (msgsize == 0)
                return ERROR_JSON_ENCODING_ERROR;
            DCS_BLK(logger_.print("\tserialized "); println(logger_, msgdoc));
            msgsize = slip_null_encoder::encode(buffer_.data(), buffer_.size(), buffer_.data(), msgsize);
            if (msgsize == 0)
                return ERROR_SLIP_ENCODING_ERROR;
            return ERROR_OK;
        }

        // CLIENT METHOD
        template <typename RTYPE>
        int deserialize_reply(JsonDocument& msgdoc, size_t msgsize, long msg_id, RTYPE& ret) {
            // decode message
            msgsize = slip_null_decoder::decode(buffer_.data(), buffer_.size(), buffer_.data(), msgsize);
            if (msgsize == 0)
                return ERROR_SLIP_DECODING_ERROR;
            DeserializationError derr = deserializeMessage(msgdoc, buffer_.data(), msgsize);
            if (derr != DeserializationError::Ok)
                return ERROR_JSON_DESER_ERROR_0 - derr.code();
            DCS_BLK(logger_.print("\tdeserialized"); println(logger_, msgdoc));
            JsonVariant jvid = msgdoc[RK_ID];
            // check for reply id
            if (jvid.isNull())
                return ERROR_JSON_INVALID_REPLY;
            long reply_id = jvid.as<long>();
            if (reply_id != msg_id)
                return ERROR_JSON_INVALID_REPLY;
            // check result in reply
            JsonVariant jvres = msgdoc[RK_RESULT];
            if (!jvid.isNull()) { // received good reply
                ret = jvres.as<RTYPE>();
                return ERROR_OK;
            }
            // check for error in reply
            return msgdoc[RK_ERROR] | ERROR_JSON_INVALID_REPLY;
        }

        // CLIENT METHOD
        int deserialize_reply(JsonDocument& msgdoc, size_t msgsize, long msg_id) {
            // decode message
            msgsize = slip_null_decoder::decode(buffer_.data(), buffer_.size(), buffer_.data(), msgsize);
            if (msgsize == 0)
                return ERROR_SLIP_DECODING_ERROR;
            DeserializationError derr = deserializeMessage(msgdoc, buffer_.data(), msgsize);
            if (derr != DeserializationError::Ok)
                return ERROR_JSON_DESER_ERROR_0 - derr.code();
            DCS_BLK(logger_.print("\tdeserialized"); logger_.println(msgdoc));
            JsonVariant jvid = msgdoc[RK_ID];
            // check for reply id
            if (jvid.isNull() || jvid.as<long>() != msg_id)
                return ERROR_JSON_INVALID_REPLY;
            // check for error in reply
            return msgdoc[RK_ERROR] | ERROR_OK;
        }

     protected:
        IS& istream_;
        OS& ostream_;
        svc::buffer buffer_;
        unsigned long timeout_ms_;
        unsigned long retry_delay_ms_;
        sys::PrintT& logger_;
    };

    /************************************************************************
     * SERVER
     ***********************************************************************/
    template <class IS, class OS, class MAP, size_t BUFSIZE>
    class json_server : public protocol_base<IS, OS> {
     public:
        typedef protocol_base<IS, OS> BaseT;
        using BaseT::logger;

        json_server(IS& istream, OS& ostream, MAP& map, unsigned long timeout_ms = JSONRPC_DEFAULT_TIMEOUT,
                    unsigned long retry_delay_ms = JSONRPC_DEFAULT_RETRY_DELAY)
            : BaseT(istream, ostream, buffer_data_, BUFSIZE, timeout_ms, retry_delay_ms),
              dispatch_map_(map) {
        }

        int check_messages() {
            size_t available = istream_.available();
            if (available == 0) {
                // std::cout << ".";
                return ERROR_OK;
            }
            // read message
            size_t msgsize = istream_.readBytesUntil(slip_stdcodes::SLIP_END, buffer_.data(), buffer_.size());
            DCS_BLK(logger_.print(SERVER_COL "SERVER << "); logger_.print_escaped(buffer_.data(), msgsize, "'"); logger_.println());
            if (msgsize == 0)
                return ERROR_JSON_TIMEOUT;
            StaticJsonDocument<svc::JDOC_SIZE> msg;
            JsonArray args = msg.as<JsonArray>(); // dummy initializion
            StaticJsonDocument<svc::JRESULT_SIZE> resultdoc;
            JsonVariant result = resultdoc.as<JsonVariant>();
            sys::StringT method;
            auto mapit = dispatch_map_.end();
            int id = -1, err = ERROR_OK;
            for (;;) { // "try" clause. Always use break to exit
                err = BaseT::deserialize_call(msg, msgsize, method, id, args);
                if (err != ERROR_OK) break;
                mapit = dispatch_map_.find(method);
                err   = (mapit == dispatch_map_.end()) ? ERROR_JSON_METHOD_NOT_FOUND : ERROR_OK;
                if (err != ERROR_OK) break;
                json_stub jstub = mapit->second;
                err             = jstub.call(args, result);
                DSRV_BLK(logger_.print(SERVER_COL "SERVER called "); logger_.print(mapit->first); serializeJson(args, logger_.printer()));
                DSRV_BLK(
                    if (err != ERROR_OK) {
                        logger_.print(" -> ERROR ");
                        logger_.println(err);
                    } else {
                        logger_.print(" -> ");
                        logger_.println(result.as<sys::StringT>());
                    });
                break;
            }
            if (id >= 0) { // server wants reply
                sys::yield();
                msg.clear();
                if (mapit != dispatch_map_.end() && !mapit->second.returns_void()) {
                    err = BaseT::serialize_reply(msg, msgsize, id, result, err);
                } else {
                    err = BaseT::serialize_reply(msg, msgsize, id, err);
                }
                if (err != ERROR_OK)
                    return err;
                size_t writesize = ostream_.write(buffer_.data(), msgsize);
                if (writesize < msgsize)
                    return ERROR_JSON_SEND_ERROR;
                DCS_BLK(logger_.print(SERVER_COL "SERVER >> "); logger_.print_escaped(buffer_.data(), msgsize, "'"); logger_.println());
            }
            return ERROR_OK;
        }

     protected:
        using BaseT::istream_;
        using BaseT::ostream_;
        using BaseT::buffer_;
        using BaseT::timeout_ms_;
        using BaseT::retry_delay_ms_;
        using BaseT::logger_;
        MAP& dispatch_map_;
        uint8_t buffer_data_[BUFSIZE];
    };

    /************************************************************************
     * CLIENT
     ***********************************************************************/
    template <class IS, class OS, size_t BUFSIZE>
    class json_client : protocol_base<IS, OS> {
     public:
        typedef protocol_base<IS, OS> BaseT;
        using BaseT::logger;

        json_client(IS& istream, OS& ostream, unsigned long timeout_ms = JSONRPC_DEFAULT_TIMEOUT,
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
                DCS_BLK(logger_.print("CLIENT call read_reply attempt "); logger_.println(attempt));
                // get reply
                StaticJsonDocument<svc::JDOC_SIZE> msg;
                last_err = read_reply(msgsize);
                if (last_err == ERROR_OK) {
                    last_err = BaseT::deserialize_reply(msg, msgsize, msg_id);
                }
                if (last_err != ERROR_OK) {
                    if (last_err == ERROR_JSON_NO_REPLY) {
                        DCS_BLK(logger_.println("CLIENT no reply yet"));
                    } else {
                        DCS_BLK(logger_.print("CLIENT bad reply ERROR "); logger_.println(last_err));
                    }
                    continue; // try again
                }
                DCS_BLK(logger_.print("CLIENT call success"));
                DCS_BLK(logger_.print("\ttime ("); logger_.print(sys::millis() - starttime); logger_.println(" ms)"));

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
                DCS_BLK(logger_.print("CLIENT call read_reply attempt "); logger_.println(attempt));
                // get reply
                StaticJsonDocument<svc::JDOC_SIZE> msg;
                last_err = read_reply(msgsize);
                if (last_err == ERROR_OK) {
                    last_err = BaseT::deserialize_reply(msg, msgsize, msg_id, ret);
                }
                if (last_err != ERROR_OK) {
                    if (last_err == ERROR_JSON_NO_REPLY) {
                        DCS_BLK(logger_.println("CLIENT no reply yet"));
                    } else {
                        DCS_BLK(logger_.print("CLIENT bad reply ERROR "); logger_.println(last_err));
                    }
                    continue; // try again
                }
                DCS_BLK(logger_.print("CLIENT call_get success"));
                DCS_BLK(logger_.print("\ttime ("); logger_.print(sys::millis() - starttime); logger_.println(" ms)"));
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
            DCS_BLK(logger_.print("CLIENT >> "); logger_.print_escaped(buffer_.data(), writesize, "'"); logger_.println());
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
                    msgsize = istream_.readBytesUntil(slip_stdcodes::SLIP_END, buffer_.data(), buffer_.size());
                    DCS_BLK(logger_.print("CLIENT << "); logger_.print_escaped(buffer_.data(), msgsize, "'"); logger_.println());
                    if (msgsize > 0) {
                        DCS_BLK(logger_.print("CLIENT read_reply found"));
                        DCS_BLK(logger_.print("\ttime ("); logger_.print(sys::millis() - starttime); logger_.println(" ms)"));
                        return ERROR_OK;
                    }
                }
                if (retry_delay_ms_ > 0) {
                    sys::delay(retry_delay_ms_);
                } else {
                    sys::yield();
                }
            }
            DCS_BLK(logger_.print("CLIENT read_reply TIMEOUT"));
            DCS_BLK(logger_.print("\ttime ("); logger_.print(sys::millis() - starttime); logger_.println(" ms)"));
            return ERROR_JSON_TIMEOUT;
        }
    #else
        int read_reply(size_t& msgsize) {
            if (istream_.available() == 0) {
                // Let the caller deal with timeouts
                return ERROR_JSON_NO_REPLY;
            }
            DCS(unsigned long starttime = sys::millis());

            msgsize = istream_.readBytesUntil(slip_stdcodes::SLIP_END, buffer_.data(), buffer_.size());
            DCS_BLK(logger_.print("CLIENT << "); logger_.print_escaped(buffer_.data(), msgsize, "'"); logger_.println());
            if (msgsize > 0) {
                DCS_BLK(logger_.print("CLIENT read_reply found"));
                DCS_BLK(logger_.print("\ttime ("); logger_.print(sys::millis() - starttime); logger_.println(" ms)"));
                return ERROR_OK;
            }
            DCS_BLK(logger_.print("CLIENT read_reply INVALID REPLY"));
            DCS_BLK(logger_.print("\ttime ("); logger_.print(sys::millis() - starttime); logger_.println(" ms)"));
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
}; // end namespace

    #undef DCS_BLK

#endif // __JSONDISPATCH_H__