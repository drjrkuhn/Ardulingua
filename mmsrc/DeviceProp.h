#pragma once

#ifndef __DEVICE_PROP__
    #define __DEVICE_PROP__

    #define NOMINMAX
    #include "DevicePropHelpers.h"
    #include <DeviceBase.h>
    //#include "MMDevice.h"
    #include <iomanip>
    #include <limits>
    #include <ostream>
    #include <type_traits>

namespace rdl {

    /*******************************************************************
     * PropInfo builder
     *******************************************************************/

    template <typename PropT>
    class PropInfo {
     public:
        /*******************************************************************
         * Setters
         *******************************************************************/

        /** Factory method to creating a PropInfo. This templated version is neccessary to catch potential problems such as
         * @code{.cpp}PropInfo<std::string> build("foo",0);@endcode which tries to inialize a std::string from a null pointer. */
        template <typename U,
                  typename std::enable_if<std::is_convertible<U, PropT>::value, bool>::type = true>
        static PropInfo build(const char* name, const U& initialValue) {
            return PropInfo(name, static_cast<const PropT>(initialValue));
        }

        /** Add brief name (only for remote properties). */
        PropInfo& withBriefName(const char* briefName) {
            briefName_ = briefName;
            return *this;
        }

        /** Add min and max value limits to the PropInfo. Sets hasLimits to true. */
        PropInfo& withLimits(double minval, double maxval) {
            minValue_  = minval;
            maxValue_  = maxval;
            hasLimits_ = true;
            return *this;
        }

        /** Add a single allowed value to the PropInfo. */
        PropInfo& withAllowedValue(const PropT& val) {
            allowedValues_.push_back(val);
            return *this;
        }

        /** Add an array of allowed values to the PropInfo using an initializer list.
         * For example: @code{.cpp} PropInfo<int>build("foo",1).withAllowedValues({1, 2, 3, 4});@endcode */
        PropInfo& withAllowedValues(const std::initializer_list<PropT>& vals) {
            allowedValues_.insert(allowedValues_.end(), vals.begin(), vals.end());
            return *this;
        }

        /** Add several allowed values to the PropInfo from a vector. */
        PropInfo& withAllowedValues(const std::vector<PropT>& vals) {
            allowedValues_.insert(allowedValues_.end(), vals.begin(), vals.end());
            return *this;
        }

        /** Specify this is a pre-init property. */
        PropInfo& withIsPreInit() {
            isPreInit_ = true;
            return *this;
        }

