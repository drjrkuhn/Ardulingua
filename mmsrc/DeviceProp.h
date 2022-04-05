#pragma once

#ifndef __DEVICE_PROP__
    #define __DEVICE_PROP__

#define NOMINMAX
    #include <DeviceBase.h>
    #include "DevicePropHelpers.h"
    //#include "MMDevice.h"
    #include <ostream>
    #include <iomanip>
    #include <limits>
    #include <type_traits>

namespace dprop {

    /*******************************************************************
    * PropInfo
    *******************************************************************/

    template <typename T>
    class PropInfo {
     public:
        /*******************************************************************
        * Setters
        *******************************************************************/

        /** Factory method to creating a PropInfo. This templated version is neccessary to catch potential problems such as
		 * @code{.cpp}PropInfo<std::string> build("foo",0);@endcode which tries to inialize a std::string from a null pointer. */
        template <typename U,
                  typename std::enable_if<std::is_convertible<U, T>::value, bool>::type = true>
        static PropInfo build(const char* name, const U& initialValue) { return PropInfo(name, static_cast<const T>(initialValue)); }

        /** Add min and max value limits to the PropInfo. Sets hasLimits to true. */
        PropInfo& withLimits(double minval, double maxval) {
            minValue_  = minval;
            maxValue_  = maxval;
            hasLimits_ = true;
            return *this;
        }

        /** Add a single allowed value to the PropInfo. */
        PropInfo& withAllowedValue(const T& val) {
            allowedValues_.push_back(val);
            return *this;
        }

        /** Add an array of allowed values to the PropInfo using an initializer list.
		 * For example: @code{.cpp} PropInfo<int>build("foo",1).withAllowedValues({1, 2, 3, 4});@endcode */
        PropInfo& withAllowedValues(const std::initializer_list<T>& vals) {
            allowedValues_.insert(allowedValues_.end(), vals.begin(), vals.end());
            return *this;
        }

        /** Add several allowed values to the PropInfo from a vector. */
        PropInfo& withAllowedValues(const std::vector<T>& vals) {
            allowedValues_.insert(allowedValues_.end(), vals.begin(), vals.end());
            return *this;
        }

        /** Specify this is a pre-init property. */
        PropInfo& withIsPreInit() {
            isPreInit_ = true;
            return *this;
        }

        /** Check that this property is read-only upon creation. It is mainly used as a double-check during createProperty. */
        PropInfo& assertReadOnly() {
            assertIsReadOnly_ = true;
            return *this;
        }

        /*******************************************************************
        * Getters
        *******************************************************************/
        /** Get the property name. */
        const char* name() const { return name_; }

        /** Get the initial property value. */
        T initialValue() const { return initialValue_; }

        /** Has the withLimits() been added? */
        bool hasLimits() const { return hasLimits_; }

        /** minimum limit value. */
        double minValue() const { return minValue_; }

        /** maximum limit value. */
        double maxValue() const { return maxValue_; }

        /** has one of the withAllowedValues() been added? */
        bool hasAllowedValues() const { return !allowedValues_.empty(); }

        /** get all of the allowed values. */
        std::vector<T> allowedValues() const { return allowedValues_; }

        /** Was this specified as a pre-init property? */
        bool isPreInit() const { return isPreInit_; }

        /** Was this specified as a read-only property? */
        bool isAssertReadOnly() const { return assertIsReadOnly_; }

        friend std::ostream& operator<< (std::ostream& os, const PropInfo& p) {
            os << "PropInfo.name=" << p.name_;
            os << " .initialValue=" << p.initialValue;
            os << " .hasLimits=" << std::boolalpha << p.hasLimits;
            os << " .minValue=" << p.minValue_;
            os << " .maxValue=" << p.maxValue_;
            os << " .isPreInit=" << std::boolalpha << p.isPreInit_;
            os << " .assertIsReadOnly=" << std::boolalpha << p.assertIsReadOnly_;
            os << " .allowedValues={";
            std::copy(p.allowedValues.begin(), p.alowedValue.end(), std::ostream_iterator<T>(os, " "));
            os << "\b}";
            return os;
        }

     protected:
        PropInfo(const char* name, const T& initialValue) : name_(name), initialValue_(initialValue) {}

        const char* name_;
        T initialValue_;
        bool hasLimits_        = false;
        double minValue_       = 0;
        double maxValue_       = 0;
        bool isPreInit_        = false;
        bool assertIsReadOnly_ = false;
        std::vector<T> allowedValues_;
    };


    /*******************************************************************
    * DeviceProp_Base
    *******************************************************************/

    /**
	* A class to hold and update a micromanager property.
    *
	* Devices should not create a DeviceProp_Base directly, but should create one of its derived members and then
	* call createDeviceProp. Micromanager updates the property through the OnExecute member function.
    *
	* The T template parameter should contain the type of the member property.The type T should be able
	* to auto-box and -unbox (auto-cast) from either MMInteger, MMFloat, or MMString.
	* For example, if T=char, int, or long, then the property will be designated MM::Integer
	* The DEV template parameter holds the device type
	*/

