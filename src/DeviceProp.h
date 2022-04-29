#pragma once

#ifndef __DEVICE_PROP__
    #define __DEVICE_PROP__

    #define NOMINMAX
    #include "DevicePropHelpers.h"
    #include "PropInfo.h"
    #include <DeviceBase.h>
    #include <iomanip>
    #include <limits>

namespace rdl {

    /*******************************************************************
     * DeviceProp_Base
     *******************************************************************/

    /**
     * A class to hold and update a micromanager property.
     *
     * For simple (i.e. local) properties, the `cachedValue_` member
     * should be treated as the "remote" value. This is our local
     * copy of the data held in the MM::Property storage.
     * 
     * Devices should not create a DeviceProp_Base directly, but should create one of its derived members and then
     * call createDeviceProp. Micromanager updates the property through the OnExecute member function.
     *
     * The PropT template parameter should contain the type of the member property.The type PropT should be able
     * to auto-box and -unbox (auto-cast) from either MMInteger, MMFloat, or MMString.
     * For example, if PropT=char, int, or long, then the property will be designated MM::Integer
     * The DeviceT template parameter holds the device type
     */

    template <class DeviceT, typename PropT>
    class DeviceProp_Base {
     public:
        typedef int (DeviceT::*NotifyChangeFnT)(const char* propName, const char* propValue);

        /** Add a callback method to the property */
        void setNotifyChange(NotifyChangeFnT& notifyChangeFunc) { notifyChangeFunc_ = notifyChangeFunc; }

        /** Get the property name */
        const std::string name() const { return name_; }

        /** Get the property brief name (for remote) */
        const std::string brief() const { return brief_; }

        /** Get the read-only flag */
        bool isReadOnly() const { return isReadOnly; }

        /** Get the sequencable flag */
        bool isSequencable() const { return isSequencable_; }

        /**
         * Set both the internal property value and the property store value directly.
         *   > NOTE: This method does NOT check the read-only flag of the property!
         *   > The read-only flag is meant primarily to prevent the GUI/user from
         *   > changing properties. The driver itself might need to update a read-only
         *   > status property. This method allows drivers to update a read-only
         *   > property behind the scenes.
         */
        virtual int SetProperty(const PropT value) {
            int ret;
            if ((ret = set_impl(value)) != DEVICE_OK) {
                return ret;
            }
            if ((ret = Assign(device_, name_.c_str(), value)) != DEVICE_OK) {
                return ret;
            }
            return notifyChange(value);
        }

        /** Get the internal property value directly (NOT the property store value). */
        virtual int GetProperty(PropT& value) { return get_impl(value); }

        /** Get the locally cached property value directly (NOT the property store value). */
        virtual int GetCachedProperty(PropT& value) { return getCached_impl(value); }

        /**
         * Get the Device that owns the property (hub or sub-device).
         * The owner device does the actual setting and getting of property values!
         */
        DeviceT* owner() const { return device_; }

     protected:
        /** Called by the properties update method. Subclasses must override. */
        virtual int OnExecute(MM::PropertyBase* __pProp, MM::ActionType __pAct) = 0;

        /**
         * Set this internal property value. Should update cachedValue_.
         * This does NOT read the underlying MM Property store for the
         * value to set via DeviceBase::GetProperty(name,value. 
         * That should happen during OnExecute
         */
        virtual int set_impl(const PropT value) = 0;

        /**
         * Get the internal device property directly.
         * This does NOT update the underlying MM Property store via
         * DeviceBase::SetProperty(name,value). That should happen during OnExecute
         */
        virtual int get_impl(PropT& val) = 0;

        /**
         * Get the inernal cached property value (last call to set()).
         * This does NOT update the underlying MM Property store via
         * DeviceBase::SetProperty(name,value). That should happen during OnExecute
         */
        virtual int getCached_impl(PropT& value) = 0;

        /**
         * Notify callback of change in value.
         * TODO: convert to a vector of listeners
         */
        virtual int notifyChange(const PropT& value) {
            if (notifyChangeFunc_) {
                return (device_->*notifyChangeFunc_)(name_.c_str(), ToString(value).c_str());
            }
            return DEVICE_OK;
        }

        /**
         * Allows a class to override any property info flags..
         * Called during createAndLinkProp
         */
        virtual PropInfo<PropT> checkPropInfo(const PropInfo<PropT>& propInfo) = 0;

