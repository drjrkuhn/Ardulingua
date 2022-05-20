#pragma once

#ifndef __SERVERPROPERTY_H__
    #define __SERVERPROPERTY_H__

    #include "Arraybuf.h"
    #include "JsonDelegate.h"
    #include "std_utility.h"
    #include "sys_PrintT.h"
    #include "sys_StringT.h"

    #define SERVERPROP_LOGGING 0

namespace rdl {

    /************************************************************************
     * Property dispatch methods using Strings for property lookup.
     *
     * The prop_any_base class is the most flexible, allowing extra template
     * parameters for extra property coding.
     *
     ************************************************************************/

    /************************************************************************
     * Flexible virtual function interface to hold a property on a json_server
     *
     * @tparam T        property value type
     * @tparam ExT      extra values such as channel, etc.
     ************************************************************************/
    template <typename T, typename... ExT>
    class prop_any_base {
     public:
        /** Absolute base class type, used for virtual dispatch */
        using RootT = prop_any_base<T, ExT...>;

        prop_any_base(const sys::StringT& brief_name) : brief_(brief_name), logger_(nullptr) {}
        // prop_any_base(const prop_any_base& other) = default;

        bool operator==(const prop_any_base& other) const { return brief_ == other.brief_; }
        bool operator!=(const prop_any_base& other) const { return brief_ != other.brief_; }

        /**
         * Generalized Function signatures for delegates.
         * Signatures are used for dispatch map creation.
         * @tparam DelegateT    either delegate or json_delegate
         */
        struct json_delegates {
            using get    = json_delegate<T, ExT...>;
            using set    = json_delegate<void, const T, ExT...>;
            using array  = json_delegate<long, ExT...>;
            using action = json_delegate<void, ExT...>;
            using flag   = json_delegate<bool, ExT...>;
        };

        ////// DISPATCH INTERFACE //////
        virtual T get(ExT... ex) const             = 0;
        virtual void set(const T value, ExT... ex) = 0;
        virtual long max_size(ExT... ex) const     = 0;
        virtual long size(ExT... ex) const         = 0;
        virtual long clear(ExT... ex)              = 0;
        virtual void add(const T value, ExT... ex) = 0;
        virtual void start(ExT... ex)              = 0;
        virtual void stop(ExT... ex)               = 0;
        virtual bool sequencable(ExT... ex) const  = 0;
        virtual bool read_only(ExT... ex) const    = 0;

        sys::StringT message(const char opcode) { return opcode + brief_; }

        virtual void logger(sys::PrintT* logger) {
            logger_ = logger;
    #if SERVERPROP_LOGGING
            if (logger_) {
                logger_->println(sys::StringT("logging property ") + brief_);
            }
    #endif
        }

     protected:
        sys::StringT brief_;
        sys::PrintT* logger_;
    };

    /**
     * Add a suite of property methods to a dispatch map.
     *
     * @tparam MapT     Type of dispatch map using the std::map interface
     * @tparam RootT Interface type for this property (should be prop_any_type<XXX>)
     *                  derived classes should keep track through the
     *                  derived::RootT type definition.
     *
     * @param map           dispatch map to add to
     * @param prop          property to add
     * @param sequencable   is this property sequencable?
     * @return size_t       number of methods added to the dispatch table
     */
    template <class MapT, class RootT>
    size_t add_to(MapT& map, RootT& prop, bool sequencable, bool read_only) {
        using PairT      = typename MapT::value_type;
        using delsig     = typename RootT::json_delegates;
        size_t startsize = map.size();

        // minimum properties are get and max_size
        map.insert(PairT(
            prop.message('?'), // get
            delsig::get::template create<RootT, &RootT::get>(&prop).stub()));
        map.insert(PairT(
            prop.message('^'), // max_size for sequences, doubles as number of channels
            delsig::array::template create<RootT, &RootT::max_size>(&prop).stub()));
        if (!read_only) {
            map.insert(PairT(
                prop.message('!'), // set
                delsig::set::template create<RootT, &RootT::set>(&prop).stub()));
        }
        if (sequencable) {
            map.insert(PairT(
                prop.message('#'), // current sequence size
                delsig::array::template create<RootT, &RootT::size>(&prop).stub()));
            map.insert(PairT(
                prop.message('0'), // clear sequence array
                delsig::array::template create<RootT, &RootT::clear>(&prop).stub()));
            map.insert(PairT(
                prop.message('+'), // add to sequence array
                delsig::set::template create<RootT, &RootT::add>(&prop).stub()));
            map.insert(PairT(
                prop.message('*'), // start sequence
                delsig::action::template create<RootT, &RootT::start>(&prop).stub()));
            map.insert(PairT(
                prop.message('~'), // stop sequence
                delsig::action::template create<RootT, &RootT::stop>(&prop).stub()));
        }
        return map.size() - startsize;
    }

