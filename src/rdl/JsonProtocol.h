#pragma once

#ifndef __JSONPROTOCOL_H__
    #define __JSONPROTOCOL_H__

    #include "JsonDelegate.h"
    #include "JsonError.h"
    #include "Logger.h"
    #include "SlipInPlace.h"
    #include "std_utility.h"
    #include "sys_PrintT.h"
    #include "sys_StreamT.h"
    #include "sys_StringT.h"
    #include "sys_timing.h"
    #include <ArduinoJson.h>

    #define JSONRPC_USE_SHORT_KEYS 1
// #define JSONRPC_USE_MSGPACK 1
// #define JSONRPC_DEBUG_CLIENTSERVER 1
// #define JSONRPC_DEBUG_SERVER_DISPATCH 1

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

    // HELPER MACROS
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

    struct jsonrpc_std_keys {
        static constexpr const char* RK_METHOD = "method";
        static constexpr const char* RK_PARAMS = "params";
        static constexpr const char* RK_ID     = "id";
        static constexpr const char* RK_RESULT = "result";
        static constexpr const char* RK_ERROR  = "error";
    };

    struct jsonrpc_short_keys {
        static constexpr const char* RK_METHOD = "m";
        static constexpr const char* RK_PARAMS = "p";
        static constexpr const char* RK_ID     = "i";
        static constexpr const char* RK_RESULT = "r";
        static constexpr const char* RK_ERROR  = "e";
    };

    #if JSONRPC_USE_SHORT_KEYS
    using jsonrpc_default_keys = jsonrpc_short_keys;
    #else
    using jsonrpc_default_keys = jsonrpc_std_keys;
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

    #define SERVER_COL "\t\t\t\t"

    /************************************************************************
     * PROTOCOL BASE
     ***********************************************************************/

    template <class KeysT>
    class protocol_base {
     public:
        static constexpr const char* key_method() noexcept { return KeysT::RK_METHOD; }
        static constexpr const char* key_params() noexcept { return KeysT::RK_PARAMS; }
        static constexpr const char* key_id() noexcept { return KeysT::RK_ID; }
        static constexpr const char* key_result() noexcept { return KeysT::RK_RESULT; }
        static constexpr const char* key_error() noexcept { return KeysT::RK_ERROR; }

        protocol_base(sys::StreamT& istream, sys::StreamT& ostream, uint8_t* buffer_data, size_t buffer_size,
                      unsigned long timeout_ms     = JSONRPC_DEFAULT_TIMEOUT,
                      unsigned long retry_delay_ms = JSONRPC_DEFAULT_RETRY_DELAY)
            : istream_(istream), ostream_(ostream), buffer_(buffer_data, buffer_size), logger_(no_logger()) {
            timeout_ms_     = timeout_ms;
            retry_delay_ms_ = retry_delay_ms;
        }

        sys::PrintT* logger() { return logger_; }

        sys::PrintT* logger(sys::PrintT* logger) {
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
                msgdoc[key_error()] = error_code;
            } else if (!result.isNull()) {
                msgdoc[key_result()] = result;
            }
            msgdoc[key_id()] = id;
            // serialize the message
            msgsize = serializeMessage(msgdoc, buffer_.data(), buffer_.size());
            if (msgsize == 0)
                return ERROR_JSON_INTERNAL_ERROR;
            DCS_BLK(logger_->print(SERVER_COL "\tserialized"); println(*logger_, msgdoc));
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
                msgdoc[key_error()] = error_code;
            }
            msgdoc[key_id()] = id;
            // serialize the message
            msgsize = serializeMessage(msgdoc, buffer_.data(), buffer_.size());
            if (msgsize == 0)
                return ERROR_JSON_INTERNAL_ERROR;
            DCS_BLK(logger_->print(SERVER_COL "\tserialized"); println(*logger_, msgdoc));
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
            DCS_BLK(logger_->print(SERVER_COL "\tdeserialized"); println(*logger_, msgdoc));
            if (!msgdoc.containsKey(key_method()))
                return ERROR_JSON_INVALID_REQUEST;
            JsonVariant jmethod = msgdoc[key_method()];
            method = jmethod.as<sys::StringT>();
            if (!msgdoc.containsKey(key_params()))
                return ERROR_JSON_INVALID_REQUEST;
            args = msgdoc[key_params()];
            if (msgdoc.containsKey(key_id()))
                id = msgdoc[key_id()];
            return ERROR_OK;
        }

        // CLIENT METHOD
        template <typename... PARAMS>
        int serialize_call(JsonDocument& msgdoc, size_t& msgsize, sys::StringT method, const int id, PARAMS... args) {
            // serialize the message
            msgdoc[key_method()] = method;
            JsonArray params     = msgdoc.createNestedArray(key_params());
            int err              = toJsonArray(params, args...);
            if (err != ERROR_OK)
                return err;
            if (id >= 0)
                msgdoc[key_id()] = id; // request reply
            // slip-encode message
            msgsize = serializeMessage(msgdoc, buffer_.data(), buffer_.size());
            if (msgsize == 0)
                return ERROR_JSON_ENCODING_ERROR;
            DCS_BLK(logger_->print("\tserialized "); println(*logger_, msgdoc));
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
            DCS_BLK(logger_->print("\tdeserialized"); println(*logger_, msgdoc));
            JsonVariant jvid = msgdoc[key_id()];
            // check for reply id
            if (jvid.isNull())
                return ERROR_JSON_INVALID_REPLY;
            long reply_id = jvid.as<long>();
            if (reply_id != msg_id)
                return ERROR_JSON_INVALID_REPLY;
            // check result in reply
            JsonVariant jvres = msgdoc[key_result()];
            if (!jvid.isNull()) { // received good reply
                ret = jvres.as<RTYPE>();
                return ERROR_OK;
            }
            // check for error in reply
            return msgdoc[key_error()] | ERROR_JSON_INVALID_REPLY;
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
            DCS_BLK(logger_->print("\tdeserialized"); println(*logger_, msgdoc));
            JsonVariant jvid = msgdoc[key_id()];
            // check for reply id
            if (jvid.isNull() || jvid.as<long>() != msg_id)
                return ERROR_JSON_INVALID_REPLY;
            // check for error in reply
            return msgdoc[key_error()] | ERROR_OK;
        }

     protected:
        static sys::PrintT* no_logger() {
            static Null_Print null_printer;
            return &null_printer;
        }

        sys::StreamT& istream_;
        sys::StreamT& ostream_;
        svc::buffer buffer_;
        unsigned long timeout_ms_;
        unsigned long retry_delay_ms_;
        sys::PrintT* logger_;
    };

}; // end namespace

#endif // __JSONPROTOCOL_H__