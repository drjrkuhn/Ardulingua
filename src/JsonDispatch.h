#pragma once

#ifndef __JSONDISPATCH_H__
    #define __JSONDISPATCH_H__

    #include "JsonDelegate.h"
    #include "Logger.h"
    #include "Polyfills/std_utility.h"
    #include "Polyfills/sys_timing.h"
    #include <ArduinoJson.h>
    #include <SlipInPlace.h>
    #include <thread>
    #include <unordered_map>

    // #define JSONRPC_USE_SHORT_KEYS 1
    // #define JSONRPC_USE_MSGPACK 1
    #define JSONRPC_DEBUG_CLIENTSERVER 1

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
        #define DCS(block) \
            { block; }
    #else
        #define DCS(block) \
            { ; }
    #endif

    // comparison operator for const char* maps
    // struct cmp_str {
    //     bool operator()(char const* a, char const* b) const { return std::strcmp(a, b) < 0; }
    // };

    constexpr size_t BUFFER_SIZE = 256;

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

    /************************************************************************
     * PROTOCOL BASE
     ***********************************************************************/
    template <class S, class STR, class LOG = logger_base<Print_null<STR>, STR>>
    class protocol_base {
     public:
        protocol_base(S& istream, S& ostream, uint8_t* buffer_data, size_t buffer_size, int max_retries = 3)
            : istream_(istream), ostream_(ostream), buffer_(buffer_data, buffer_size) {
            max_retries_ = max_retries;
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

        template <typename... PARAMS>
        int serialize_call(JsonDocument& msgdoc, size_t& msgsize, STR method, const int id, PARAMS... args) {
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
            DCS(logger_.print("\tserialized "); logger_.println(msgdoc));
            msgsize = slip_null_encoder::encode(buffer_.data(), buffer_.size(), buffer_.data(), msgsize);
            if (msgsize == 0)
                return ERROR_SLIP_ENCODING_ERROR;
            return ERROR_OK;
        }

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
            DCS(logger_.print("\tserialized"); logger_.println(msgdoc));
            msgsize = slip_null_encoder::encode(buffer_.data(), buffer_.size(), buffer_.data(), msgsize);
            if (msgsize == 0)
                return ERROR_SLIP_ENCODING_ERROR;
            return ERROR_OK;
        }

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
            DCS(logger_.print("\tserialized"); logger_.println(msgdoc));
            msgsize = slip_null_encoder::encode(buffer_.data(), buffer_.size(), buffer_.data(), msgsize);
            if (msgsize == 0)
                return ERROR_SLIP_ENCODING_ERROR;
            return ERROR_OK;
        }

        int deserialize_call(JsonDocument& msgdoc, size_t msgsize, STR& method, int& id, JsonArray& args) {
            // slip decode the message
            msgsize = slip_null_decoder::decode(buffer_.data(), buffer_.size(), buffer_.data(), msgsize);
            if (msgsize == 0)
                return ERROR_SLIP_DECODING_ERROR;
            // deserialize the message
            DeserializationError derr = deserializeMessage(msgdoc, buffer_.data(), msgsize);
            if (derr != DeserializationError::Ok)
                return ERROR_JSON_DESER_ERROR_0 - derr.code();
            DCS(logger_.print("\tdeserialized"); logger_.println(msgdoc));
            id = 37;
            if (!msgdoc.containsKey(RK_METHOD))
                return ERROR_JSON_INVALID_REQUEST;
            method = msgdoc[RK_METHOD].as<STR>();
            if (!msgdoc.containsKey(RK_PARAMS))
                return ERROR_JSON_INVALID_REQUEST;
            args = msgdoc[RK_PARAMS];
            if (msgdoc.containsKey(RK_ID))
                id = msgdoc[RK_ID];
            return ERROR_OK;
        }

        template <typename RTYPE>
        int deserialize_reply(JsonDocument& msgdoc, size_t msgsize, int msg_id, RTYPE& ret) {
            // decode message
            msgsize = slip_null_decoder::decode(buffer_.data(), buffer_.size(), buffer_.data(), msgsize);
            if (msgsize == 0)
                return ERROR_SLIP_DECODING_ERROR;
            DeserializationError derr = deserializeMessage(msgdoc, buffer_.data(), msgsize);
            if (derr != DeserializationError::Ok)
                return ERROR_JSON_DESER_ERROR_0 - derr.code();
            DCS(logger_.print("\tdeserialized"); logger_.println(msgdoc));
            JsonVariant jvid = msgdoc[RK_ID];
            // check for reply id
            if (jvid.isNull())
                return ERROR_JSON_INVALID_REPLY;
            int reply_id = jvid.as<int>();
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

        int deserialize_reply(JsonDocument& msgdoc, size_t msgsize, int msg_id) {
            // decode message
            msgsize = slip_null_decoder::decode(buffer_.data(), buffer_.size(), buffer_.data(), msgsize);
            if (msgsize == 0)
                return ERROR_SLIP_DECODING_ERROR;
            DeserializationError derr = deserializeMessage(msgdoc, buffer_.data(), msgsize);
            if (derr != DeserializationError::Ok)
                return ERROR_JSON_DESER_ERROR_0 - derr.code();
            DCS(logger_.print("\tdeserialized"); logger_.println(msgdoc));
            JsonVariant jvid = msgdoc[RK_ID];
            // check for reply id
            if (jvid.isNull() || jvid.as<int>() != msg_id)
                return ERROR_JSON_INVALID_REPLY;
            // check for error in reply
            return msgdoc[RK_ERROR] | ERROR_OK;
        }

        LOG& logger() { return logger_; }
        LOG& logger(LOG& log) {
            logger_ = log;
            return logger_;
        }

     protected:
        S& istream_;
        S& ostream_;
        svc::buffer buffer_;
        int max_retries_;
        LOG EMPTY_LOGGER;
        LOG& logger_ = EMPTY_LOGGER;
    };

    /************************************************************************
     * SERVER
     ***********************************************************************/
    template <class S, class MAP, class STR, size_t BUFSIZE, class LOG = logger_base<Print_null<STR>, STR>>
    class jsonserver : public protocol_base<S, STR, LOG> {
     public:
        typedef protocol_base<S, STR, LOG> BaseT;
        using BaseT::logger;

        jsonserver(S& istream, S& ostream, MAP& map, int max_retries = 2)
            : BaseT(istream, ostream, buffer_data_, BUFSIZE, max_retries),
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
            DCS(logger_.print("SERVER << "); logger_.print_escaped(buffer_.data(), msgsize, "'"); logger_.println());
            if (msgsize == 0)
                return ERROR_JSON_TIMEOUT;
            StaticJsonDocument<svc::JDOC_SIZE> msg;
            JsonArray args = msg.as<JsonArray>(); // dummy initializion
            StaticJsonDocument<svc::JRESULT_SIZE> resultdoc;
            JsonVariant result = resultdoc.as<JsonVariant>();
            STR method;
            auto mapit = dispatch_map_.end();
            int id = -1, err = ERROR_OK;
            for (;;) { // "try" clause. Always use break to exit
                err = BaseT::deserialize_call(msg, msgsize, method, id, args);
                if (err != ERROR_OK) break;
                mapit = dispatch_map_.find(method);
                err   = (mapit == dispatch_map_.end()) ? ERROR_JSON_METHOD_NOT_FOUND : ERROR_OK;
                if (err != ERROR_OK) break;
                err = mapit->second.call(args, result);
                break;
            }
            if (id >= 0) { // server wants reply
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
                DCS(logger_.print("SERVER >> "); logger_.print_escaped(buffer_.data(), msgsize, "'"); logger_.println());
            }
            return ERROR_OK;
        }

     protected:
        using BaseT::istream_;
        using BaseT::ostream_;
        using BaseT::buffer_;
        using BaseT::max_retries_;
        using BaseT::logger_;
        MAP& dispatch_map_;
        uint8_t buffer_data_[BUFSIZE];
    };

    /************************************************************************
     * CLIENT
     ***********************************************************************/
    template <class S, class STR, size_t BUFSIZE, class LOG = logger_base<Print_null<STR>, STR>>
    class jsonclient : protocol_base<S, STR, LOG> {
     public:
        typedef protocol_base<S, STR, LOG> BaseT;
        using BaseT::logger;

        jsonclient(S& istream, S& ostream, long reply_wait_ms = 200, int max_retries = 2)
            : BaseT(istream, ostream, buffer_data_, BUFSIZE, max_retries), // buffer_(buffer_data_, BUFSIZE),
              reply_wait_ms_(reply_wait_ms), nextid_(1) {
        }

        template <typename... PARAMS>
        int call(const char* method, PARAMS... args) {
            int tries    = max_retries_;
            int last_err = ERROR_OK;
            size_t msgsize;
            while (tries-- > 0) {
                int msg_id = nextid_++;
                last_err   = notify_impl<PARAMS...>(method, msg_id, args...);
                // get reply
                StaticJsonDocument<svc::JDOC_SIZE> msg;
                last_err = read_reply(msgsize);
                if (last_err != ERROR_OK || msgsize == 0)
                    return last_err;
                last_err = BaseT::deserialize_reply(msg, msgsize, msg_id);
                if (last_err != ERROR_OK)
                    continue; // try again
                // all good
                return ERROR_OK;
            }
            return last_err;
        }

        template <typename RTYPE, typename... PARAMS>
        int call(const char* method, RTYPE& ret, PARAMS... args) {
            int tries    = max_retries_;
            int last_err = ERROR_OK;
            size_t msgsize;
            while (tries-- > 0) {
                int msg_id = static_cast<int>(nextid_++);
                last_err   = notify_impl<PARAMS...>(method, msg_id, args...);
                // get reply
                StaticJsonDocument<svc::JDOC_SIZE> msg;
                last_err = read_reply(msgsize);
                if (last_err != ERROR_OK || msgsize == 0)
                    return last_err;
                last_err = BaseT::deserialize_reply(msg, msgsize, msg_id, ret);
                if (last_err != ERROR_OK)
                    continue; // try again
                // all good
                return ERROR_OK;
            }
            return last_err;
        }

        template <typename... PARAMS>
        int notify(const char* method, PARAMS... args) {
            return notify_impl(method, -1, args...);
        }

     protected:
        template <typename... PARAMS>
        int notify_impl(const char* method, int msg_id, PARAMS... args) {
            int last_err = ERROR_OK;
            size_t msgsize;
            StaticJsonDocument<svc::JDOC_SIZE> msg;
            last_err = this->template serialize_call<PARAMS...>(msg, msgsize, method, msg_id, args...);
            if (last_err != ERROR_OK)
                return last_err;
            size_t writesize = ostream_.write(buffer_.data(), msgsize);
            if (writesize < msgsize)
                return ERROR_JSON_SEND_ERROR;
            DCS(logger_.print("CLIENT >> "); logger_.print_escaped(buffer_.data(), writesize, "'"); logger_.println());
            return ERROR_OK;
        }

        int read_reply(size_t& msgsize) {
            msgsize        = 0;
            int wait_tries = max_retries_;
            while (wait_tries-- > 0) {
                if (istream_.available() > 0) {
                    msgsize = istream_.readBytesUntil(slip_stdcodes::SLIP_END, buffer_.data(), buffer_.size());
                    DCS(logger_.print("CLIENT << "); logger_.print_escaped(buffer_.data(), msgsize, "'"); logger_.println());
                    if (msgsize > 0)
                        return ERROR_OK;
                }
                if (reply_wait_ms_ > 0) {
                    sys_delay(reply_wait_ms_);
                } else {
                    sys_yield();
                }
            }
            return ERROR_JSON_TIMEOUT;
        }

        using BaseT::istream_;
        using BaseT::ostream_;
        using BaseT::buffer_;
        using BaseT::max_retries_;
        using BaseT::logger_;
        long reply_wait_ms_;
        size_t nextid_;
        uint8_t buffer_data_[BUFSIZE];
    };
}; // end namespace

    #undef DCS

#endif // __JSONDISPATCH_H__