    /************************************************************************
     * Base to hold a sequencable property value.
     *
     * Servers can derive specialized property handlers from this base.
     * Be sure to add `using BaseT::RootT` to keep track of the root
     * interface class.
     * @code{.cpp}
     * class my_prop : public simple_prop<int,String,100> {
     *   public:
     *     using BaseT = simple_prop<int,String,100>;
     *     using typename BaseT::RootT; // used for dispatch map creation
     * ...
     * @endcode
     *
     * @tparam T        property value type
     ************************************************************************/
    template <typename T>
    class simple_prop : public prop_any_base<T> {
        // TODO: use ATOMIC_BLOCK found in avr-libc <util/atomic.h> or mutex
     public:
        using BaseT = prop_any_base<T>;
        using ThisT = simple_prop<T>;
        using RootT = BaseT; // used for dispatch map creation
        using BaseT::brief_;
        using BaseT::logger_;
        using BaseT::logger;

        simple_prop(simple_prop<T>& lvalue)  = default;
        simple_prop(simple_prop<T>&& rvalue) = default;

        virtual ~simple_prop() {}

        ////// IMPLEMENT INTERFACE //////
        virtual T get() const override {
    #if SERVERPROP_LOGGING
            if (logger_) {
                logger_->println(brief_ + " simple prop get -> " + sys::to_string(value_));
            }
    #endif
            return value_;
        }
        virtual void set(const T value) override {
    #if SERVERPROP_LOGGING
            if (logger_) {
                logger_->println(brief_ + " simple prop set = " + sys::to_string(value));
            }
    #endif
            value_ = value;
        }
        virtual long max_size() const override {
            return sequence_.max_size();
        }
        virtual long size() const override {
            return size_;
        }
        virtual long clear() override {
            size_       = 0;
            next_index_ = 0;
            return 0;
        }
        virtual void add(const T value) override {
            if (size_ < sequence_.max_size())
                sequence_[size_++] = value;
        }
        virtual void start() override {
            next_index_ = 0;
            started_    = true;
        }
        virtual void stop() override {
            started_ = false;
        }
        virtual bool sequencable() const override {
            return sequence_.max_size() > 0;
        }
        virtual bool read_only() const override {
            return read_only_;
        }

     protected:
        simple_prop(const sys::StringT& brief_name, const T initial, bool read_only = false)
            : BaseT(brief_name), value_(initial), read_only_(read_only), next_index_(0),
              size_(0), started_(false), sequence_() {
        }

        T value_;
        bool read_only_;
        volatile long next_index_;
        long size_;
        volatile bool started_;
        arraybuf<T> sequence_;
    };

    template <typename T, long MAX_SEQUENCE_SIZE>
    class static_simple_prop : public simple_prop<T> {
     public:
        static_simple_prop(const sys::StringT& brief_name, const T initial, bool read_only = false)
            : simple_prop<T>(brief_name, initial, read_only) {
            // Initialized after base class
            simple_prop<T>::sequence_ = std::move(static_sequence_);
        }

     protected:
        static_arraybuf<T, MAX_SEQUENCE_SIZE> static_sequence_;
    };

    template <typename T>
    class dynamic_simple_prop : public simple_prop<T> {
     public:
        dynamic_simple_prop(const sys::StringT& brief_name, const T initial, long max_sequence_size, bool read_only = false)
            : simple_prop<T>(brief_name, initial, read_only) {
            simple_prop<T>::sequence_ = std::move(dynamic_arraybuf<T, long>(max_sequence_size));
        }
    };

