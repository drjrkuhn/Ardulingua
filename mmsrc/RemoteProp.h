#pragma once

#ifndef __REMOTEPROP_H__
#define __REMOTEPROP_H__

#include "DeviceProp.h"
#include "DevicePropHelpers.h"
#include "DeviceError.h"

namespace rdl {

    /** Throw an error during createRemotePropH on bad communication at creation? */
    const bool CREATE_FAILS_IF_ERR_COMMUNICATION = false;


    template <typename PropT, typename RemoteT, class DeviceT, class HubT>
    class RemoteProp_Base : public DeviceProp_Base<PropT, DeviceT> {
        using BaseT = DeviceProp_Base<PropT, DeviceT>;
        using ProtoT = std::stringstream; // TODO: FIX
        using ActionT = MM::ActionT<RemoteProp_Base<PropT, DeviceT, HubT>>

     protected:
        RemoteProp_Base() : stream_(nullptr) {}
        ProtoT* stream_;

        virtual ~RemoteProp_Base() {}

      protected:
        /**	Link the property to the device through the stream and initialize from the propInfo.
	    * **THIS IS THE PRIMARY ENTRY POINT for creating remote properties.**
		*/
        int createRemotePropH(DeviceT* device, ProtoT* stream, const PropInfo<PropT>& propInfo, CommandSet& cmdSet) {
            pProto_ = stream;
            cmds_   = cmdSet;
            int ret;
            bool readOnly        = cmds_.propInfo() == 0;
            bool useInitialValue = false;
            if (readOnly && cmds_.cmdGet()) {
                typename ProtoT::StreamGuard monitor(pProto_);
                // We do not have an initial value and must retrieve and cached the value from the device
                if ((ret = getRemoteValueH(BaseT::cachedValue_)) != DEVICE_OK && CREATE_FAILS_IF_ERR_COMMUNICATION) {
                    return ERR_COMMUNICATION;
                }
                useInitialValue = false;
            } else if (cmds_.propInfo()) {
                useInitialValue = true;
            }
            MM::ActionFunctor* pAct = new ActionT(this, &RemoteProp_Base<PropT, DeviceT, HubT>::OnExecute);
            ret                     = createDevicePropH(device, propInfo, pAct, readOnly, useInitialValue);
            if (ret == DEVICE_OK && cmds_.propInfo()) {
                typename ProtoT::StreamGuard monitor(pProto_);
                // set the property on the remote device if possible
                if ((ret = setRemoteValueH(BaseT::cachedValue_)) != DEVICE_OK && CREATE_FAILS_IF_ERR_COMMUNICATION) {
                    return ERR_COMMUNICATION;
                }
            }
            return DEVICE_OK;
        }


        /*******************************************************************
        * Property getting/setting - internal
        *******************************************************************/
        /** Get the value from the remote. Derived classes may override. */
        virtual int getRemoteValueH(PropT& __val) {


            if (cmds_.hasChan()) {
                if (pProto_->dispatchChannelGet(cmds_.cmdGet(), cmds_.cmdChan(), __val)) {
                    return DEVICE_OK;
                }
            } else {
                if (pProto_->dispatchGet(cmds_.cmdGet(), __val)) {
                    return DEVICE_OK;
                }
            }
            return ERR_COMMUNICATION;
        }

        /** Set the value on the remote. Derived classes may override. */
        virtual int setRemoteValueH(const PropT& __val) {
            if (cmds_.hasChan()) {
                if (pProto_->dispatchChannelSet(cmds_.propInfo(), cmds_.cmdChan(), __val)) {
                    return DEVICE_OK;
                }
            } else {
                if (pProto_->dispatchSet(cmds_.propInfo(), __val)) {
                    return DEVICE_OK;
                }
            }
            return ERR_COMMUNICATION;
        }

        ////////////////////////////////////////////////////////////////////
        /// Sequence setting and triggering
        /// Sub-classes may override to change the default behavior

        /** Get the maximum size of the remote sequence. Derived classes may override. */
        virtual int getRemoteSequenceSizeH(hprot::prot_size_t& __size) const {
            __size = getRemoteArrayMaxSizeH<PropT>(cmds_.cmdSetSeq());
            return DEVICE_OK;
        }

