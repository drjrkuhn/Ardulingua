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
	* The T template parameter should contain the type of the member property.
	* The type T should be able to auto-box and -unbox (auto-cast) from
	* either MMInteger, MMFloat, or MMString.
	* For example, if T=char, int, or long, then the property will be designated MM::Integer
    *
	* The DEV template parameter holds the device type
	*/
    template <typename T, typename DEV>
    class LocalProp_Base : public DeviceProp_Base<T, DEV> {
     protected:
        using ActionType = MM::Action<LocalProp_Base<T, DEV>>;

        LocalProp_Base() {}
        virtual ~LocalProp_Base() {}
        
        /*	Link the property to the device and initialize from the propInfo.	*/
        int createLocalPropH(DEV* device, const PropInfo<T>& propInfo, bool readOnly = false, bool usePropInfoInitialValue = true) {
            MM::ActionFunctor* pAct = new ActionType(this, &LocalProp_Base<T, DEV>::OnExecute);
            readOnly_               = readOnly;
            return createDevicePropH(device, propInfo, pAct, readOnly, usePropInfoInitialValue);
        }

        /** Get the value before updating the property. Derived classes may override. */
        virtual int getLocalValueH(T& value) {
            value = cachedValue_;
            return DEVICE_OK;
        }

        /** Set the value after updating the property. Derived classes may override. */
        virtual int setLocalValueH(const T& value) {
            cachedValue_ = value;
            return DEVICE_OK;
        }

        /** Called by the properties update method */
        virtual int OnExecute(MM::PropertyBase* pprop, MM::ActionType action) override {
            int ret;
            // Use our helper functions above to do the work
            if (action == MM::BeforeGet) {
                T temp;
                if ((ret = getLocalValueH(temp)) != DEVICE_OK) { return ret; }
                Assign(*pprop, temp);
            } else if (!readOnly_ && action == MM::AfterSet) {
                T temp;
                Assign(temp, *pprop);
                if ((ret = setLocalValueH(temp)) != DEVICE_OK) { return ret; }
                return notifyChangeH(temp);
            }
            return DEVICE_OK;
        }

     protected:
        bool readOnly_ = false;
        T cachedValue_;
    };

    /*******************************************************************
    * LocalProp (Read/Write)
    *******************************************************************/

    /**
	* A class for holding a local read/write property value for a device.
    * The T template parameter should contain the type of the member property.
    * The type T should be able to auto-box and -unbox (auto-cast) from
    * either MMInteger, MMFloat, or MMString.
    * For example, if T=char, int, or long, then the property will be designated MM::Integer
    * The DEV template parameter holds the device type
	*/
    template <typename T, typename DEV>
    class LocalProp : public LocalProp_Base<T, DEV> {
     public:
         //using LocalProp_Base<T,DEV>::getCachedValue;

        /** A local read/write property that will be initialized from the PropInfo initialValue. */
        LocalProp() : LocalProp(false, true) {}

        /** A local read/write property that will be initialized with the given initialValue.
			This value overrides the PropInfo initialValue. */
        LocalProp(const T& initialValue) : LocalProp(false, false), cachedValue_(initialValue) {}

        virtual int createLocalProp(DEV* device, const PropInfo<T>& propInfo) {
            return createLocalPropH(device, propInfo, readOnly_, initFromPropInfo_);
        }

     protected:
        LocalProp(bool readOnly, bool initFromPropInfo) : readOnly_(readOnly), initFromPropInfo_(initFromPropInfo) {}

        using LocalProp_Base<T,DEV>::cachedValue_;
        bool readOnly_;
        bool initFromPropInfo_;
    };

    /*******************************************************************
    * LocalReadOnlyProp (Read-only)
    *******************************************************************/

    /**
    * A class for holding a local read-only property value for a device.
	* The T template parameter should contain the type of the member property.
	* The type T should be able to auto-box and -unbox (auto-cast) from
	* either MMInteger, MMFloat, or MMString.
	* For example, if T=char, int, or long, then the property will be designated MM::Integer
    * The DEV template parameter holds the device type
	*/
    template <typename T, typename DEV>
    class LocalReadOnlyProp : public LocalProp<T, DEV> {
     public:
        /** A local read-only property that will be initialized from the PropInfo initialValue. */
        LocalReadOnlyProp() : LocalProp(true, true) {}

        /** A local read-only property that will be initialized with the given initialValue.
		This value overrides the PropInfo initialValue. */
        LocalReadOnlyProp(const T& initialValue) : LocalProp(true, false), cachedValue_(initialValue) {}

        /** Set the cached value of a read-only property. If the property was not yet created through
		createLocalProp, then this value overrides the PropInfo initialValue. */
        int setCachedValue(const T& value) {
            initFromPropInfo_ = false;
            int ret;
            if ((ret = setLocalValueH(value)) != DEVICE_OK) { return ret; }
            return notifyChangeH(value);
        }

     protected:
        using LocalProp_Base<T, DEV>::cachedValue_;
        using LocalProp_Base<T, DEV>::initFromPropInfo_;

    };

}; // namespace dprop

#endif // __LOCAL_PROP__