    /************************************************************************
     * Base to hold an array of simple_prop properties.
     *
     * A second 'int channel' parameter in every call selects the appropriate
     * simple_property_base channel. Create individual channel properties
     * separately and add them to this holder. There is no "remove" method
     * since this is for one-time server method dispatch setup.
     *
     * @tparam T        property value type
     ************************************************************************/
    template <typename T>
    class channel_prop : public prop_any_base<T, int> {
        // TODO: use ATOMIC_BLOCK found in avr-libc <util/atomic.h> or mutex
     public:
        using BaseT = prop_any_base<T, int>;
        using ThisT = channel_prop<T>;
        using ChanPtrT = prop_any_base<T>*; // seq_capacity_ doesn't matter for the reference
        using RooT  = BaseT;             // used for dispatch map creation
        using BaseT::brief_;
        using BaseT::logger_;
        using BaseT::logger;

        /** Copy constructor. Can't be const because of volatile members. */
        channel_prop(channel_prop<T>& lvalue)  = default;
        /** Move constructor. */
        channel_prop(channel_prop<T>&& rvalue) = default;
        /** Create from existing fixed array of channel pointers. */
        channel_prop(const sys::StringT& brief_name, ChanPtrT* chan_buf, int chan_buf_size)
            : BaseT(brief_name), num_channels_(chan_buf_size), channels_(chan_buf, chan_buf_size) {
        }

        virtual ~channel_prop() {}

        int add(ChanPtrT prop) {
            if (num_channels_ + 1 > channels_.max_size()) return num_channels_;
            channels_[num_channels_++] = prop;
            return num_channels_;
        }

        int add(ChanPtrT props[], int nchan) {
            if (num_channels_ + nchan > channels_.max_size()) return num_channels_;
            for (int i = 0; i < nchan; i++)
                add(props[i]);
            return num_channels_;
        }

        ////// IMPLEMENT INTERFACE //////
        virtual T get(int chan) const override {
    #if SERVERPROP_LOGGING
            if (logger_) {
                logger_->println(brief_ + " chan prop get[" + sys::to_string(chan) + "]" + " num_channels_: " + sys::to_string(num_channels_));
            }
    #endif
            if (chan >= 0 && chan < num_channels_) {
                T value = channels_[chan]->get();
    #if SERVERPROP_LOGGING
                if (logger_) {
                    logger_->println(sys::StringT(" -> ") + sys::to_string(value));
                }
    #endif
                return value;
            } else {
                return T();
            }
        }
        virtual void set(const T value, int chan) override {
    #if SERVERPROP_LOGGING
            if (logger_) {
                logger_->print(brief_ + " chan prop set[" + sys::to_string(chan) + "]" + " = " + sys::to_string(value));
                logger_->println(" num_channels_: " + sys::to_string(num_channels_));
            }
    #endif
            if (chan >= 0 && chan < num_channels_) {
                channels_[chan]->set(value);
            }
        }
        /**
         * Gets the maximum sequence size of a single chan or
         * the total number of channels if chan<0
         */
        virtual long max_size(int chan) const override {
            if (chan < 0) {
    #if SERVERPROP_LOGGING
                if (logger_) {
                    logger_->println(brief_ + " chan prop max_size[all] -> " + sys::to_string(num_channels_));
                }
    #endif
                return num_channels_;
            } else if (chan < num_channels_) {
                long size = channels_[chan]->max_size();
    #if SERVERPROP_LOGGING
                if (logger_) {
                    logger_->println(brief_ + " chan prop max_size[" + sys::to_string(chan) + "]" + " -> " + sys::to_string(size));
                }
    #endif
                return size;
            } else {
                return 0;
            }
        }
        virtual long size(int chan) const override {
            if (chan >= 0 && chan < num_channels_) {
                long size = channels_[chan]->size();
    #if SERVERPROP_LOGGING
                if (logger_) {
                    logger_->println(brief_ + " chan prop size[" + sys::to_string(chan) + "]" + " -> " + sys::to_string(size));
                }
    #endif
                return size;
            } else {
                return 0;
            }
        }
        virtual long clear(int chan) override {
            if (chan >= 0 && chan < num_channels_) {
    #if SERVERPROP_LOGGING
                if (logger_) {
                    logger_->println(brief_ + " chan prop clear[" + sys::to_string(chan) + "]");
                }
    #endif
                return channels_[chan]->clear();
            } else {
                return 0;
            }
        }
        virtual void add(const T value, int chan) override {
            if (chan >= 0 && chan < num_channels_) {
    #if SERVERPROP_LOGGING
                if (logger_) {
                    logger_->println(brief_ + " chan prop add[" + sys::to_string(chan) + "]" + " -> " + sys::to_string(value));
                }
    #endif
                channels_[chan]->add(value);
            }
        }
        virtual void start(int chan) override {
            if (chan < 0) {
    #if SERVERPROP_LOGGING
                if (logger_) {
                    logger_->println(brief_ + " chan prop start[all]");
                }
    #endif
                for (int i = 0; i < num_channels_; i++) {
                    channels_[i]->start();
                }
            } else if (chan < num_channels_) {
    #if SERVERPROP_LOGGING
                if (logger_) {
                    logger_->println(brief_ + " chan prop start[" + sys::to_string(chan) + "]");
                }
    #endif
                channels_[chan]->start();
            }
        }
        virtual void stop(int chan) override {
            if (chan < 0) {
    #if SERVERPROP_LOGGING
                if (logger_) {
                    logger_->println(brief_ + " chan prop stop[all]");
                }
    #endif
                for (int i = 0; i < num_channels_; i++) {
                    channels_[i]->stop();
                }
            } else if (chan < num_channels_) {
    #if SERVERPROP_LOGGING
                if (logger_) {
                    logger_->println(brief_ + " chan prop stop[" + sys::to_string(chan) + "]");
                }
    #endif
                channels_[chan]->stop();
            }
        }

