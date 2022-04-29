#pragma once

#ifndef __REMOTEPROP_H__
    #define __REMOTEPROP_H__

    #include "DeviceError.h"
    #include "DeviceProp.h"
    #include "DevicePropHelpers.h"
    #include "StreamAdapter.h"
    #include "../rdl/Delegate.h"
    #include "../rdl/JsonDispatch.h"
    #include <Stream.h>
    #include <tuple>

/************************************************************************
     * # Remote properties
     * 
     * ## Briefs and Property Codes
     * 
     * Property get/set/sequencing rely on JsonDelegate meth_str names with
     * special prepended codes. The remote property name should be a short,
     * unique character sequence like "prop". We will call this abbreviation
     * of the property name the property `brief`.
     * 
     * We prepend a single-character opcode to the property brief to denote
     * a standard property operation. The server is responsible for
     * dispatching the coded brief to the appropriate function. A possible
     * mechanism is detailed below, but dispatch tables are flexible and
     * can use pure callback functions, class methods, or lambda with capture
     * 
     *   > NOTE: codes are prepended rather than appended to make string
     *   > matching terminate earlier during dispatch map meth_str lookup.
     *   > `brief` meth_str tags should be kept to a few characters for the same
     *   > reason. Even a fast hash-map will need to loop through the entire
     *   > string to compute the hash-value for lookup. So use a brief meth_str
     *   > tag like "dv" rather than "MyDACOutputValueInVolts".
     * 
     * |opcode| operation                          |meth[^1]| server signature[^2]                      |
     * |:----:|:-----------------------------------|:-----:|:-------------------------------------------|
     * |  ?   | GET value                          | get   | call<T,EX...>("?brief",ex...)->T           |
     * |  !   | SET value                          | set   | call<void,T,EX...>("!brief",t,ex...)       |
     * |  !   | NSET value - no reply              | set   | notify<void,T,EX...>("!brief",t,ex...)     |
     * |  *   | ACT task                           | act   | call<void,EX...>("*brief",ex...)           |
     * |  *   | NOTIFY task (DO without response)  | act   | notify<void,EX...>("*brief",ex...)         |
     * |  --  | ==== SEQUENCE/ARRAY COMMANDS ====  | --    | --                                         |
     * |  ^   | GET maximum size of seq array      | array | call<long,EX...>("^brief",ex...)->long |
     * |  #   | GET number of values in seq array  | array | call<long,EX...>("#brief",ex...)->long |
     * |  0   | CLEAR seq array                    | array | notify<long,EX...>("0brief",ex...)->dummy|
     * |  +   | ADD value to sequence array        | set   | notify<void,T,EX...>("+brief",ex...)       |
     * |  *   | ACT task doubles as start seq.     | act   | call<void,EX...>("*brief",ex...)           |
     * |  *   | NOTIFY task to start seq.          | act   | notify<void,EX...>("*brief",ex...)         |
     * |  ~   | STOP sequence                      | act   | call<void,EX...>("~brief",ex...)           |
     * |  ~   | STOP sequence                      | act   | notify<void,EX...>("~brief",ex...)         |
     * 
     * [^1]: meth is the client meth_str whose parameters match the call/notify signature
     * [^2]: Signature of the server meth_str. T is the property type on the device, EX... are an 
     * optional set of extra parameters such as channel number
     * 
     * ### Client transform/dispatch methods
     * 
     * From the signature table above, we need four local methods for transforming MM Properties
     * into eventual RPC calls on the server. The client meth_str might also transform the
     * MM::PropertyType into the type T required by the server. Each meth_str type includes
     * an optional set of compile-time extra parameters such as channel number, pin number, etc.
     * What the server does with this iinformation depends on the meth_str opcode.
     * 
     * - get: gets the remote property value
     * - set: sets the remote property value
     * - act (action): performs some task associated with the property and check status. Can be called or notified.
     * - array: (array actions) gets either the current or maximum array size or clears the array
     * 
     * ### Set/Get pair and volatile remote properties
     * 
     * The normal 'SET' call meth_str doesn't return the value actually set on the remote
     * device - just an OK (returns caller id) or error number.
     * 
     * If we want to verify and retrive the value that was actually set to the device
     * We can use a NSET then GET. A normal SET-GET RPC pair would need to wait for two
     * replies, one from the SET call, one from the GET call. Instead we can use 
     * NSET (notify-SET, i.e. no reply) followed immediately by a GET call.
     * 
     * A **volatile** remote property can cange behind-the-scenes. We cannot
     * rely on a cached value and the remote property might not preserve the
     * exact value of a SET operation. Volatile properties must:
     * - Always Use NSET-GET pairs when setting
     * - Always use GET and never use cached values.
     * 
     * ### Sequences and array value streaming (notify)
     * 
     * For sequence arrays, the client can send a stream of array notifications
     * Use `0prop` to clear then a string of `+prop`, `+prop`, ... to add values
     * to the array in order. Array setting can be speedy because the client
     * doesn't need to wait for replies to each array optimizatoin.
     * Instead, the client can check on the progress every, say,
     * 20th value with a `#prop` GET call. The results should match the number of 
     * array values sent so far. A final `#prop` GET call at the end can verify
     * the array was properly filled. The number of consecutive values to
     * send safely will depend on the size of the transmission buffer and 
     * the MessagePack'd size of each json notification call.
     * 
     * Clients should first send a `^prop` GET call to query the maximum array
     * size on the remote device.
     * 
     * ### Server decoding
     * 
     * Lambda methods in the server's dispatch map can make the process
     * of routing opcodes simpler. The server can hard-code each coded meth_str 
     * call with a series of key/lambda function pairs. 
     * 
     * For example in pseudo-code, a property with a value and
     * possible sequence might be coded as lambda captures (pseudocode):
     * @code
     *  map<String,function> dispatch_map {
     *      {"?prop", call<int>(     [&prop_]()->int            { return prop_; })},
     *      {"!prop", call<void,int>([&prop_](int val)          { prop_ = val; })},
     *      {"^prop", call<int>(     [&pseq_max_]()->int        { return pseq_max_; })},
     *      {"#prop", call<int>(     [&pseq_count_]()->int      { return pseq_count_; })},
     *      {"0prop", call<int>(     [&pseq_count_]()->int      { pseq_count_ = 0; return 0; })},
     *      {"+prop", call<void,int>([&pseq_,&acount_](int val) { pseq_[pseq_count_++] = val; })}
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
namespace rdlmm {

    #ifndef REMOTE_PROP_ARRAY_CHUNK_SIZE
        #define REMOTE_PROP_ARRAY_CHUNK_SIZE 10
    #endif

    template <class DeviceT, typename LocalT, typename RemoteT, typename... ExT>
    class RemoteProp_Base : public DeviceProp_Base<DeviceT, LocalT> {
     protected:
        using BaseT          = DeviceProp_Base<DeviceT, LocalT>;
        using ThisT          = RemoteProp_Base<DeviceT, LocalT, RemoteT, ExT...>;
        using ActionT        = MM::Action<ThisT>;
        using StreamAdapterT = HubStreamAdapter<DeviceT, arduino::Stream>;
        using ClientT        = rdl::json_client<StreamAdapterT, StreamAdapterT, std::string, JSONRCP_BUFFER_SIZE>;
        using ExtrasT        = std::tuple<ExT...>;

        RemoteProp_Base() : client_(nullptr) {}
        ClientT* client_;
        ExtrasT extra_;
        delegate<RetT<RemoteT>, LocalT> to_remote_fn_;
        delegate<RetT<LocalT>, RemoteT> to_local_fn_;
        long cached_max_seq_size_;

     public:
        /*	Link the property to the device and initialize from the propInfo. */
        virtual int create(DeviceT* device, ClientT* client, const PropInfo<LocalT>& propInfo, ExT... args) {
            client_       = client;
            brief_        = propInfo.brief(); // copy early for get/set before createAndLinkProp()
            extra_        = std::tie(args...);
            cached_max_seq_size_ = -1;  // trigger a get max size at the beginning
            to_remote_fn_ = delegate<RetT<RemoteT>, LocalT>::create([](LocalT v) { return static_cast<RemoteT>(v); });
            to_local_fn_  = delegate<RetT<LocalT>, RemoteT>::create([](RemoteT v) { return static_cast<LocalT>(v); });

            if (propInfo.hasInitialValue()) {
                LocalT v = propInfo.initialValue();
                set_impl(v);
            } else {
                LocalT v;
                get_impl(v);
                cachedValue_ = v;
            }
            MM::ActionFunctor* pAct = new ActionT(this, &ThisT::OnExecute);
            return createAndLinkProp(device, propInfo, pAct);
        }

     protected:
        virtual PropInfo<LocalT> checkPropInfo(const PropInfo<LocalT>& propInfo) override {
            return propInfo;
            //PropInfo<PropT> checked = propInfo;
            //if (initFromCache_) {
            //    checked.witoutInitialValue();
            //}
            //return checked;
        }
        std::string meth_str(const char opcode) const {
            std::string res(1, opcode);
            res.append(brief_);
            return res;
        }

        template <typename T>
        std::tuple<T, ExT...> withextras(T t) const {
            return std::tuple_cat(std::tie(t), extra_);
        }

        ExtrasT extras() const {
            return extra_;
        }

        virtual int set_impl(const LocalT localv) override {
            RemoteT remotev = to_remote_fn_(localv);
            int ret;
            if (isVolatile_) {
                // NSET-GET pair
                ret = client_->notify_tuple(meth_str('!').c_str(), withextras(remotev));
                if (ret != ERROR_OK)
                    return ret;
                ret = client_->call_get_tuple<RemoteT>(meth_str('?').c_str(), remotev, extras());
                if (ret != ERROR_OK)
                    return ret;
                cachedValue_ = to_local_fn_(remotev);
            } else {
                // SET call
                ret = client_->call_tuple(meth_str('!').c_str(), withextras(remotev));
                if (ret != ERROR_OK)
                    return ret;
                cachedValue_ = localv;
            }
            return DEVICE_OK;
        }

        ///** Get the value before updating the property. Derived classes may override. */
        virtual int get_impl(LocalT& localv) override {
            RemoteT remotev = 0;
            int ret         = client_->call_get_tuple<RemoteT, ExtrasT>(meth_str('?').c_str(), remotev, extra_);
            if (ret != ERROR_OK)
                return ret;
            localv       = to_local_fn_(remotev);
            cachedValue_ = localv;
            return DEVICE_OK;
        }

        ///** Get the value before updating the property. Derived classes may override. */
        virtual int getCached_impl(LocalT& localv) override {
            if (isVolatile_)
                return get_impl(localv);
            localv = cachedValue_;
            return ERROR_OK;
        }

        virtual int OnExecute(MM::PropertyBase* pprop, MM::ActionType action) override {
            int ret;
            if (action == MM::BeforeGet) {
                LocalT temp, oldv = cachedValue_;
                if ((ret = getCached_impl(temp)) != DEVICE_OK) { return ret; }
                Assign(*pprop, temp);
                if (temp != oldv)
                    return notifyChange(temp);
            } else if (!isReadOnly_ && action == MM::AfterSet) {
                LocalT temp, oldv = cachedValue_;
                Assign(temp, *pprop);
                if ((ret = set_impl(temp)) != DEVICE_OK)
                    return ret;
                if (temp != oldv)
                    return notifyChange(temp);
            } else if (action == MM::IsSequenceable) {
                if (!isSequencable_) {
                    // SetSequencable(0) indicates that the property cannot be sequenced
                    pprop->SetSequenceable(0);
                } else {
                    long max_size = 0;
                    if (cached_max_seq_size_ < 0) {
                        // IsSequenceable is called multiple times throughout a properties lifetime,
                        // often repeately. Rather than calling the remote device again and again,
                        // we ONLY get the max sequence size once and cache it.
                        // **WARNING** assumes each remote property has a static maximum sequence size
                        if ((ret = client_->call_get_tuple<long>(meth_str('^').c_str(), max_size, extras())) != DEVICE_OK)
                            max_size = 0;
                        else
                            cached_max_seq_size_ = max_size;
                    } else {
                        max_size = cached_max_seq_size_;
                    }
                    pprop->SetSequenceable(static_cast<long>(max_size));
                }
            } else if (isSequencable_ && action == MM::AfterLoadSequence) {
                // send the sequence to the device
                std::vector<std::string> sequence = pprop->GetSequence();

                long seqsize    = static_cast<long>(sequence.size());
                long remotesize = 0;
                if ((ret = client_->notify_tuple(meth_str('0').c_str(), extras())) != DEVICE_OK) {
                    return ret;
                }
                for (int i = 0; i < seqsize; i++) {
                    // add the value
                    LocalT localv   = Parse<LocalT>(sequence[i]);
                    RemoteT remotev = to_remote_fn_(localv);
                    if ((ret = client_->notify_tuple(meth_str('+').c_str(), withextras(remotev))) != DEVICE_OK) {
                        return ret;
                    }
                    int size = i+1;
                    if (size % REMOTE_PROP_ARRAY_CHUNK_SIZE == 0 || size == seqsize) {
                        // verify the current size
                        if ((ret = client_->call_get_tuple<long>(meth_str('#').c_str(), remotesize, extras())) != ERROR_OK) {
                            return ret;
                        }
                        if (size != remotesize) {
                            return ERR_WRITE_FAILED;
                        }
                    }
                }
            } else if (isSequencable_ && action == MM::StartSequence) {
                if ((ret = client_->call_tuple(meth_str('*').c_str(), extras())) != DEVICE_OK)
                    return ret;
            } else if (isSequencable_ && action == MM::StopSequence) {
                if ((ret = client_->call_tuple(meth_str('~').c_str(), extras())) != DEVICE_OK)
                    return ret;
            }
            return DEVICE_OK;
        }

        //      /**	Link the property to the device through the stream and initialize from the propInfo.
        //   * **THIS IS THE PRIMARY ENTRY POINT for creating remote properties.**
        //*/
        //      int create(DeviceT* device, ClientT* client, const PropInfo<LocalT>& propInfo) {
        //          client_ = client;

        //          MM::ActionFunctor* action = new ActionT(this, &RemoteProp_Base<DeviceT, LocalT>::OnExecute);

        //          bool useInitialValue = false;
        //          try {
        //              ASSERT_TRUE(propInfo.brief() != nullptr && !propInfo.brief().empty()), ERROR_JSON_METHOD_NOT_FOUND);
        //              // set the property on the remote device if possible
        //              LocalT val;
        //              ASSERT_OK(client_->call<RetT<LocalT>>(meth_str('?').c_str(), val));
        //              ASSERT_OK(BaseT::createAndLinkProp(device, propInfo, action, propInfo.isReadOnly(), useInitialValue));
        //              BaseT::cachedValue_ = val;
        //              return DEVICE_OK;
        //          } catch (DeviceResultException e) {
        //              // Create an "ERROR" property if assert was turned off during compile.
        //              device->CreateProperty(propInfo.name(), "CreateProperty ERROR: read-write property did not assertReadOnly", MM::String, true);
        //              return DEVICE_INVALID_PROPERTY;
        //          }
        //      }
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
    //template <typename LocalT, class DeviceT, class HubT>
    //class SimpleRemoteProp : public RemoteProp_Base<LocalT, DeviceT, HubT> {
    //    typedef hprot::DeviceHexProtocol<HubT> ProtoT;

    //public:
    //   int createRemoteProp(DeviceT* device, ProtoT* stream, const PropInfo<LocalT>& propInfo, CommandSet& __cmds) {
    //       assert(__cmds.propInfo() || __cmds.cmdGet());
    //       return createRemoteProp_impl(device, stream, propInfo, __cmds);
    //   }
    //};

    template <class DeviceT, typename LocalT>
    struct SimpleRemnoteProp : public RemoteProp_Base<DeviceT, LocalT, LocalT> {
     public:
        using BaseT = RemoteProp_Base<DeviceT, LocalT, LocalT>;
        using ThisT = SimpleRemnoteProp<DeviceT, LocalT>;
    };

    /**
	A class to hold a write-only remote property value.

	\ingroup RemoteProp

	Micromanager updates
	the property through the OnExecute member function, which in turn
	sets the value from the device and updates the getCachedValue.
	Get simply returns the last cached value.
	*/
    //template <typename LocalT, class DeviceT, class HubT>
    //class RemoteCachedProp : public RemoteProp_Base<LocalT, DeviceT, HubT> {
    //    typedef hprot::DeviceHexProtocol<HubT> ProtoT;

    // public:
    //    int createRemoteProp(DeviceT* device, ProtoT* stream, const PropInfo<LocalT>& propInfo, CommandSet& __cmds) {
    //        assert(__cmds.propInfo());
    //        return createRemoteProp_impl(device, stream, propInfo, __cmds);
    //    }
    //};

    /**
	A class to hold a sequencable write-only remote property value.

	\ingroup RemoteProp

	Micromanager updates the property through the OnExecute member function,
	which in turn sets the value from the device and updates the getCachedValue.
	Get simply returns the last cached value. This version exposes the sequencable
	helper interface.

	*/
    //template <typename LocalT, class DeviceT, class HubT>
    //class RemoteSequenceableProp : public RemoteProp_Base<LocalT, DeviceT, HubT> {
    //    typedef hprot::DeviceHexProtocol<HubT> ProtoT;

    // public:
    //    int createRemoteProp(DeviceT* device, ProtoT* stream, const PropInfo<LocalT>& propInfo, CommandSet& __cmds) {
    //        assert(__cmds.propInfo() && __cmds.cmdSetSeq() && __cmds.cmdStartSeq() && __cmds.cmdStopSeq());
    //        return createRemoteProp_impl(device, stream, propInfo, __cmds);
    //    }

    //    std::vector<LocalT> GetRemoteArray() {
    //        return getRemoteArrayH<LocalT>(RemoteProp_Base<LocalT, DeviceT, HubT>::cmds_.cmdGetSeq());
    //    }

    //    /** Get the maximum size of the remote sequence. */
    //    int getRemoteSequenceSize(hprot::prot_size_t& __size) const {
    //        return getRemoteSequenceSize_impl(__size);
    //    }

    //    /** Set a remote sequence. */
    //    int setRemoteSequence(const std::vector<std::string> __sequence) {
    //        return setRemoteSequence_impl(__sequence);
    //    }

    //    /** Start the remote sequence. */
    //    int startRemoteSequence() {
    //        return startRemoteSequence_impl();
    //    }

    //    /** Stop the remote sequence. */
    //    int stopRemoteSequence() {
    //        return stopRemoteSequence_impl();
    //    }

    // protected:
    //};

    /**
	A class to hold a read-only remote property value.

	\ingroup RemoteProp

	Micromanager updates
	the property through the OnExecute member function, which in turn
	gets the value from the device and updates the getCachedValue.
	Sets do nothing.
	*/
    //template <typename LocalT, class DeviceT, class HubT>
    //class RemoteReadOnlyProp : public RemoteProp_Base<LocalT, DeviceT, HubT> {
    //    typedef RemoteProp_Base<LocalT, DeviceT, HubT> BaseT;
    //    typedef hprot::DeviceHexProtocol<HubT> ProtoT;

    // public:
    //    int createRemoteProp(DeviceT* device, ProtoT* stream, const PropInfo<LocalT>& propInfo, CommandSet& __cmds) {
    //        assert(__cmds.cmdGet());
    //        return createRemoteProp_impl(device, stream, propInfo, __cmds);
    //    }
    //};

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
