
#pragma once

#ifndef __DEVICEPROPHELPERS_H__
    #define __DEVICEPROPHELPERS_H__

    #define NOMINMAX
    //#include "DeviceBase.h"
    //#include "MMDevice.h"
    //#include <limits>
    #include <map>
    #include <string>
    #include <type_traits>

namespace dprop {

    /*******************************************************************
    * enable_if switches for MM::PropertyTypes
    *******************************************************************/
    /** type test for signed or unsigned integer that can be safely cast as long */
    template <typename T>
    struct is_mm_integral { static constexpr bool value = std::is_integral<T>::value && sizeof(T) <= sizeof(long); };

    /** type test for float or double */
    template <typename T>
    struct is_mm_floating_point { static constexpr bool value = std::is_floating_point<T>::value; };

    /** type test for reading or writing std::string */
    template <typename T>
    struct is_mm_string { static constexpr bool value = std::is_base_of<std::string, T>::value; };

    /** type test for reading null terminated strings (const char*).
	* @note we cannot use these for writing as we do not know the size of char* buffer.	*/
    template <typename T>
    struct is_mm_c_str { static constexpr bool value =
                             std::is_same<typename std::decay<T>::type, char*>::value ||
                             std::is_same<typename std::decay<T>::type, const char*>::value; };

    /** type test for combined mm assignable */
    template <typename T>
    struct is_mm_lvalue {
        static constexpr bool value = is_mm_integral<T>::value || is_mm_floating_point<T>::value || is_mm_string<T>::value;
    };

    /** type test for combined mm readable */
    template <typename T>
    struct is_mm_rvalue {
        static constexpr bool value = is_mm_integral<T>::value || is_mm_floating_point<T>::value || is_mm_string<T>::value || is_mm_c_str<T>::value;
    };

    /*******************************************************************
    * MM::PropertyType marshalling
    * 
    * Get the corresponding MM::PropertyType of a given typename.
    *******************************************************************/

    template <typename T, typename std::enable_if<is_mm_integral<T>::value, bool>::type = true>
    inline MM::PropertyType MMPropertyType_of(const T&) {
        return MM::Integer;
    }

    template <typename T, typename std::enable_if<is_mm_floating_point<T>::value, bool>::type = true>
    inline MM::PropertyType MMPropertyType_of(const T&) {
        return MM::Float;
    }

    template <typename T, typename std::enable_if<is_mm_string<T>::value, bool>::type = true>
    inline MM::PropertyType MMPropertyType_of(const T&) {
        return MM::String;
    }

    template <typename T, typename std::enable_if<is_mm_c_str<T>::value, bool>::type = true>
    inline MM::PropertyType MMPropertyType_of(const T) {
        return MM::String;
    }

    /*******************************************************************
    * mm values from strings
    *******************************************************************/
    template <class T, typename std::enable_if<is_mm_integral<T>::value, bool>::type = true>
    inline typename T Parse(const std::string& str) {
        std::stringstream parser(str);
        long temp = 0;
        parser >> temp;
        return static_cast<T>(temp);
    }

    /** Parse a floating point value from a std::string */
    template <class T, typename std::enable_if<is_mm_floating_point<T>::value, bool>::type = true>
    inline typename T Parse(const std::string& str) {
        std::stringstream parser(str);
        double temp = 0;
        parser >> temp;
        return static_cast<T>(temp);
    }

    /** Parse a string from a std::string */
    template <class T, typename std::enable_if<is_mm_string<T>::value, bool>::type = true>
    inline typename T Parse(const std::string& str) {
        return str;
    }

    /*******************************************************************
    * strings from mm values
    * 
    * dprop::to_string normally calls std::to_string for output
    * but these do some type checking and coversion first
    *******************************************************************/

    /** convert mm integral to string */
    template <class T, typename std::enable_if<is_mm_integral<T>::value, bool>::type = true>
    inline std::string ToString(const T& value) {
        return std::to_string(static_cast<long>(value));
    }

    /** convert mm floating point to string */
    template <typename T, typename std::enable_if<is_mm_floating_point<T>::value, bool>::type = true>
    inline std::string ToString(const T& value) {
        return std::to_string(static_cast<double>(value));
    }

    /** convert mm string to string (included for auto-marshalling). */
    template <typename T, typename std::enable_if<is_mm_string<T>::value, bool>::type = true>
    inline std::string ToString(const T& value) {
        return value;
    }

    /** convert const char* to std::string */
    template <typename T, typename std::enable_if<is_mm_c_str<T>::value, bool>::type = true>
    inline std::string ToString(const T value) {
        return std::string(value);
    }

