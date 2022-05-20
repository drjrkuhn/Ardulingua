#pragma once

#ifndef __JSONSERVER_H__
    #define __JSONSERVER_H__

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
    #include <assert.h>

namespace rdl {

    /************************************************************************
     * SERVER
     ***********************************************************************/
    template <class MapT, class KeysT, size_t BUFSIZE>
    class json_server : public protocol_base<KeysT> {
     public:
        using BaseT = protocol_base<KeysT>;
        using BaseT::logger;

        json_server(sys::StreamT& istream, sys::StreamT& ostream, MapT& map, unsigned long timeout_ms = JSONRPC_DEFAULT_TIMEOUT,
                    unsigned long retry_delay_ms = JSONRPC_DEFAULT_RETRY_DELAY)
            : BaseT(istream, ostream, timeout_ms, retry_delay_ms), dispatch_map_(map), static_buffer_() {
            // MUST wait to initialize buffer until after static_buffer creation
            BaseT::buffer_ = static_buffer_;
        }

        int check_messages() {
            assert(buffer_.valid());
            size_t available = istream_.available();
            if (available == 0) {
                // std::cout << ".";
                return ERROR_OK;
            }
            // read message
            size_t msgsize = istream_.readBytesUntil(slip_std_codes::SLIP_END, buffer_.data(), buffer_.max_size());
            DCS_BLK(logger_->print(SERVER_COL "SERVER << "); print_escaped(*logger_, buffer_.data(), msgsize, "'"); logger_->println());
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
                if (err == ERROR_JSON_METHOD_NOT_FOUND) {
                    DCS_BLK(logger_->print(SERVER_COL "SERVER method "); logger_->print(method); logger_->println(" not found"));
                } else {
                    DCS_BLK(logger_->print(SERVER_COL "SERVER calling "); logger_->print(method); logger_->println());
                }
                if (err != ERROR_OK) break;
                json_stub jstub = mapit->second;
                err             = jstub.call(args, result);
                DSRV_BLK(logger_->print(SERVER_COL "SERVER called "); logger_->print(mapit->first); sys::StringT astr; ArduinoJson::serializeJson(args, astr); logger_->print(astr));
                DSRV_BLK(
                    if (err != ERROR_OK) {
                        logger_->print(" -> ERROR ");
                        logger_->println(err);
                    } else {
                        logger_->print(" -> ");
                        logger_->println(result.as<sys::StringT>());
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
                DCS_BLK(logger_->print(SERVER_COL "SERVER >> "); print_escaped(*logger_, buffer_.data(), msgsize, "'"); logger_->println());
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
        MapT& dispatch_map_;

        static_arraybuf<uint8_t, BUFSIZE> static_buffer_;
    };

} // namespace

#endif // __JSONSERVER_H__