        /** Set a remote sequence. Derived classes may override. */
        virtual int setRemoteSequenceH(const std::vector<std::string> __sequence) {
            hprot::prot_size_t maxSize = getRemoteArrayMaxSizeH<PropT>(cmds_.cmdSetSeq());
            if (__sequence.size() > maxSize) {
                return DEVICE_SEQUENCE_TOO_LARGE;
            }
            // Use a helper function to send the sequence string array
            // to the device
            if (!putRemoteStringArrayH<PropT>(cmds_.cmdSetSeq(), __sequence, maxSize)) {
                return ERR_COMMUNICATION;
            }
            return DEVICE_OK;
        }

        /** Start the remote sequence. Derived classes may override. */
        virtual int startRemoteSequenceH() {
            if (cmds_.hasChan()) {
                if (pProto_->dispatchChannelTask(cmds_.cmdStartSeq(), cmds_.cmdChan())) {
                    return DEVICE_OK;
                }
            } else {
                if (pProto_->dispatchTask(cmds_.cmdStartSeq())) {
                    return DEVICE_OK;
                }
            }
            return ERR_COMMUNICATION;
        }

        /** Stop the remote sequence. Derived classes may override. */
        virtual int stopRemoteSequenceH() {
            if (cmds_.hasChan()) {
                if (pProto_->dispatchChannelTask(cmds_.cmdStopSeq(), cmds_.cmdChan())) {
                    return DEVICE_OK;
                }
            } else {
                if (pProto_->dispatchTask(cmds_.cmdStopSeq())) {
                    return DEVICE_OK;
                }
            }
            return ERR_COMMUNICATION;
        }

        /* Called by the properties update method.
			 This is the main Property update routine. */
        virtual int OnExecute(MM::PropertyBase* pProp, MM::ActionT eAct) override {
            typename ProtoT::StreamGuard monitor(pProto_);
            int result;
            if (eAct == MM::BeforeGet) {
                if (cmds_.cmdGet()) {
                    // read the value from the remote device
                    PropT temp;
                    if ((result = getRemoteValueH(temp)) != DEVICE_OK) {
                        return result;
                    }
                    BaseT::cachedValue_ = temp;
                    SetProp<PropT>(pProp, BaseT::cachedValue_);
                } else {
                    // Just use the getCachedValue
                    SetProp<PropT>(pProp, BaseT::cachedValue_);
                }
            } else if (cmds_.propInfo() && eAct == MM::AfterSet) {
                PropT temp;
                SetValue<PropT>(temp, pProp);
                if ((result = setRemoteValueH(temp)) != DEVICE_OK) {
                    return result;
                }
                BaseT::cachedValue_ = temp;
                return notifyChangeH(BaseT::cachedValue_);
            } else if (cmds_.cmdSetSeq() && eAct == MM::IsSequenceable) {
                hprot::prot_size_t maxSize;
                if ((result = getRemoteSequenceSizeH(maxSize)) != DEVICE_OK) {
                    return result;
                }
                // maxSize will be zero if there was no setSeqCommand
                // or an error occurred. SetSequencable(0) indicates
                // that the property cannot be sequenced
                pProp->SetSequenceable(maxSize);
            } else if (cmds_.cmdSetSeq() && eAct == MM::AfterLoadSequence) {
                std::vector<std::string> sequence = pProp->GetSequence();
                if ((result = setRemoteSequenceH(sequence)) != DEVICE_OK) {
                    return result;
                }
            } else if (cmds_.cmdSetSeq() && eAct == MM::StartSequence) {
                if ((result = startRemoteSequenceH()) != DEVICE_OK) {
                    return result;
                }
            } else if (cmds_.cmdSetSeq() && eAct == MM::StopSequence) {
                if ((result = stopRemoteSequenceH()) != DEVICE_OK) {
                    return result;
                }
            }
            return DEVICE_OK;
        }
    };

    /////////////////////////////////////////////////////////////////////////////
    // Specific RemoteProp implementations
    /////////////////////////////////////////////////////////////////////////////

