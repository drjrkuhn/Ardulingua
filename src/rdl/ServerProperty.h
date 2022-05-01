#pragma once

#ifndef __SERVERPROPERTY_H__
    #define __SERVERPROPERTY_H__

    #include "JsonDelegate.h"
    #include "sys_PrintT.h"
    #include "sys_StringT.h"

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
        prop_any_base(prop_any_base& other) = default;

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

        void logger(sys::PrintT* logger) {
            logger_ = logger;
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
     * class my_prop : public simple_prop_base<int,String,100> {
     *   public:
     *     using BaseT = simple_prop_base<int,String,100>;
     *     using typename BaseT::RootT; // used for dispatch map creation
     * ...
     * @endcode
     *
     * @tparam T        property value type
     * @tparam MAX_SEQ_SIZE maximum sequence size (at compile time)
     ************************************************************************/
    template <typename T, long MAX_SEQ_SIZE, bool READONLY = false>
    class simple_prop_base : public prop_any_base<T> {
        // TODO: use ATOMIC_BLOCK found in avr-libc <util/atomic.h> or mutex
     public:
        using BaseT = prop_any_base<T>;
        using ThisT = simple_prop_base<T, MAX_SEQ_SIZE, READONLY>;
        using typename BaseT::RootT; // used for dispatch map creation
        using BaseT::brief_;
        using BaseT::logger_;

        simple_prop_base(const sys::StringT& brief_name, const T initial, bool sequencable = false)
            : BaseT(brief_name),
              value_(initial), sequencable_(sequencable), max_size_(MAX_SEQ_SIZE),
              next_index_(0), size_(0), started_(false) {
        }

        ////// IMPLEMENT INTERFACE //////
        virtual T get() const override {
            if (logger_) {
                logger_->println(brief_ + " simple prop get -> " + sys::to_string(value_));
            }
            return value_;
        }
        virtual void set(const T value) override {
            if (logger_) {
                logger_->println(brief_ + " simple prop set = " + sys::to_string(value));
            }
            value_ = value;
        }
        virtual long max_size() const override {
            if (sequencable())
                return max_size_;
            else
                return 0;
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
            if (size_ >= max_size_ || !sequencable())
                return;
            values_[size_++] = value;
        }
        virtual void start() override {
            next_index_ = 0;
            started_    = true;
        }
        virtual void stop() override {
            started_ = false;
        }
        virtual bool sequencable() const override {
            return sequencable_;
        }
        virtual bool read_only() const override {
            return READONLY;
        }

     protected:
        T value_;
        bool sequencable_;
        const long max_size_;
        volatile long next_index_;
        long size_;
        volatile bool started_;
        T values_[MAX_SEQ_SIZE];
    };

    /************************************************************************
     * Base to hold an array of simple_prop_base properties.
     *
     * A second 'int channel' parameter in every call selects the appropriate
     * simple_property_base channel. Create individual channel properties
     * separately and add them to this holder. There is no "remove" method
     * since this is for one-time server method dispatch setup.
     *
     * @tparam T        property value type
     * @tparam StringT     strings used by the server (std::string or arudino::String)
     * @tparam MAX_SEQ_SIZE maximum sequence size (at compile time)
     ************************************************************************/
    template <typename T, int MAX_CHANNELS>
    class channel_prop_base : public prop_any_base<T, int> {
        // TODO: use ATOMIC_BLOCK found in avr-libc <util/atomic.h> or mutex
     public:
        using BaseT = prop_any_base<T, int>;
        using ThisT = channel_prop_base<T, MAX_CHANNELS>;
        using ChanT = prop_any_base<T>; // MAX_SEQ_SIZE doesn't matter for the reference
        using typename BaseT::RootT;    // used for dispatch map creation
        using BaseT::logger_;
        using BaseT::brief_;

        channel_prop_base(const sys::StringT& brief_name)
            : BaseT(brief_name), num_channels_(0) {
        }
        channel_prop_base(const sys::StringT& brief_name, ChanT* props[], int nchan)
            : BaseT(brief_name), num_channels_(0) {
            add(props, nchan);
        }

        int add(ChanT* prop) {
            if (num_channels_ + 1 > MAX_CHANNELS) return num_channels_;
            channels_[num_channels_++] = prop;
            return num_channels_;
        }

        int add(ChanT* props[], int nchan) {
            if (num_channels_ + nchan > MAX_CHANNELS) return num_channels_;
            for (int i = 0; i < nchan; i++)
                add(props[i]);
            return num_channels_;
        }

        ////// IMPLEMENT INTERFACE //////
        virtual T get(int chan) const override {
            if (chan >= 0 && chan < num_channels_) {
                T value = channels_[chan]->get();
                if (logger_) {
                    logger_->println(brief_ + " chan prop get[" + sys::to_string(chan) + "]" + " -> " + sys::to_string(value));
                }
                return value;
            } else {
                return T();
            }
        }
        virtual void set(const T value, int chan) override {
            if (chan >= 0 && chan < num_channels_) {
                if (logger_) {
                    logger_->println(brief_ + " chan prop set[" + sys::to_string(chan) + "]" + " = " + sys::to_string(value));
                }
                channels_[chan]->set(value);
            }
        }
        /**
         * Gets the maximum sequence size of a single chan or
         * the total number of channels if chan<0
         */
        virtual long max_size(int chan) const override {
            if (chan < 0) {
                if (logger_) {
                    logger_->println(brief_ + " chan prop max_size[all] -> " + sys::to_string(num_channels_));
                }
                return num_channels_;
            } else if (chan < num_channels_) {
                long size = channels_[chan]->max_size();
                if (logger_) {
                    logger_->println(brief_ + " chan prop max_size[" + sys::to_string(chan) + "]" + " -> " + sys::to_string(size));
                }
                return size;
            } else {
                return 0;
            }
        }
        virtual long size(int chan) const override {
            if (chan >= 0 && chan < num_channels_) {
                long size = channels_[chan]->size();
                if (logger_) {
                    logger_->println(brief_ + " chan prop size[" + sys::to_string(chan) + "]" + " -> " + sys::to_string(size));
                }
                return size;
            } else {
                return 0;
            }
        }
        virtual long clear(int chan) override {
            if (chan >= 0 && chan < num_channels_) {
                if (logger_) {
                    logger_->println(brief_ + " chan prop clear[" + sys::to_string(chan) + "]");
                }
                return channels_[chan]->clear();
            } else {
                return 0;
            }
        }
        virtual void add(const T value, int chan) override {
            if (chan >= 0 && chan < num_channels_) {
                if (logger_) {
                    logger_->println(brief_ + " chan prop add[" + sys::to_string(chan) + "]" + " -> " + sys::to_string(value));
                }
                channels_[chan]->add(value);
            }
        }
        virtual void start(int chan) override {
            if (chan < 0) {
                if (logger_) {
                    logger_->println(brief_ + " chan prop start[" + sys::to_string(chan) + "]");
                }
                for (int i = 0; i < num_channels_; i++) {
                    channels_[i]->start();
                }
            } else if (chan < num_channels_) {
                if (logger_) {
                    logger_->println(brief_ + " chan prop start[all]");
                }
                channels_[chan]->start();
            }
        }
        virtual void stop(int chan) override {
            if (chan < 0) {
                if (logger_) {
                    logger_->println(brief_ + " chan prop stop[" + sys::to_string(chan) + "]");
                }
                for (int i = 0; i < num_channels_; i++) {
                    channels_[i]->stop();
                }
            } else if (chan < num_channels_) {
                if (logger_) {
                    logger_->println(brief_ + " chan prop stop[all]");
                }
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
            }
            if (chan < num_channels_) {
                seqable = channels_[chan]->sequencable();
            }
            if (logger_) {
                logger_->println(brief_ + " chan prop sequencable[" + sys::to_string(chan) + "]" + " -> " + (seqable ? "true":"false"));
            }
            return seqable;
        }

        /** call with chan < 0 to check if any channels are read_only  */
        virtual bool read_only(int chan) const override {
            bool ronly = false;
            if (chan < 0) { // check all channels
                for (int i = 0; i < num_channels_; i++) {
                    if (!channels_[i]->read_only()) {
                        ronly = true;
                        break;
                    }
                }
            }
            if (chan < num_channels_) {
                ronly = channels_[chan]->read_only();
            }
            if (logger_) {
                logger_->println(brief_ + " chan prop read_only[" + sys::to_string(chan) + "]" + " -> " + (ronly ? "true":"false"));
            }
            return ronly;
        }

     protected:
        int num_channels_;
        ChanT* channels_[MAX_CHANNELS];
    };

};

#endif // __SERVERPROPERTY_H__