        /** Link the property to the device and initialize from the checkedInfo. */
        int createAndLinkProp(DeviceT* device, const PropInfo<PropT>& propInfo,
                              MM::ActionFunctor* action) {
            assert(device != nullptr);
            auto checkedInfo = checkPropInfo(propInfo);
            device_          = device;
            name_            = checkedInfo.name();
            brief_           = checkedInfo.brief();
            isReadOnly_      = checkedInfo.isReadOnly();
            isSequencable_   = checkedInfo.isSequencable();
            isVolatile_      = checkedInfo.isVolatileValue();

            if (checkedInfo.hasInitialValue()) {
                cachedValue_ = checkedInfo.initialValue();
            }
            int ret = device_->CreateProperty(name_.c_str(), ToString(cachedValue_).c_str(),
                                              MMPropertyType_of<PropT>(cachedValue_),
                                              isReadOnly_, action, checkedInfo.isPreInit());
            if (ret != DEVICE_OK) {
                return ret;
            }
            if (checkedInfo.hasLimits()) {
                ret = device->SetPropertyLimits(checkedInfo.name().c_str(), checkedInfo.minValue(), checkedInfo.maxValue());
            }
            if (checkedInfo.hasAllowedValues()) {
                std::vector<std::string> allowedStrings;
                for (PropT aval : checkedInfo.allowedValues()) {
                    allowedStrings.push_back(ToString(aval));
                }
                device->SetAllowedValues(checkedInfo.name().c_str(), allowedStrings);
            }
            return ret;
        }

     protected:
        /** Protected constructor - only called by derived classes */
        DeviceProp_Base()
            : device_(nullptr),
              name_(),
              brief_(),
              cachedValue_(),
              isReadOnly_(false),
              isSequencable_(false),
              isVolatile_(false),
              notifyChangeFunc_(nullptr) {}

        DeviceT* device_;
        std::string name_;
        std::string brief_;
        PropT cachedValue_;
        bool isReadOnly_;
        bool isSequencable_;
        bool isVolatile_;
        NotifyChangeFnT notifyChangeFunc_;

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

        /** Creates a property that calls an update function on device from the checkedInfo.
         *
         * The initial value is given as a parameter.
         * This version requires a device member function of the form
         * int OnProperty(MM::PropertyBase* pPropt, MM::ActionType eAct) The initial value is taken from checkedInfo.
         */
        template <typename PropT, class DeviceT>
        static inline int createMMPropOnDevice(DeviceT* device,
                                               const PropInfo<PropT>& checkedInfo,
                                               const PropT& initialValue,
                                               int (DeviceT::*fn)(MM::PropertyBase* pPropt, MM::ActionType eAct),
                                               bool isReadOnly = false) {
            MM::Action<DeviceT>* action = fn ? new MM::Action<DeviceT>(device, fn) : nullptr;
            return createMMPropOnDevice(device, checkedInfo, initialValue, action, isReadOnly);
        };

        ///** Creates a property Without initial value that calls an update function on device from the checkedInfo.
        // *
        // * Create MM device properties from PropInfo information. This version requires a device member function of the form
        // * int OnProperty(MM::PropertyBase* pPropt, MM::ActionType eAct) The initial value is taken from checkedInfo.
        // */
        //template <typename PropT, class DeviceT>
        //static inline int createMMPropOnDevice(DeviceT* device,
        //                                       const PropInfo<PropT>& checkedInfo,
        //                                       int (DeviceT::*fn)(MM::PropertyBase* pPropt, MM::ActionType eAct),
        //                                       bool isReadOnly = false) {
        //    return createMMPropOnDevice(device, checkedInfo, checkedInfo.initialValue(), fn, isReadOnly);
        //}

        /** Creates a property that calls an update action on device from the checkedInfo.
         *
         * ### THIS IS THE MAIN PROPERTY CREATION ENTRY POINT
         *
         * The initial value is given as a parameter.
         */
        template <typename PropT, class DeviceT>
        static inline int createMMPropOnDevice(DeviceT* device,
                                               const PropInfo<PropT>& checkedInfo,
                                               PropT initialValue,
                                               MM::ActionFunctor* action,
                                               bool isReadOnly = false) {
            // double-check the read-only flag if the checkedInfo was created with the assertReadOnly() flag
            if (checkedInfo.isReadOnly() && !isReadOnly) {
                assert(checkedInfo.isReadOnly() == isReadOnly);
                // Create an "ERROR" property if assert was turned off during compile.
                device->CreateProperty(checkedInfo.name(), "CreateProperty ERROR: read-write property did not assertReadOnly", MM::String, true);
                return DEVICE_INVALID_PROPERTY;
            }
            int ret = device->CreateProperty(checkedInfo.name(),
                                             ToString(initialValue).c_str(),
                                             MMPropertyType_of<PropT>(initialValue),
                                             isReadOnly,
                                             action,
                                             checkedInfo.isPreInit());
            if (ret != DEVICE_OK) {
                return ret;
            }
            if (checkedInfo.hasLimits()) {
                ret = device->SetPropertyLimits(checkedInfo.name(), checkedInfo.minValue(), checkedInfo.maxValue());
            }
            if (checkedInfo.hasAllowedValues()) {
                std::vector<std::string> allowedStrings;
                for (PropT aval : checkedInfo.allowedValues()) {
                    allowedStrings.push_back(ToString(aval));
                }
                device->SetAllowedValues(checkedInfo.name(), allowedStrings);
            }
            return ret;
        };
    #endif
    };

}; // namespace aling

#endif // __DEVICE_PROP__
