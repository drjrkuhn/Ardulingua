#pragma once

#ifndef __LOCAL_PROP__
    #define __LOCAL_PROP__

    #include "DeviceProp.h"

namespace rdl {

/* Think of the the Properties as simple storage containers
* The BeforeGet action occurs before the Device Adapter's 
*  client views the property and should query the hardware 
*  if necessary, AfterSet occurs after the client has specified 
*  a new value of the property and should write the property 
*  out to the hardware if necessary. 
*  (( 'pProp->Get' might better be thought of as 'pProp->Retrieve' 
*  and  'pProp->Set' might better be thought of as 'pProp->Store'))   
*/
//int YourLaserController::OnPowerSetpoint(MM::PropertyBase* pProp, MM::ActionType eAct) {
//    double powerSetpoint;
//    if (eAct == MM::BeforeGet) {
//        // retrieve the current setpoint from the controller
//        GetPowerSetpoint(powerSetpoint);
//        // put the retrieved value into the Property storage
//        pProp->Set(powerSetpoint);
//    } else if (eAct == MM::AfterSet) {
//        // retrieve the requested value from the Property storage
//        pProp->Get(powerSetpoint);
//        double achievedSetpoint;
//        // set the value to the controller, retrieve the realized (quantized) value from the controller
//        SetPowerSetpoint(powerSetpoint, achievedSetpoint);
//        if (0. != powerSetpoint) {
//            double fractionError = fabs(achievedSetpoint - powerSetpoint) / powerSetpoint;
//            if ((0.05 < fractionError) && (fractionError < 0.10))
//                // reflect the quantized value back to the client
//                pProp->Set(achievedSetpoint);
//        }
//    }
//    return DEVICE_OK;
//}

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
    template <class DeviceT, typename PropT>
    class LocalProp_Base : public DeviceProp_Base<DeviceT, PropT> {
     protected:
        using BaseT   = DeviceProp_Base<DeviceT, PropT>;
        using ActionT = MM::Action<LocalProp_Base<DeviceT, PropT>>;

        LocalProp_Base() = default;

        /*	Link the property to the device and initialize from the propInfo.	*/
        virtual int create(DeviceT* device, const PropInfo<PropT>& propInfo, bool readOnly = false, bool usePropInfoInitialValue = true) {
            MM::ActionFunctor* pAct = new ActionT(this, &LocalProp_Base<DeviceT, PropT>::OnExecute);
            readOnly_               = readOnly;
            return createAndLinkProp(device, propInfo, pAct, readOnly, usePropInfoInitialValue);
        }

        /** Called by the properties update method */
        virtual int OnExecute(MM::PropertyBase* pprop, MM::ActionType action) override {
            int ret;
            // Use our helper functions above to do the work
            if (action == MM::BeforeGet) {
                PropT temp;
                // GET the property from the device. In our case a locally
                // cached value
                if ((ret = getCached_impl(temp)) != DEVICE_OK) { return ret; }
                // Set the property storage value to the one we GOT
                Assign(*pprop, temp);
            } else if (!readOnly_ && action == MM::AfterSet) {
                PropT temp;
                // Get the property storage value that was just
                // set by the GUI or Core
                Assign(temp, *pprop);
                // SET the property to the device. In our case a locally
                // cached value
                if ((ret = set_impl(temp)) != DEVICE_OK) {
                    return ret;
                }
                return notifyChange(temp);
            }
            // Note that attempting to set a read-only property does
            // not generate an error by convention
            return DEVICE_OK;
        }

        /** Set the value after updating the property. Derived classes may override. */
        virtual int set_impl(const PropT value) override {
            cachedValue_ = value;
            return DEVICE_OK;
        }

        ///** Get the value before updating the property. Derived classes may override. */
        virtual int get_impl(PropT& value) const override {
            return getCached_impl(value);
        }

        /** Get the cached property value (last call to set()) */
        virtual int getCached_impl(PropT& value) const {
            value = cachedValue_;
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
    template <class DeviceT, typename PropT>
    class LocalProp : public LocalProp_Base<DeviceT, PropT> {
     protected:
        using BaseT = LocalProp_Base<DeviceT, PropT>;

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
    template <class DeviceT, typename PropT>
    class LocalReadOnlyProp : public LocalProp<DeviceT, PropT> {
     protected:
        using BaseT = LocalProp<DeviceT, PropT>;

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
