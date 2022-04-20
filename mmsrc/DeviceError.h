#pragma once

#ifndef __DEVICEERROR_H__
    #define __DEVICEERROR_H__

    #define NOMINMAX
    #include <DeviceBase.h>
    #include <MMDeviceConstants.h>
    #include <functional>
    #include <string>
    #include <type_traits>

namespace rdl {

    /** Common errors */
    #define ERR_UNKNOWN_POSITION 101
    #define ERR_INITIALIZE_FAILED 102
    #define ERR_WRITE_FAILED 103
    #define ERR_CLOSE_FAILED 104
    #define ERR_FIRMWARE_NOT_FOUND 105
    #define ERR_PORT_OPEN_FAILED 106
    #define ERR_COMMUNICATION 107
    #define ERR_NO_PORT_SET 108
    #define ERR_VERSION_MISMATCH 109

    #define COMMON_ERR_MAXCODE ERR_VERSION_MISMATCH

/** 
* Initialize common error codes on a device. 
* 
* Unfortunately, CDeviceBase::SetErrorText is protected in DeviceBase.h, so we cannot use it directly. 
* Instead, we can rely on C++11 lambda functions!
*
* An example usage in a MM::Device constructor would look like this
*
* \code{.cpp}
* MyDevice::MyDevice() : initialized_(false) {
*    InitializeDefaultErrorMessages();
*    initCommonErrors("Arduino", 
*	    g_Min_MMVersion, 
*	    [this](int err, const char* txt) { 
*		    SetErrorText(err, txt); 
*	    });
*    ...
* }
* @endcode
*
* @see [C++11 Lambda Functions](http://en.cppreference.com/w/cpp/language/lambda) 
* for detailed notes on the [this] capture list semantics
* 
* @param remoteName	name of the remote device (such as "Arduino")
* @param minFirmwareVersion	minimum compatible firmware version for the remote device
* @param setErrorText C++11 lambda function that sets an error for this device
*/
    inline void initCommonErrors(const char* remoteName, long minFirmwareVersion, std::function<void(int, const char*)> setErrorText) {
        using namespace std;
        setErrorText(ERR_UNKNOWN_POSITION, "Requested position not available in this device");
        setErrorText(ERR_INITIALIZE_FAILED, "Initialization of the device failed");
        setErrorText(ERR_WRITE_FAILED, "Failed to write data to the device");
        setErrorText(ERR_CLOSE_FAILED, "Failed closing the device");
        setErrorText(ERR_FIRMWARE_NOT_FOUND, (string("Did not find the ") + remoteName + " with the correct firmware.  Is it connected to this serial port?").c_str());
        setErrorText(ERR_PORT_OPEN_FAILED, (string("Failed opening the ") + remoteName + " USB device").c_str());
        setErrorText(ERR_COMMUNICATION, (string("Problem communicating with the ") + remoteName).c_str());
        setErrorText(ERR_NO_PORT_SET, (string("Hub Device not found. The ") + remoteName + " Hub device is needed to create this device").c_str());
        setErrorText(ERR_VERSION_MISMATCH, (string("The firmware version on the ") + remoteName + " is not compatible with this adapter. Please use firmware version >= " + std::to_string(minFirmwareVersion)).c_str());
    }

    struct DeviceResultException : public std::exception {
        int error;
        const char* file;
        int line;

        DeviceResultException(int error, const char* file, int line)
            : error(error), file(file), line(line) {}

        template <typename DEV>
        inline std::string format(const DEV* device) {
            char text[MM::MaxStrLength];
            std::stringstream os;
            os << file << "(" << line << "): ";
            device->GetName(text);
            os << " device " << text;
            device->GetErrorText(error, text);
            os << " error " << error << ": " << text;
            return os.str();
        }

        inline std::string format() {
            std::stringstream os;
            os << file << "(" << line << "): ";
            os << " error " << error;
            return os.str();

        }

    };

    /** Throws an exception if the results of a standard MM device operation 
    * was not DEVICE_OK. Only works on results of type int. */
    inline void assertResult(int result, const char* file, int line) throw(DeviceResultException) {
        //static_assert(std::is_same<T, int>::value, "assertOK/assertResult only works for int results. Are you sure this is a device error code?");
        if (result != DEVICE_OK) {
            throw DeviceResultException(result, file, line);
        }
    }

    inline void assertResult(bool ok, const char* file, int line) throw(DeviceResultException) {
        //static_assert(std::is_same<T, int>::value, "assertOK/assertResult only works for int results. Are you sure this is a device error code?");
        if (!ok) {
            throw DeviceResultException(DEVICE_ERR, file, line);
        }
    }

    #ifndef ASSERT_OK
        // Must be macro to get __FILE__ and __LINE__
        #define ASSERT_OK(RET) rdl::assertResult((RET), __FILE__, __LINE__)
    #endif

}; // namespace

#endif // __DEVICEERROR_H__
