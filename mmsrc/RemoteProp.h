#pragma once

#ifndef __REMOTEPROP_H__
    #define __REMOTEPROP_H__

    #include "DeviceError.h"
    #include "DeviceProp.h"
    #include "DevicePropHelpers.h"
    #include <Delegate.h>
    #include <JsonDispatch.h>
    #include <StreamAdapter.h>

/************************************************************************
     * # Remote properties
     * 
     * ## Briefs and Property Codes
     * 
     * Property get/set/sequencing rely on JsonDelegate method names with
     * special prepended codes. The remote property name should be a short,
     * unique character sequence like "prop". We will call this abbreviation
     * of the property name the property `brief`.
     * 
     * We prepend a single-character code to the property brief to denote
     * a standard property operation. The server is responsible for
     * dispatching the coded brief to the appropriate function. A possible
     * mechanism is detailed below, but dispatch tables are flexible and
     * can use pure callback functions, class methods, or lambda with capture
     * 
     *   > NOTE: codes are prepended rather than appended to make string
     *   > matching terminate earlier during dispatch map method lookup.
     *   > `brief` method tags should be kept to a few characters for the same
     *   > reason. Even a fast hash-map will need to loop through the entire
     *   > string to compute the hash-value for lookup. So use a brief method
     *   > tag like "dv" rather than "MyDACOutputValueInVolts".
     * 
     * | code | operation                          |meth[^1]| server signature[^2]                      |
     * |:----:|:-----------------------------------|:-----:|:-------------------------------------------|
     * |  ?   | GET value                          | get   | call<T,EX...>("?brief",ex...)->T           |
     * |  !   | SET value                          | set   | call<void,T,EX...>("!brief",t,ex...)       |
     * |  !   | SETN value - no reply              | set   | notify<void,T,EX...>("!brief",t,ex...)     |
     * |  *   | ACT task                           | act   | call<void,EX...>("*brief",ex...)           |
     * |  >   | NOTIFY task (DO without response)  | act   | notify<void,EX...>(">brief",ex...)         |
     * |  --  | ==== SEQUENCE/ARRAY COMMANDS ====  | --    | --                                         |
     * |  ^   | GET maximum size of seq array      | array | call<size_t,EX...>("^brief",ex...)->size_t |
     * |  #   | GET number of values in seq array  | array | call<size_t,EX...>("#brief",ex...)->size_t |
     * |  0   | CLEAR seq array                    | array | notify<size_t,EX...>("0brief",ex...)->dummy|
     * |  +   | ADD value to sequence array        | set   | notify<void,T,EX...>("+brief",ex...)       |
     * |  *   | `ACT task` doubles as start seq.   | act   | call<void,EX...>("*brief",ex...)           |
     * |  >   | `NOTIFY task` to start seq.        | act   | notify<void,EX...>(">brief",ex...)         |
     * 
     * [^1]: meth is the client method whose parameters match the call/notify signature
     * [^2]: Signature of the server method. T is the property type on the device, EX... are an 
     * optional set of extra parameters such as channel number
     * 
     * ### Client transform/dispatch methods
     * 
     * From the signature table above, we need four local methods for transforming MM Properties
     * into eventual RPC calls on the server. The client method might also transform the
     * MM::PropertyType into the type T required by the server. Each method type includes
     * an optional set of compile-time extra parameters such as channel number, pin number, etc.
     * What the server does with this iinformation depends on the method code.
     * 
     * - get: gets the remote property value
     * - set: sets the remote property value
     * - act (action): performs some task associated with the property and check status
     * - notify: perform some task associated with the property but don't check status
     * - array: (array actions) gets either the current or maximum array size or clears the array
     * 
     * ### Set/Get pair
     * 
     * The normal 'SET' call method doesn't return the value actually set on the remote
     * device - just an OK (returns caller id) or error code.
     * 
     * If we want to verify and retrive the value that was actually set to the device
     * We can use a SETN then GET. A normal SET-GET RPC pair would need to wait for two
     * replies. Instead we can use SETN (SET with a notify, aka no reply) followed immediately
     * by a GET call.
     * 
     * 
     * ### Sequences and array value streaming (notify)
     * 
     * For sequence arrays, the client can send a stream of array
     * notifications like `0prop`, `+prop`, `+prop`, ... to clear then start
     * setting the array. Array setting can be speedy because the client
     * doesn't need to wait for replies to each array optimizatoin.
     * Instead, the client can check on the progress every, say,
     * 20th value with `#prop`. The results should match the number of 
     * array values sent so far. A final `#prop` query at the end can verify
     * the array was properly filled. The number of consecutive values to
     * send safely will depend on the size of the transmission buffer and 
     * the MessagePack'd size of each json notification call.
     * 
     * ### Server decoding
     * 
     * Lambda methods in the server's dispatch map can make the process
     * of routing codes simpler with little code bloat. The server can
     * hard-code each coded method call with a series of key/lambda function
     * pairs. For example in pseudo code, a property with a value and
     * possible sequence might be coded as lambda captures (pseudocode):
     * @code 
     *  map<String,function> dispatch_map {
     *      {"?prop", call<int>(     [&prop_]()->int            { return prop_; })},
     *      {"!prop", call<void,int>([&prop_](int val)          { prop_ = val; })},
     *      {"^prop", call<int>(     [&pseq_max_]()->int        { return pseq_max_; })},
     *      {"#prop", call<int>(     [&pseq_count_]()->int      { return pseq_count_; })},
     *      {"0prop", call<int>(     [&pseq_count_]()->int      { pseq_count_ = 0; return 0; })},
     *      {"+prop", call<void,int>([&pseq_,&acount_](int val) { pseq_[pseq_count_] = val; })}
     *  };
     * @endcode 
     * Future version of remote dispatch might incorporate auto-decoding 
     * dispatch for sequencable/array properties at the server level.
     * 
     * ## Extra parameters
     * 
     * Properties may have extra call parameters for routing the command
     * to the appropriate place. For example, a short `channel` parameter
     * might be used to route a property to the appropriate DAC channel
     * or a `pin` parameter could indicate a digital I/O pin to set.
     * 
     * The MM driver client is responsible for populating these extra
     * values during the `call` dispatch command and the server is
     * responsible for decoding the extra parameter and taking the
     * appropriate action. The client/server RPC dispatch mechanism has
     * variadic template arguments for precisely this reason!
     * 
     * ## Transforimg properties
     * 
     * Some properties need different types for the client and server.
     * For example, the client MM device might want to set analog ouput
     * as a floating-point number, but the remote device DAC only takes
     * 16-bit integers. Or the client device uses `state` strings but
     * the remote device expects numeric `enum` state values.
     * 
     * A transforming property allows the client device to add a lambda
     * function (with possible [&] captures) to transform the property
     * from the client type to the required server type on-the-fly before
     * sending the remote property.
     * 
     ***********************************************************************/