        /** call with chan < 0 to check if all channels are sequencable  */
        virtual bool sequencable(int chan) const override {
            bool seqable = true;
            if (chan < 0) { // check all channels
                for (int i = 0; i < num_channels_; i++) {
                    if (!channels_[i]->sequencable()) {
                        seqable = false;
                        break;
                    }
                }
            } else if (chan < num_channels_) {
                seqable = channels_[chan]->sequencable();
            }
    #if SERVERPROP_LOGGING
            if (logger_) {
                logger_->println(brief_ + " chan prop sequencable[" + sys::to_string(chan) + "]" + " -> " + (seqable ? "true" : "false"));
            }
    #endif
            return seqable;
        }

        /** call with chan < 0 to check if any channel is read_only  */
        virtual bool read_only(int chan) const override {
            bool ronly = false;
            if (chan < 0) { // check all channels
                for (int i = 0; i < num_channels_; i++) {
                    if (channels_[i]->read_only()) {
                        ronly = true;
                        break;
                    }
                }
            } else if (chan < num_channels_) {
                ronly = channels_[chan]->read_only();
            }
    #if SERVERPROP_LOGGING
            if (logger_) {
                logger_->println(brief_ + " chan prop read_only[" + sys::to_string(chan) + "]" + " -> " + (ronly ? "true" : "false"));
            }
    #endif
            return ronly;
        }

     protected:
        channel_prop(const sys::StringT& brief_name)
            : BaseT(brief_name), num_channels_(0) {
        }

        int num_channels_;
        arraybuf<ChanPtrT, int> channels_;
    };

    template <typename T, int MAX_CHANNELS>
    class static_channel_prop : public channel_prop<T> {
     public:
        static_channel_prop(const sys::StringT& brief_name)
            : channel_prop<T>(brief_name) {
            channel_prop<T>::channels_ = std::move(static_channels_);
        }

     protected:
        static_arraybuf<typename channel_prop<T>::ChanPtrT, MAX_CHANNELS, int> static_channels_;
    };

    template <typename T>
    class dynamic_channel_prop : public channel_prop<T> {
     public:
        dynamic_channel_prop(const sys::StringT& brief_name, int max_channels)
            : channel_prop<T>(brief_name) {
            channel_prop<T>::channels_ = std::move(dynamic_arraybuf<typename channel_prop<T>::ChanPtrT, int>(max_channels));
        }
    };

};

#endif // __SERVERPROPERTY_H__