    template <typename T, class DEV>
    class DeviceProp_Base {
     public:
        typedef int (DEV::*NotifyChangeFunction)(const char* propName, const char* propValue);

        virtual ~DeviceProp_Base() {}
        const T& getCachedValue() { return cachedValue_; }

        void setNotifyChange(NotifyChangeFunction& notifyChangeFunc) { notifyChangeFunc_ = notifyChangeFunc; }

        /** Sets the Device Property, which updates the getCachedValue */
        int setProperty(const T& val) {
            int ret;
            if ((ret = SetDeviceProp(device_, name_, val)) != DEVICE_OK) {
                return ret;
            };
            return notifyChangeH(val);
        }

        int getProperty(T& val) const { return Assign(val, device_, name_); };
        const char* name() const { return name_; }

     protected:
        DeviceProp_Base() {}

        T cachedValue_;
        DEV* device_                           = nullptr;
        const char* name_                      = nullptr;
        NotifyChangeFunction notifyChangeFunc_ = nullptr;

        int notifyChangeH(const T& val) {
            // TODO: Combine
            std::string sval = ToString<T>(val);
            if (notifyChangeFunc_) { return (device_->*notifyChangeFunc_)(name_, sval.c_str()); }
            return DEVICE_OK;
        }

        /** Link the property to the device and initialize from the propInfo. */
        int createDevicePropH(DEV* device, const PropInfo<T>& propInfo,
                              MM::ActionFunctor* action, bool readOnly, bool useInitialValue) {
            assert(device != nullptr);
            device_ = device;
            name_   = propInfo.name();
            if (useInitialValue) { cachedValue_ = propInfo.initialValue(); }
            return createDeviceProp(device_, propInfo, cachedValue_, action, readOnly);
        }

        /** Called by the properties update method. Subclasses must override. */
        virtual int OnExecute(MM::PropertyBase* __pProp, MM::ActionType __pAct) = 0;
    };

    /*******************************************************************
    * Device Property Factory Function
    * 
    * Two main version: 
    * - one based on a MM Action
    * - the other creates an MM Action from a device method
    *******************************************************************/

    /**
     * Creates a property that calls an update action on device from the propInfo. 
     * The initial val is gien as a parameter.
     */
    template <typename T, class DEV>
    int createDeviceProp(DEV* device, const PropInfo<T>& propInfo, T initialValue,
                         MM::ActionFunctor* action, bool readOnly = false) {

        // double-check the read-only flag if the propInfo was created with the assertReadOnly() flag
        if (propInfo.isAssertReadOnly() && !readOnly) {
            assert(propInfo.isAssertReadOnly() == readOnly);
            // Create an "ERROR" property if assert was turned off during compile.
            device->CreateProperty(propInfo.name(), "CreateProperty ERROR: read-write property did not assertReadOnly", MM::String, true);
            return DEVICE_INVALID_PROPERTY;
        }
        std::string sval = ToString<T>(initialValue);
        int ret          = device->CreateProperty(propInfo.name(), sval.c_str(), MMPropertyType_of<T>(initialValue), readOnly, action, propInfo.isPreInit());
        if (ret != DEVICE_OK) { return ret; }
        if (propInfo.hasLimits()) { ret = device->SetPropertyLimits(propInfo.name(), propInfo.minValue(), propInfo.maxValue()); }
        if (propInfo.hasAllowedValues()) {
            std::vector<std::string> allowedStrings;
            for (T aval : propInfo.allowedValues()) {
                allowedStrings.push_back(ToString<T>(aval));
            }
            device->SetAllowedValues(propInfo.name(), allowedStrings);
        }
        return ret;
    };

    /**
    * Creates a property that calls an update function on device from the propInfo. 
    * The initial value is given as a parameter.
    * 
    * This version requires a device member function of the form
	* int OnProperty(MM::PropertyBase* pPropt, MM::ActionType eAct) The initial value is taken from propInfo.
    */
    template <typename T, class DEV>
    int createDeviceProp(DEV* device, const PropInfo<T>& propInfo, const T& initialValue,
                         int (DEV::*fn)(MM::PropertyBase* pPropt, MM::ActionType eAct), bool readOnly = false) {
        MM::Action<DEV>* action = nullptr;
        if (fn != nullptr) { action = new MM::Action<DEV>(device, fn); }
        return createDeviceProp(device, propInfo, initialValue, action, readOnly);
    };

    /** Creates a property that calls an update function on device from the propInfo.
    * 
	* Create MM device properties from PropInfo information. This version requires a device member function of the form
	* int OnProperty(MM::PropertyBase* pPropt, MM::ActionType eAct) The initial value is taken from propInfo.
	*/
    template <typename T, class DEV>
    int createDeviceProp(DEV* device, const PropInfo<T>& propInfo,
                         int (DEV::*fn)(MM::PropertyBase* pPropt, MM::ActionType eAct), bool readOnly = false) {
        return createDeviceProp(device, propInfo, propInfo.initialValue(), fn, readOnly);
    }

}; // namespace dprop

#endif // __DEVICE_PROP__