namespace rdl {

    //enum struct codes : char {
    //    get      = '?',
    //    set      = '!',
    //    act      = '*',
    //    notify   = '>',
    //    max_size = '^',
    //    size     = '#',
    //    clear    = '0',
    //    add      = '+'
    //};

    /** Throw an error during createRemoteProp_impl on bad communication at creation? */
    const bool CREATE_FAILS_IF_ERR_COMMUNICATION = false;

    const size_t BSIZE = rdl::BUFFER_SIZE;

    template <class DeviceT, typename PropT, typename RemoteT, typename... ExT>
    class RemoteProp_Base : public DeviceProp_Base<DeviceT, PropT> {
        using BaseT   = DeviceProp_Base<DeviceT, PropT>;
        using ActionT = MM::ActionT<RemoteProp_Base<DeviceT, PropT, RemoteT>>;
        using ClientT = jsonclient<rdl::StreamAdapter, std::string, BSIZE>;

     protected:
        RemoteProp_Base() : client_(nullptr) {}
        ClientT* client_;
        delegate get_delegate_;
        delegate set_delegate_;
        delegate do_delegate_;
        delegate array_delegate_;

     protected:
        /**	Link the property to the device through the stream and initialize from the propInfo.
	    * **THIS IS THE PRIMARY ENTRY POINT for creating remote properties.**
		*/
        int create(DeviceT* device, ClientT* client, const PropInfo<PropT>& propInfo) {
            client_ = client;

            MM::ActionFunctor* action = new ActionT(this, &RemoteProp_Base<DeviceT, PropT>::OnExecute);

            bool useInitialValue = false;
            //if (readOnly && cmds_.cmdGet()) {
            //    typename ProtoT::StreamGuard monitor(pProto_);
            //    // We do not have an initial value and must retrieve and cached the value from the device
            //    if ((ret = getValue(BaseT::cachedValue_)) != DEVICE_OK && CREATE_FAILS_IF_ERR_COMMUNICATION) {
            //        return ERR_COMMUNICATION;
            //    }
            //    useInitialValue = false;
            //} else if (cmds_.propInfo()) {
            //    useInitialValue = true;
            //}
            try {
                ASSERT_TRUE(propInfo.brief() != nullptr || !propInfo.brief().empty()), ERROR_JSON_METHOD_NOT_FOUND);
                // set the property on the remote device if possible
                std::string method = "?" + BaseT::brief_;
                PropT val;
                ASSERT_OK(client_->call<RetT<PropT>>(method.c_str(), val));
                ASSERT_OK(BaseT::createAndLinkProp(device, propInfo, action, propInfo.isReadOnly(), useInitialValue));
                BaseT::cachedValue_ = val;
                return DEVICE_OK;
            } catch (DeviceResultException e) {
                // Create an "ERROR" property if assert was turned off during compile.
                device->CreateProperty(propInfo.name(), "CreateProperty ERROR: read-write property did not assertReadOnly", MM::String, true);
                return DEVICE_INVALID_PROPERTY;
            }
        }