    /**  convert MM::PropertyType to std::string */
    //template <typename T, typename std::enable_if<std::is_same<T, MM::PropertyType>::value, bool>::type = true>
    inline std::string ToString(const MM::PropertyType& value) {
    #define __MAP_ENTRY(type) \
        { type, #type } // stringize macro
        static std::map<MM::PropertyType, const char*> types = {
            __MAP_ENTRY(MM::Undef), __MAP_ENTRY(MM::String), __MAP_ENTRY(MM::Float), __MAP_ENTRY(MM::Integer)};
    #undef __MAP_ENTRY
        return types[value];
    }

    /** convert MM::Property value to std::string */
    inline std::string ToString(const MM::Property& prop) {
        std::string val;
        prop.Get(val);
        return val;
    }

    /*******************************************************************
    * ASSIGNMENT "OPERATORS"
    * 
    * Assign(dest, source)
    *
    * A set of assignment "operators" for setting properties from values
    * or values from properties.
    *******************************************************************/

    /*******************************************************************
    * Set Property from mm value
    *******************************************************************/

    /** assign MM::Property from mm integral */
    template <typename T, typename std::enable_if<is_mm_integral<T>::value, bool>::type = true>
    inline bool Assign(MM::PropertyBase& prop, const T& value) {
        long temp = static_cast<long>(value);
        return prop.Set(temp);
    }

    /** assign MM::Property from mm floating point */
    template <typename T, typename std::enable_if<is_mm_floating_point<T>::value, bool>::type = true>
    inline bool Assign(MM::PropertyBase& prop, const T& value) {
        double temp = static_cast<double>(value);
        return prop.Set(temp);
    }

    /** assign MM::Property from mm string */
    template <typename T, typename std::enable_if<is_mm_string<T>::value, bool>::type = true>
    inline bool Assign(MM::PropertyBase& prop, const T& value) {
        return prop.Set(value.c_str());
    }

    /** assign MM::Property from const char* */
    template <typename T, typename std::enable_if<is_mm_c_str<T>::value, bool>::type = true>
    inline bool Assign(MM::PropertyBase& prop, const T value) {
        return prop.Set(value);
    }

    /*******************************************************************
    * Set mm value from Property
    *******************************************************************/

    /** assign mm integral from MM::Property */
    template <typename T, typename std::enable_if<is_mm_integral<T>::value, bool>::type = true>
    inline bool Assign(T& value, const MM::PropertyBase& prop) {
        long temp;
        bool ret = prop.Get(temp);
        value    = static_cast<T>(temp);
        return ret;
    }

    /** assign mm floating point from MM::Property */
    template <typename T, typename std::enable_if<is_mm_floating_point<T>::value, bool>::type = true>
    inline bool Assign(T& value, const MM::PropertyBase& prop) {
        double temp;
        bool ret = prop.Get(temp);
        value    = static_cast<T>(temp);
        return ret;
    }

    /** assign mm string from MM::Property */
    template <typename T, typename std::enable_if<is_mm_string<T>::value, bool>::type = true>
    inline bool Assign(T& value, const MM::PropertyBase& prop) {
        return prop.Get(value);
    }

    /*******************************************************************
    * Set a device property from a name and a value
    *******************************************************************/

    /**
    * Set a property by name for a given typename T on a given device DEV.
    * 
    * Devices can only set a property by name and a string value.
    * This method marshals the property value to a string and
    * calls the DEV->SetProperty(propname, stringvalue).
	*/
    template <typename T, class DEV, typename std::enable_if<is_mm_rvalue<T>::value, bool>::type = true>
    inline int Assign(DEV* device, const char* propName, const T& value) {
        return device->SetProperty(propName, ToString<T>(value).c_str());
    }

    /*******************************************************************
    * Get a device property from a name and a value
    *******************************************************************/

    /**
    * Get a property by name for a given typename T on a given device DEV.
    * 
    * Devices can get properties by value. These methods marshal the
	* type T to the proper device->GetProperty(name, val) call.
	*/
    template <typename T, class DEV, typename std::enable_if<is_mm_integral<T>::value, bool>::type = true>
    inline int Assign(T& value, DEV* device, const char* propName) {
        long temp;
        int ret;
        if ((ret = device->GetProperty(propName, temp)) != DEVICE_OK) {
            return ret;
        }
        value = static_cast<T>(temp);
        return ret;
    }

    /**
    * Get a property by name for a given typename T on a given device DEV.
	* Devices can get properties by value. These methods marshal the
	* type T to the proper device->GetProperty(name, val) call.
	*/
    template <typename T, class DEV, typename std::enable_if<is_mm_floating_point<T>::value, bool>::type = true>
    inline int Assign(T& value, DEV* device, const char* propName) {
        double temp;
        int ret;
        if ((ret = device->GetProperty(propName, temp)) != DEVICE_OK) {
            return ret;
        }
        value = static_cast<T>(temp);
        return ret;
    }

    /**
    * Get a string property by name for a given typename T on a given device DEV.
    *
    * Getting string device properties is a little unsafe in Micromanager. You can
	* only get one by passing a char* buffer to the GetProperty method. This means
	* the buffer must be big enough. We will use the constant MM::MaxStrLength
	* to define the size of an intermediate buffer, temporarily create a buffer in 
	* free memory (instead of the stack), get the property, then copy the buffer to 
	* a string and free the buffer.
	*/
    template <typename T, class DEV, typename std::enable_if<is_mm_string<T>::value, bool>::type = true>
    inline int Assign(T& value, DEV* device, const char* propName) {
        char* resBuf = new char[MM::MaxStrLength];
        int ret      = device->GetProperty(propName, resBuf);
        if (ret == DEVICE_OK) {
            value.assign(resBuf);
        }
        delete resBuf;
        return ret;
    }

}; // namespace dprop

#endif // __DEVICEPROPHELPERS_H__
