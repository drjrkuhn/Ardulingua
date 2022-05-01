#pragma once

#ifndef __JSONERRORS_H__
#define __JSONERRORS_H__

#include <ArduinoJson.h>

namespace rdl {
    constexpr int ERROR_OK                    = 0;
    constexpr int ERROR_JSON_PARSE_ERROR      = -32700;
    constexpr int ERROR_JSON_INVALID_REQUEST  = -32600;
    constexpr int ERROR_JSON_METHOD_NOT_FOUND = -32601;
    constexpr int ERROR_JSON_INVALID_PARAMS   = -32602;
    constexpr int ERROR_JSON_INTERNAL_ERROR   = -32603;

    constexpr int ERROR_JSON_RET_NOT_SET    = -32000;
    constexpr int ERROR_JSON_ENCODING_ERROR = -32001;
    constexpr int ERROR_JSON_SEND_ERROR     = -32002;
    constexpr int ERROR_JSON_TIMEOUT        = -32003;
    constexpr int ERROR_JSON_NO_REPLY       = -32004;
    constexpr int ERROR_JSON_INVALID_REPLY  = -32005;
    constexpr int ERROR_SLIP_ENCODING_ERROR = -32006;
    constexpr int ERROR_SLIP_DECODING_ERROR = -32007;

    constexpr int ERROR_JSON_DESER_ERROR_0          = -32090;
    constexpr int ERROR_JSON_DESER_EMPTY_INPUT      = ERROR_JSON_DESER_ERROR_0 - ArduinoJson::DeserializationError::EmptyInput;
    constexpr int ERROR_JSON_DESER_INCOMPLETE_INPUT = ERROR_JSON_DESER_ERROR_0 - ArduinoJson::DeserializationError::IncompleteInput;
    constexpr int ERROR_JSON_DESER_INVALID_INPUT    = ERROR_JSON_DESER_ERROR_0 - ArduinoJson::DeserializationError::InvalidInput;
    constexpr int ERROR_JSON_DESER_NO_MEMORY        = ERROR_JSON_DESER_ERROR_0 - ArduinoJson::DeserializationError::NoMemory;
    constexpr int ERROR_JSON_DESER_TOO_DEEP         = ERROR_JSON_DESER_ERROR_0 - ArduinoJson::DeserializationError::TooDeep;
} // namespace

#endif // __JSONERRORS_H__