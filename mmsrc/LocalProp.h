#pragma once

#ifndef __LOCAL_PROP__
    #define __LOCAL_PROP__

    #include "DeviceProp.h"

namespace rdl {

    /*******************************************************************
     * LocalProp_Base
     *******************************************************************/

    /**
     * A base class for holding local property value.
     * Micromanager updates the property through the OnExecute member function.
     *
     * The PropT template parameter should contain the type of the member property.
     * The type PropT should be able to auto-box and -unbox (auto-cast) from
     * either MMInteger, MMFloat, or MMString.
     * For example, if PropT=char, int, or long, then the property will be designated MM::Integer
     *
     * The DeviceT template parameter holds the device type
     */
    template <typename PropT, typename DeviceT>
    class LocalProp_Base : public DeviceProp_Base<PropT, DeviceT> {
     public:
        /** Get the value before updating the property. Derived classes may override. */
        virtual int get(PropT& value) {
            value = cachedValue_;
            return DEVICE_OK;
        }

        /** Set the value after updating the property. Derived classes may override. */
        virtual int set(const PropT& value) {
            cachedValue_ = value;
            return DEVICE_OK;
        }

     protected:
        using BaseT   = DeviceProp_Base<PropT, DeviceT>;
        using ActionT = MM::Action<LocalProp_Base<PropT, DeviceT>>;

        LocalProp_Base() = default;

        /*	Link the property to the device and initialize from the propInfo.	*/
        virtual int create(DeviceT* device, const PropInfo<PropT>& propInfo, bool readOnly = false, bool usePropInfoInitialValue = true) {
            MM::ActionFunctor* pAct = new ActionT(this, &LocalProp_Base<PropT, DeviceT>::OnExecute);
            readOnly_               = readOnly;
            return createAndLinkProp(device, propInfo, pAct, readOnly, usePropInfoInitialValue);
        }

        /** Called by the properties update method */
        virtual int OnExecute(MM::PropertyBase* pprop, MM::ActionType action) override {
            int ret;
            // Use our helper functions above to do the work
            if (action == MM::BeforeGet) {
                PropT temp;
                if ((ret = get(temp)) != DEVICE_OK) { return ret; }
                Assign(*pprop, temp);
            } else if (!readOnly_ && action == MM::AfterSet) {
                PropT temp;
                Assign(temp, *pprop);
                if ((ret = set(temp)) != DEVICE_OK) { return ret; }
                return notifyChange(temp);
            }
            return DEVICE_OK;
        }

     protected:
        using BaseT::readOnly_;
        using BaseT::cachedValue_;
    };

    /*******************************************************************
     * LocalProp (Read/Write)
     *******************************************************************/

    /**
     * A class for holding a local read/write property value for a device.
     * The PropT template parameter should contain the type of the member property.
     * The type PropT should be able to auto-box and -unbox (auto-cast) from
     * either MMInteger, MMFloat, or MMString.
     * For example, if PropT=char, int, or long, then the property will be designated MM::Integer
     * The DeviceT template parameter holds the device type
     */
    template <typename PropT, typename DeviceT>
    class LocalProp : public LocalProp_Base<PropT, DeviceT> {
     protected:
        using BaseT = LocalProp_Base<PropT, DeviceT>;

     public:
        /** A local read/write property that will be initialized from the PropInfo initialValue. */
        LocalProp() : LocalProp(false, true) {}

        /** A local read/write property that will be initialized with the given initialValue.
         * This value overrides the PropInfo initialValue. */
        LocalProp(const PropT& initialValue) : LocalProp(false, false) {}

        /** Create a local property from a given PropInfo builder */
        int create(DeviceT* device, const PropInfo<PropT>& propInfo) {
            return BaseT::create(device, propInfo, readOnly_, initFromPropInfo_);
        }

     protected:
        LocalProp(bool readOnly, bool initFromPropInfo)
            : initFromPropInfo_(initFromPropInfo) {
            readOnly_ = readOnly;
        }

        using BaseT::readOnly_;
        bool initFromPropInfo_;
    };

    /*******************************************************************
     * LocalReadOnlyProp (Read-only)
     *******************************************************************/

    /**
     * A class for holding a local read-only property value for a device.
     * The PropT template parameter should contain the type of the member property.
     * The type PropT should be able to auto-box and -unbox (auto-cast) from
     * either MMInteger, MMFloat, or MMString.
     * For example, if PropT=char, int, or long, then the property will be designated MM::Integer
     * The DeviceT template parameter holds the device type
     */
    template <typename PropT, typename DeviceT>
    class LocalReadOnlyProp : public LocalProp<PropT, DeviceT> {
     protected:
        using BaseT = LocalProp<PropT, DeviceT>;

     public:
        /** A local read-only property that will be initialized from the PropInfo initialValue. */
        LocalReadOnlyProp() : LocalProp(true, true) {}

        /** A local read-only property that will be initialized with the given initialValue.
        This value overrides the PropInfo initialValue. */
        LocalReadOnlyProp(const PropT& initialValue) : LocalProp(true, false) {
            setCachedValue(initialValue);
        }

        /** Set the cached value of a read-only property. If the property was not yet created through
        create, then this value overrides the PropInfo initialValue. */
        int setCached(const PropT& value) {
            initFromPropInfo_ = false;
            int ret;
            if ((ret = setValue(value)) != DEVICE_OK) {
                return ret;
            }
            return notifyChange(value);
        }

     protected:
        using BaseT::initFromPropInfo_;
    };

}; // namespace rdl

#endif // __LOCAL_PROP__
