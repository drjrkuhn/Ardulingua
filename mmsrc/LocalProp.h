#pragma once

#ifndef __LOCAL_PROP__
    #define __LOCAL_PROP__

    #include "DeviceProp.h"

namespace dprop {

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
	* The DEVICE template parameter holds the device type
	*/
    template <typename PropT, typename DEVICE>
    class LocalProp_Base : public DeviceProp_Base<PropT, DEVICE> {
     protected:
        using ActionType = MM::Action<LocalProp_Base<PropT, DEVICE>>;
        using DeviceProp_Base<PropT, DEVICE>::cachedValue_;

        LocalProp_Base() {}
        virtual ~LocalProp_Base() {}

        /*	Link the property to the device and initialize from the propInfo.	*/
        int createLocalPropH(DEVICE* device, const PropInfo<PropT>& propInfo, bool readOnly = false, bool usePropInfoInitialValue = true) {
            MM::ActionFunctor* pAct = new ActionType(this, &LocalProp_Base<PropT, DEVICE>::OnExecute);
            readOnly_               = readOnly;
            return createAndLinkProp(device, propInfo, pAct, readOnly, usePropInfoInitialValue);
        }

        /** Get the value before updating the property. Derived classes may override. */
        virtual int getLocalValueH(PropT& value) {
            value = cachedValue_;
            return DEVICE_OK;
        }

        /** Set the value after updating the property. Derived classes may override. */
        virtual int setLocalValueH(const PropT& value) {
            cachedValue_ = value;
            return DEVICE_OK;
        }

        /** Called by the properties update method */
        virtual int OnExecute(MM::PropertyBase* pprop, MM::ActionType action) override {
            int ret;
            // Use our helper functions above to do the work
            if (action == MM::BeforeGet) {
                PropT temp;
                if ((ret = getLocalValueH(temp)) != DEVICE_OK) { return ret; }
                Assign(*pprop, temp);
            } else if (!readOnly_ && action == MM::AfterSet) {
                PropT temp;
                Assign(temp, *pprop);
                if ((ret = setLocalValueH(temp)) != DEVICE_OK) { return ret; }
                return notifyChangeH(temp);
            }
            return DEVICE_OK;
        }

     protected:
        using DeviceProp_Base<PropT, DEVICE>::readOnly_;
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
    * The DEVICE template parameter holds the device type
	*/
    template <typename PropT, typename DEVICE>
    class LocalProp : public LocalProp_Base<PropT, DEVICE> {
     public:
        /** A local read/write property that will be initialized from the PropInfo initialValue. */
        LocalProp() : LocalProp(false, true) {}

        /** A local read/write property that will be initialized with the given initialValue.
			This value overrides the PropInfo initialValue. */
        LocalProp(const PropT& initialValue) : LocalProp(false, false) {}

        virtual int createLocalProp(DEVICE* device, const PropInfo<PropT>& propInfo) {
            return createLocalPropH(device, propInfo, readOnly_, initFromPropInfo_);
        }

     protected:
        LocalProp(bool readOnly, bool initFromPropInfo) : initFromPropInfo_(initFromPropInfo) { readOnly_ = readOnly; }

        using LocalProp_Base<PropT, DEVICE>::readOnly_;
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
    * The DEVICE template parameter holds the device type
	*/
    template <typename PropT, typename DEVICE>
    class LocalReadOnlyProp : public LocalProp<PropT, DEVICE> {
     public:
        /** A local read-only property that will be initialized from the PropInfo initialValue. */
        LocalReadOnlyProp() : LocalProp(true, true) {}

        /** A local read-only property that will be initialized with the given initialValue.
		This value overrides the PropInfo initialValue. */
        LocalReadOnlyProp(const PropT& initialValue) : LocalProp(true, false) {
            setCachedValue(initialValue);
        }

        /** Set the cached value of a read-only property. If the property was not yet created through
		createLocalProp, then this value overrides the PropInfo initialValue. */
        int setCached(const PropT& value) {
            initFromPropInfo_ = false;
            int ret;
            if ((ret = setLocalValueH(value)) != DEVICE_OK) { return ret; }
            return notifyChangeH(value);
        }

     protected:
        using LocalProp<PropT, DEVICE>::initFromPropInfo_;
    };

}; // namespace dprop

#endif // __LOCAL_PROP__
