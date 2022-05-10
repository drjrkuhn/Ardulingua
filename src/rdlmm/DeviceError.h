#pragma once

#ifndef __DEVICEERROR_H__
    #define __DEVICEERROR_H__

    #include "../rdl/JsonError.h"
    #include "../rdl/sys_StringT.h"
    #define NOMINMAX
    #include <DeviceBase.h>
    #include <MMDeviceConstants.h>
    #include <functional>
    #include <string>
    #include <type_traits>

namespace rdlmm {

    /** Common errors */
    constexpr int ERR_UNKNOWN_POSITION   = 101;
    constexpr int ERR_INITIALIZE_FAILED  = 102;
    constexpr int ERR_WRITE_FAILED       = 103;
    constexpr int ERR_CLOSE_FAILED       = 104;
    constexpr int ERR_FIRMWARE_NOT_FOUND = 105;
    constexpr int ERR_PORT_OPEN_FAILED   = 106;
    constexpr int ERR_COMMUNICATION      = 107;
    constexpr int ERR_NO_PORT_SET        = 108;
    constexpr int ERR_VERSION_MISMATCH   = 109;
    // Devices should define custom error codes after this
    constexpr int COMMON_ERR_MAXCODE = ERR_VERSION_MISMATCH;

    namespace svc {
        template <class DeviceT>
        struct deverr_accessor : public DeviceT {
            static void SetErrorText(DeviceT* target, int errorCode, const char* text) {
                void (DeviceT::*fn)(int, const char*) = &DeviceT::SetErrorText;
                (target->*fn)(errorCode, text);
            }
        };
    }
    /** 
    * Initialize common error codes on a device. 
    * 
    * CDeviceBase::SetErrorText is protected in DeviceBase.h, so we cannot use it directly. 
    * We define a private accessor class to access it.
    *
    * @tparam DeviceT   class of hub device
    * @param hub        hub device to use
    * @param remoteName	name of the remote device (such as "Arduino")
    * @param minFirmwareVersion	minimum compatible firmware version for the remote device
    */
    template <class DeviceT>
    inline void InitCommonErrors(DeviceT* hub, const char* remoteName, long minFirmwareVersion) {
        using namespace sys;
        using namespace rdl;
        using namespace svc;
        using acc = deverr_accessor<DeviceT>;
        acc::SetErrorText(
            hub, ERR_UNKNOWN_POSITION,
            "Requested position not available in this device");
        acc::SetErrorText(
            hub, ERR_INITIALIZE_FAILED,
            "Initialization of the device failed");
        acc::SetErrorText(
            hub, ERR_WRITE_FAILED,
            "Failed to write data to the device");
        acc::SetErrorText(
            hub, ERR_CLOSE_FAILED,
            "Failed closing the device");
        acc::SetErrorText(
            hub, ERR_FIRMWARE_NOT_FOUND,
            (StringT("Did not find the ") + remoteName + " with the correct firmware.  Is it connected to this serial port_impl?").c_str());
        acc::SetErrorText(
            hub, ERR_PORT_OPEN_FAILED,
            (StringT("Failed opening the ") + remoteName + " USB device").c_str());
        acc::SetErrorText(
            hub, ERR_COMMUNICATION,
            (StringT("Problem communicating with the ") + remoteName).c_str());
        acc::SetErrorText(
            hub, ERR_NO_PORT_SET,
            (StringT("Hub Device not found. The ") + remoteName + " Hub device is needed to create this device").c_str());
        acc::SetErrorText(
            hub, ERR_VERSION_MISMATCH,
            (StringT("The firmware version on the ") + remoteName + " is not compatible with this adapter. Please use firmware version >= " + std::to_string(minFirmwareVersion)).c_str());

        acc::SetErrorText(
            hub, ERROR_JSON_PARSE_ERROR,
            "JSON parse error");
        acc::SetErrorText(
            hub, ERROR_JSON_INVALID_REQUEST,
            "JSON invalid request");
        acc::SetErrorText(
            hub, ERROR_JSON_METHOD_NOT_FOUND,
            "JSON method not found");
        acc::SetErrorText(
            hub, ERROR_JSON_INVALID_PARAMS,
            "JSON invalid parameters");
        acc::SetErrorText(
            hub, ERROR_JSON_INTERNAL_ERROR,
            "JSON internal error");

        acc::SetErrorText(
            hub, ERROR_JSON_RET_NOT_SET,
            "JSON Return not set");
        acc::SetErrorText(
            hub, ERROR_JSON_ENCODING_ERROR,
            "JSON encoding error");
        acc::SetErrorText(
            hub, ERROR_JSON_SEND_ERROR,
            "JSON send error");
        acc::SetErrorText(
            hub, ERROR_JSON_TIMEOUT,
            "JSON timout");
        acc::SetErrorText(
            hub, ERROR_JSON_NO_REPLY,
            "JSON no reply");
        acc::SetErrorText(
            hub, ERROR_JSON_INVALID_REPLY,
            "JSON invalid reply");
        acc::SetErrorText(
            hub, ERROR_SLIP_ENCODING_ERROR,
            "SLIP encoding error");
        acc::SetErrorText(
            hub, ERROR_SLIP_DECODING_ERROR,
            "SLIP decoding error");

        acc::SetErrorText(
            hub, ERROR_JSON_DESER_EMPTY_INPUT,
            "JSON deserialize empty input");
        acc::SetErrorText(
            hub, ERROR_JSON_DESER_INCOMPLETE_INPUT,
            "JSON deserialize incomplete input");
        acc::SetErrorText(
            hub, ERROR_JSON_DESER_INVALID_INPUT,
            "JSON deserialize invalid input");
        acc::SetErrorText(
            hub, ERROR_JSON_DESER_NO_MEMORY,
            "JSON deserialize no memory");
        acc::SetErrorText(
            hub, ERROR_JSON_DESER_TOO_DEEP,
            "JSON deserialize too deep");
    }