    /**
		A class to hold a read/write remote property value.

		\ingroup RemoteProp

		Micromanager updates the
		property through the OnExecute member function, which in turn
		gets or sets the value from the device.
	*/
    template <typename PropT, class DeviceT, class HubT>
    class RemoteProp : public RemoteProp_Base<PropT, DeviceT, HubT> {
        typedef hprot::DeviceHexProtocol<HubT> ProtoT;

     public:
        int createRemoteProp(DeviceT* device, ProtoT* stream, const PropInfo<PropT>& propInfo, CommandSet& __cmds) {
            assert(__cmds.propInfo() || __cmds.cmdGet());
            return createRemotePropH(device, stream, propInfo, __cmds);
        }
    };

    /**
	A class to hold a write-only remote property value.

	\ingroup RemoteProp

	Micromanager updates
	the property through the OnExecute member function, which in turn
	sets the value from the device and updates the getCachedValue.
	Get simply returns the last cached value.
	*/
    template <typename PropT, class DeviceT, class HubT>
    class RemoteCachedProp : public RemoteProp_Base<PropT, DeviceT, HubT> {
        typedef hprot::DeviceHexProtocol<HubT> ProtoT;

     public:
        int createRemoteProp(DeviceT* device, ProtoT* stream, const PropInfo<PropT>& propInfo, CommandSet& __cmds) {
            assert(__cmds.propInfo());
            return createRemotePropH(device, stream, propInfo, __cmds);
        }
    };

    /**
	A class to hold a sequencable write-only remote property value.

	\ingroup RemoteProp

	Micromanager updates the property through the OnExecute member function,
	which in turn sets the value from the device and updates the getCachedValue.
	Get simply returns the last cached value. This version exposes the sequencable
	helper interface.

	*/
    template <typename PropT, class DeviceT, class HubT>
    class RemoteSequenceableProp : public RemoteProp_Base<PropT, DeviceT, HubT> {
        typedef hprot::DeviceHexProtocol<HubT> ProtoT;

     public:
        int createRemoteProp(DeviceT* device, ProtoT* stream, const PropInfo<PropT>& propInfo, CommandSet& __cmds) {
            assert(__cmds.propInfo() && __cmds.cmdSetSeq() && __cmds.cmdStartSeq() && __cmds.cmdStopSeq());
            return createRemotePropH(device, stream, propInfo, __cmds);
        }

        std::vector<PropT> GetRemoteArray() {
            return getRemoteArrayH<PropT>(RemoteProp_Base<PropT, DeviceT, HubT>::cmds_.cmdGetSeq());
        }

        /** Get the maximum size of the remote sequence. */
        int getRemoteSequenceSize(hprot::prot_size_t& __size) const {
            return getRemoteSequenceSizeH(__size);
        }

        /** Set a remote sequence. */
        int setRemoteSequence(const std::vector<std::string> __sequence) {
            return setRemoteSequenceH(__sequence);
        }

        /** Start the remote sequence. */
        int startRemoteSequence() {
            return startRemoteSequenceH();
        }

        /** Stop the remote sequence. */
        int stopRemoteSequence() {
            return stopRemoteSequenceH();
        }

     protected:
    };

    /**
	A class to hold a read-only remote property value.

	\ingroup RemoteProp

	Micromanager updates
	the property through the OnExecute member function, which in turn
	gets the value from the device and updates the getCachedValue.
	Sets do nothing.
	*/
    template <typename PropT, class DeviceT, class HubT>
    class RemoteReadOnlyProp : public RemoteProp_Base<PropT, DeviceT, HubT> {
        typedef RemoteProp_Base<PropT, DeviceT, HubT> BaseT;
        typedef hprot::DeviceHexProtocol<HubT> ProtoT;

     public:
        int createRemoteProp(DeviceT* device, ProtoT* stream, const PropInfo<PropT>& propInfo, CommandSet& __cmds) {
            assert(__cmds.cmdGet());
            return createRemotePropH(device, stream, propInfo, __cmds);
        }
    };



}; // namespace rdl



#endif // __REMOTEPROP_H__