        /** Specify this is a sequencable property (only for remote properties). */
        PropInfo& withIsSequencable() {
            isSequencable_ = true;
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

        /** Get the property brief name (for remote). */
        const char* briefName() const { return briefName_; }

        /** Get the initial property value. */
        PropT initialValue() const { return initialValue_; }

        /** Has the withLimits() been added? */
        bool hasLimits() const { return hasLimits_; }

        /** minimum limit value. */
        double minValue() const { return minValue_; }

        /** maximum limit value. */
        double maxValue() const { return maxValue_; }

        /** has one of the withAllowedValues() been added? */
        bool hasAllowedValues() const { return !allowedValues_.empty(); }

        /** get all of the allowed values. */
        std::vector<PropT> allowedValues() const { return allowedValues_; }

        /** Was this specified as a pre-init property? */
        bool isPreInit() const { return isPreInit_; }

        /** Was this specified as a sequencable property? */
        bool isSequencable() const { return isSequencable_; }

        /** Was this specified as a read-only property? */
        bool isReadOnly() const { return assertIsReadOnly_; }

        friend std::ostream& operator<<(std::ostream& os, const PropInfo& p) {
            os << "PropInfo.name=" << p.name_;
            os << " .briefName=" << p.briefName_;
            os << " .initialValue=" << p.initialValue;
            os << " .hasLimits=" << std::boolalpha << p.hasLimits;
            os << " .minValue=" << p.minValue_;
            os << " .maxValue=" << p.maxValue_;
            os << " .isPreInit=" << std::boolalpha << p.isPreInit_;
            os << " .isSequencable=" << std::boolalpha << p.isSequencable_;
            os << " .assertIsReadOnly=" << std::boolalpha << p.assertIsReadOnly_;
            os << " .allowedValues={";
            std::copy(p.allowedValues.begin(), p.alowedValue.end(), std::ostream_iterator<PropT>(os, " "));
            os << "\b}";
            return os;
        }

     protected:
        PropInfo(const char* name, const PropT& initialValue) : name_(name), initialValue_(initialValue) {}

        const char* name_;
        const char* briefName_ = nullptr;
        PropT initialValue_;
        bool hasLimits_        = false;
        double minValue_       = 0;
        double maxValue_       = 0;
        bool isSequencable_    = false;
        bool isPreInit_        = false;
        bool assertIsReadOnly_ = false;
        std::vector<PropT> allowedValues_;
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
     * The PropT template parameter should contain the type of the member property.The type PropT should be able
     * to auto-box and -unbox (auto-cast) from either MMInteger, MMFloat, or MMString.
     * For example, if PropT=char, int, or long, then the property will be designated MM::Integer
     * The DeviceT template parameter holds the device type
     */

    template <typename PropT, class DeviceT>
    class DeviceProp_Base {
     public:
        typedef int (DeviceT::*NotifyChangeFnT)(const char* propName, const char* propValue);

        /** Add a callback method to the property */
        void setNotifyChange(NotifyChangeFnT& notifyChangeFunc) {
            notifyChangeFunc_ = notifyChangeFunc;
        }

        /** Sets the Device Property, which updates the getCachedValue */
        virtual int set(const PropT& value) {
            int ret;
            if ((ret = Assign(device_, name_.c_str(), value)) != DEVICE_OK) return ret;
            cachedValue_ = value;
            return notifyChange(value);
        }

        /** Get the device property directly */
        virtual int get(PropT& val) const { return Assign(val, device_, name_.c_str()); };

        /** Get the cached property value (last call to set()) */
        virtual int getCached(PropT& value) const {
            value = cachedValue_;
            return DEVICE_OK;
        }
        /** Get the property name */
        const std::string name() const { return name_; }

        /** Get the property brief name (for remote) */
        const std::string briefName() const { return briefName_; }

        /** Get the read-only flag */
        bool isReadOnly() const { return readOnly_; }

        bool isSequencable() const { return sequencable_; }

        /**
         * Get the Device that owns the property (hub or sub-device).
         * The owner device does the actual setting and getting of property values!
         */
        DeviceT* owner() const { return device_; }

     protected:
        /** Protected constructor - only called by derived classes */
        DeviceProp_Base() : cachedValue_(), readOnly_(false), device_(nullptr), name_(""), notifyChangeFunc_(nullptr) {}

        /** Call the registered callback if any */
        virtual int notifyChange(const PropT& val) {
            if (notifyChangeFunc_) {
                return (device_->*notifyChangeFunc_)(name_.c_str(), ToString(val).c_str());
            }
            return DEVICE_OK;
        }

        /** Link the property to the device and initialize from the propInfo. */
        int createAndLinkProp(DeviceT* device,
                              const PropInfo<PropT>& propInfo,
                              MM::ActionFunctor* action,
                              bool readOnly,
                              bool useInfoInitialValue) {
            assert(device != nullptr);
            device_      = device;
            name_        = propInfo.name();
            briefName_   = propInfo.briefName() ? propInfo.briefName() : std::string("");
            readOnly_    = readOnly;
            sequencable_ = propInfo.isReadOnly();
            if (useInfoInitialValue) {
                cachedValue_ = propInfo.initialValue();
            }

            // double-check the read-only flag if the propInfo was created with the assertReadOnly() flag
            if (propInfo.isReadOnly() && !readOnly) {
                assert(propInfo.isReadOnly() == readOnly);
                // Create an "ERROR" property if assert was turned off during compile.
                device->CreateProperty(propInfo.name(), "CreateProperty ERROR: read-write property did not assertReadOnly", MM::String, true);
                return DEVICE_INVALID_PROPERTY;
            }
            int ret = device_->CreateProperty(name_.c_str(), ToString(cachedValue_).c_str(),
                                              MMPropertyType_of<PropT>(cachedValue_),
                                              readOnly, action, propInfo.isPreInit());
            if (ret != DEVICE_OK) {
                return ret;
            }
            if (propInfo.hasLimits()) {
                ret = device->SetPropertyLimits(propInfo.name(), propInfo.minValue(), propInfo.maxValue());
            }
            if (propInfo.hasAllowedValues()) {
                std::vector<std::string> allowedStrings;
                for (PropT aval : propInfo.allowedValues()) {
                    allowedStrings.push_back(ToString(aval));
                }
                device->SetAllowedValues(propInfo.name(), allowedStrings);
            }
            return ret;
        }

        /** Called by the properties update method. Subclasses must override. */
        virtual int OnExecute(MM::PropertyBase* __pProp, MM::ActionType __pAct) = 0;

#if 0
     private:
        /*******************************************************************
        * Create underlying MM::Property on device
        * 
        * TODO: Just integrate createMMPropOnDevice into createAndLinkProp
        *       and remove these
        *
        * - one based on a MM Action 
        * - the other creates an MM Action from a device method
        ******************************************************************* /

        /** Creates a property that calls an update function on device from the propInfo.
         *
         * The initial value is given as a parameter.
         * This version requires a device member function of the form
         * int OnProperty(MM::PropertyBase* pPropt, MM::ActionType eAct) The initial value is taken from propInfo.
         */
        template <typename PropT, class DeviceT>
        static inline int createMMPropOnDevice(DeviceT* device,
                                               const PropInfo<PropT>& propInfo,
                                               const PropT& initialValue,
                                               int (DeviceT::*fn)(MM::PropertyBase* pPropt, MM::ActionType eAct),
                                               bool readOnly = false) {
            MM::Action<DeviceT>* action = fn ? new MM::Action<DeviceT>(device, fn) : nullptr;
            return createMMPropOnDevice(device, propInfo, initialValue, action, readOnly);
        };

        ///** Creates a property Without initial value that calls an update function on device from the propInfo.
        // *
        // * Create MM device properties from PropInfo information. This version requires a device member function of the form
        // * int OnProperty(MM::PropertyBase* pPropt, MM::ActionType eAct) The initial value is taken from propInfo.
        // */
        //template <typename PropT, class DeviceT>
        //static inline int createMMPropOnDevice(DeviceT* device,
        //                                       const PropInfo<PropT>& propInfo,
        //                                       int (DeviceT::*fn)(MM::PropertyBase* pPropt, MM::ActionType eAct),
        //                                       bool readOnly = false) {
        //    return createMMPropOnDevice(device, propInfo, propInfo.initialValue(), fn, readOnly);
        //}

        /** Creates a property that calls an update action on device from the propInfo.
         *
         * ### THIS IS THE MAIN PROPERTY CREATION ENTRY POINT
         *
         * The initial val is given as a parameter.
         */
        template <typename PropT, class DeviceT>
        static inline int createMMPropOnDevice(DeviceT* device,
                                               const PropInfo<PropT>& propInfo,
                                               PropT initialValue,
                                               MM::ActionFunctor* action,
                                               bool readOnly = false) {
            // double-check the read-only flag if the propInfo was created with the assertReadOnly() flag
            if (propInfo.isReadOnly() && !readOnly) {
                assert(propInfo.isReadOnly() == readOnly);
                // Create an "ERROR" property if assert was turned off during compile.
                device->CreateProperty(propInfo.name(), "CreateProperty ERROR: read-write property did not assertReadOnly", MM::String, true);
                return DEVICE_INVALID_PROPERTY;
            }
            int ret = device->CreateProperty(propInfo.name(),
                                             ToString(initialValue).c_str(),
                                             MMPropertyType_of<PropT>(initialValue),
                                             readOnly,
                                             action,
                                             propInfo.isPreInit());
            if (ret != DEVICE_OK) {
                return ret;
            }
            if (propInfo.hasLimits()) {
                ret = device->SetPropertyLimits(propInfo.name(), propInfo.minValue(), propInfo.maxValue());
            }
            if (propInfo.hasAllowedValues()) {
                std::vector<std::string> allowedStrings;
                for (PropT aval : propInfo.allowedValues()) {
                    allowedStrings.push_back(ToString(aval));
                }
                device->SetAllowedValues(propInfo.name(), allowedStrings);
            }
            return ret;
        };
#endif


     protected:
        PropT cachedValue_;
        bool readOnly_;
        bool sequencable_;
        DeviceT* device_;
        std::string name_;
        std::string briefName_;
        NotifyChangeFnT notifyChangeFunc_;
    };

}; // namespace aling

#endif // __DEVICE_PROP__