    /**
     * MM Device exception for try/catch/finally in device drivers.
     * 
    * CMMError, the MM error exception, is only found in the MMCore. It is not really
    * available to device. This class defines a new, parallel exception type for our own
    * devices.
     * 
     */
    struct DeviceResultException : public std::exception {
        int error;
        const char* file;
        int line;

        DeviceResultException(int error, const char* file, int line)
            : error(error), file(file), line(line) {}

        template <typename DeviceT>
        inline sys::StringT format(const DeviceT* device) {
            char text[MM::MaxStrLength];
            std::stringstream os;
            os << file << "(" << line << "): ";
            device->GetName(text);
            os << " device " << text;
            device->GetErrorText(error, text);
            os << " error " << error << ": " << text;
            return os.str();
        }

        inline sys::StringT format() {
            std::stringstream os;
            os << file << "(" << line << "): ";
            os << " error " << error;
            return os.str();
        }
    };

    /** 
    * Throws an exception if the results of a standard MM device operation 
    * was not DEVICE_OK. Only works on results of type int. 
    */
    inline void assertResult(int result, const char* file, int line) throw(DeviceResultException) {
        //static_assert(std::is_same<T, int>::value, "assertOK/assertResult only works for int results. Are you sure this is a device error code?");
        if (result != DEVICE_OK) {
            throw DeviceResultException(result, file, line);
        }
    }

    #ifndef ASSERT_OK
        // Must be macro to get __FILE__ and __LINE__
        #define ASSERT_OK(RET) rdlmm::assertResult((RET), __FILE__, __LINE__)
    #endif

    #ifndef ASSERT_TRUE
        // Must be macro to get __FILE__ and __LINE__
        #define ASSERT_TRUE(COND, ERROR)                          \
            if (!(COND)) {                                        \
                rdlmm::assertResult((ERROR), __FILE__, __LINE__); \
            }
    #endif

    #ifndef THROW_DEVICE_ERROR
        #define THROW_DEVICE_ERROR(ERR) throw rdlmm::DeviceResultException(ERR, __FILE__, __LINE__);
    #endif

}; // namespace

#endif // __DEVICEERROR_H__