        /** Sets the Remote Device Property, which updates the getCachedValue */
        virtual int set_impl(const PropT value) {
            int ret = set_delegate.call<RetT<int>,std::string,PropT)(method_name('!'), value);
            if (ret != DEVICE_OK) 
                return ret;
            cachedValue_ = value;
            return notifyChange(value);
        }

        /** Get the device property directly */
        virtual int get(PropT& value) const {
            int ret = set_delegate.call<RetT<int>,std::string,PropT&)(method_name('?'), value);
            if (ret != DEVICE_OK) 
                return ret;
            cachedValue_ = value;
            return notifyChange(value);
        };

        /** Get the cached property value (last call to set()) */
        virtual int getCached(PropT& value) const {
            value = cachedValue_;
            return DEVICE_OK;
        }

        /*******************************************************************
        * Property getting/setting - internal
        *******************************************************************/
        /** Get the value from the remote. Derived classes may override. */
        virtual int getValue(PropT& __val) {

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
        virtual int setRemote_impl(const PropT& __val) {
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
        virtual int getRemoteSequenceSize_impl(hprot::prot_size_t& __size) const {
            __size = getRemoteArrayMaxSizeH<PropT>(cmds_.cmdSetSeq());
            return DEVICE_OK;
        }

        /** Set a remote sequence. Derived classes may override. */
        virtual int setRemoteSequence_impl(const std::vector<std::string> __sequence) {
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
        virtual int startRemoteSequence_impl() {
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
        virtual int stopRemoteSequence_impl() {
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

        /*
        * Called by the properties update method.
        * This is the main Property update routine. 
        * 
        * See DeviceBase::OnLabel for example
        */
        virtual int OnExecute(MM::PropertyBase* pProp, MM::ActionT eAct) override {
            int ret;
            if (eAct == MM::BeforeGet) {
                PropT temp;
                if ((ret = get(temp)) != DEVICE_OK) { return ret; }
                cachedValue_ = temp;
                Assign(*pprop, temp);
            } else if (!isReadOnly && eAct == MM::AfterSet) {
                PropT temp;
                Assign(temp, *pprop);
                if ((ret = set(temp)) != DEVICE_OK) {
                    return ret;
                }
                return notifyChange(temp);
            } else if (isSequencable_ && eAct == MM::IsSequenceable) {
                hprot::prot_size_t maxSize;
                if ((result = getRemoteSequenceSize_impl(maxSize)) != DEVICE_OK) {
                    return result;
                }
                // maxSize will be zero if there was no setSeqCommand
                // or an error occurred. SetSequencable(0) indicates
                // that the property cannot be sequenced
                pProp->SetSequenceable(maxSize);
            } else if (cmds_.cmdSetSeq() && eAct == MM::AfterLoadSequence) {
                std::vector<std::string> sequence = pProp->GetSequence();
                if ((result = setRemoteSequence_impl(sequence)) != DEVICE_OK) {
                    return result;
                }
            } else if (cmds_.cmdSetSeq() && eAct == MM::StartSequence) {
                if ((result = startRemoteSequence_impl()) != DEVICE_OK) {
                    return result;
                }
            } else if (cmds_.cmdSetSeq() && eAct == MM::StopSequence) {
                if ((result = stopRemoteSequence_impl()) != DEVICE_OK) {
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
            return createRemoteProp_impl(device, stream, propInfo, __cmds);
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
            return createRemoteProp_impl(device, stream, propInfo, __cmds);
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
            return createRemoteProp_impl(device, stream, propInfo, __cmds);
        }

        std::vector<PropT> GetRemoteArray() {
            return getRemoteArrayH<PropT>(RemoteProp_Base<PropT, DeviceT, HubT>::cmds_.cmdGetSeq());
        }

        /** Get the maximum size of the remote sequence. */
        int getRemoteSequenceSize(hprot::prot_size_t& __size) const {
            return getRemoteSequenceSize_impl(__size);
        }

        /** Set a remote sequence. */
        int setRemoteSequence(const std::vector<std::string> __sequence) {
            return setRemoteSequence_impl(__sequence);
        }

        /** Start the remote sequence. */
        int startRemoteSequence() {
            return startRemoteSequence_impl();
        }

        /** Stop the remote sequence. */
        int stopRemoteSequence() {
            return stopRemoteSequence_impl();
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
            return createRemoteProp_impl(device, stream, propInfo, __cmds);
        }
    };

    #if OLD_DeviceHexProtocol_Detection_Code
    /** Used by DEV::DetectDevice to determine if a given serial port is actively
			connected to a valid slave device. 
		
			Pseudo-code for the detection process
			\code{.cpp}
			tryStream(DEV* __target, std::string __stream, long __baudRate) 
			{
				... // Call a series of methods that boil down to __target->setupStreamPort(__stream);
				this->startProtocol(__target, __stream);
				this->purgeComPort();
				int ret = this->testProtocol();
				this->endProtocol();
				if (ret == DEVICE_OK) {
					... // cleanup __target
					return MM::CanCommunicate;
				} else {
					... // cleanup __target
					return MM::MM::CanNotCommunicate;
				}
			}
			\endcode

			\warning endProtocol() is called after testProtocol(), so the protocol functions will no longer work.

			@param __target	pointer to device to check
			@param __stream serial port name to check, usually taken from some preInit "port" property.
			@param __baudRate baud-rate to try (must be same as Arudino Serial.begin(baudRate) setting.
			healthy slave device on this __stream.
			@return @see MM::DeviceDetectionStatus
		*/
    MM::DeviceDetectionStatus tryStream(DEV* __target, std::string __stream, long __baudRate) {
        BaseClass::StreamGuard(this);
        MM::DeviceDetectionStatus result = MM::Misconfigured;
        char defaultAnswerTimeout[MM::MaxStrLength];
        try {
            // convert stream name to lower case
            std::string streamLowerCase = __stream;
            std::transform(streamLowerCase.begin(), streamLowerCase.end(), streamLowerCase.begin(), ::tolower);
            if (0 < streamLowerCase.length() && 0 != streamLowerCase.compare("undefined") && 0 != streamLowerCase.compare("unknown")) {
                const char* streamName = __stream.c_str();
                result                 = MM::CanNotCommunicate;

                MM::Core* core = accessor::callGetCoreCallback(__target);

                // record the default answer time out
                core->GetDeviceProperty(streamName, MM::g_Keyword_AnswerTimeout, defaultAnswerTimeout);

                // device specific default communication parameters for Arduino
                core->SetDeviceProperty(streamName, MM::g_Keyword_BaudRate, std::to_string(__baudRate).c_str());
                core->SetDeviceProperty(streamName, MM::g_Keyword_DataBits, g_SerialDataBits);
                core->SetDeviceProperty(streamName, MM::g_Keyword_Parity, g_SerialParity);
                core->SetDeviceProperty(streamName, MM::g_Keyword_StopBits, g_SerialStopBits);
                core->SetDeviceProperty(streamName, MM::g_Keyword_Handshaking, g_SerialHandshaking);
                core->SetDeviceProperty(streamName, MM::g_Keyword_AnswerTimeout, g_SerialAnswerTimeout);
                core->SetDeviceProperty(streamName, MM::g_Keyword_DelayBetweenCharsMs, g_SerialDelayBetweenCharsMs);
                MM::Device* pS = core->GetDevice(__target, streamName);

                pS->Initialize();

                // The first second or so after opening the serial port, the Arduino is
                // waiting for firmwareupgrades.  Simply sleep 2 seconds.
                CDeviceUtils::SleepMs(2000);
                startProtocol(__target, __stream);
                purgeComPort();
                // Try the detection function
                int ret = testProtocol();
                if (ret == DEVICE_OK) {
                    // Device was detected!
                    result = MM::CanCommunicate;
                } else {
                    // Device was not detected. Keep result = MM::CanNotCommunicate
                    accessor::callLogMessageCode(__target, ret, true);
                }
                endProtocol();
                pS->Shutdown();
                // always restore the AnswerTimeout to the default
                core->SetDeviceProperty(streamName, MM::g_Keyword_AnswerTimeout, defaultAnswerTimeout);
            }
        } catch (...) {
            accessor::callLogMessage(__target, "Exception in DetectDevice tryStream!", false);
        }
        return result;
    }
    #endif

}; // namespace rdl

#endif // __REMOTEPROP_H__
