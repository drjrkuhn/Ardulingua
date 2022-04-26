#pragma once

#ifndef __SERVERPROPERTY_H__
    #define __SERVERPROPERTY_H__

namespace rdl {

    template <class DT, typename T, class StrT, class MapT, typename... ExT>
    class prop_any_base {
     public:
        prop_any_base(const StrT& brief_name) : brief_(brief_name) {}

        // TODO: use ATOMIC_BLOCK found in avr-libc <util/atomic.h> or mutex
        virtual T get(ExT... ex) const { return static_cast<const DT*>(this)->get_impl(ex...); }
        void set(const T value, ExT... ex) { static_cast<DT*>(this)->set_impl(value, ex...); }
        long max_size(ExT... ex) const { return static_cast<const DT*>(this)->max_size_impl(ex...); }
        long size(ExT... ex) const { return static_cast<const DT*>(this)->size_impl(ex...); }
        long clear(ExT... ex) { return static_cast<DT*>(this)->clear_impl(ex...); }
        void add(const T value, ExT... ex) { static_cast<DT*>(this)->add_impl(value, ex...); }
        void start(ExT... ex) { static_cast<DT*>(this)->start_impl(ex...); }
        void stop(ExT... ex) { static_cast<DT*>(this)->stop_impl(ex...); }
        bool sequencable(ExT... ex) { static_cast<DT*>(this)->sequencable_impl(ex...); }

        StrT message(const char opcode) { return opcode + brief_; }
        int add_to(MapT& map);

     protected:
        StrT brief_;
    };

    template <class DT, typename T, class StrT, class MapT, long MAX_SIZE>
    class simple_prop_base
        : public prop_any_base<simple_prop_base<DT, T, StrT, MapT, MAX_SIZE>, T, StrT, MapT> {
     public:
        using ThisT = simple_prop_base<DT, T, StrT, MapT, MAX_SIZE>;
        using BaseT = prop_any_base<ThisT, T, StrT, MapT>;
        friend BaseT;

        using BaseT::get;
        using BaseT::set;
        using BaseT::max_size;
        using BaseT::clear;
        using BaseT::add;
        using BaseT::start;
        using BaseT::sequencable;
        using BaseT::message;
        using BaseT::add_to;


        simple_prop_base(const StrT& brief_name, const T initial, bool sequencable = false)
            : BaseT(brief_name), value_(initial), max_size_(MAX_SIZE), sequencable_(sequencable),
              next_index_(0), size_(0), started_(false) {
        }

     protected:
        T get_impl() const { return value_; }
        void set_impl(const T value) { value_ = value; }
        long max_size_impl() const { return max_size_; }
        long size_impl() const { return size_; }
        void clear_impl() {
            size_       = 0;
            next_index_ = 0;
        }
        void add_impl(const T value) {
            if (size_ >= max_size_)
                return;
            values_[size_++] = value;
        }
        void start_impl() {
            next_index_ = 0;
            started_    = true;
        }
        void stop_impl() { started_ = false; }
        bool sequencable_impl() { return sequencable_; }

        StrT brief_;
        T value_;
        bool sequencable_;
        const long max_size_;
        volatile long next_index_;
        long size_;
        volatile bool started_;
        T values_[MAX_SIZE];
    };

    template <class DT, typename T, class StrT, class MapT, typename... ExT>
    int prop_any_base<DT, T, StrT, MapT, ExT...>::add_to(MapT& map) {
        // using PairT   = typename MapT::value_type;
        using S       = prop_any_base<DT, T, StrT, MapT, ExT...>;
        auto sget = &S::get;
        // int startsize = map.size();
        // auto jd = json_delegate<T, ExT...>::template create<S, &S::get>(this);
        json_delegate<void,T,ExT...> jd;
        jd = json_delegate<void, T, ExT...>::template create<S, &S::set>(this);
        // map.emplace(PairT(message('?'), json_delegate<T, ExT...>::template create<S, &S::get>(this).stub()));
        // map.emplace(PairT(message('!'), json_delegate<void, T, ExT...>::template create<S, &S::set>(this).stub()));
        // if (sequencable()) {
        //     map.emplace(PairT(message('^'), json_delegate<long, ExT...>::template create<S, &S::max_size>(this).stub()));
        //     map.emplace(PairT(message('#'), json_delegate<long, ExT...>::template create<S, &S::size>(this).stub()));
        //     map.emplace(PairT(message('0'), json_delegate<long, ExT...>::template create<S, &S::clear>(this).stub()));
        //     map.emplace(PairT(message('+'), json_delegate<void, T, ExT...>::template create<S, &S::add>(this).stub()));
        //     map.emplace(PairT(message('*'), json_delegate<void, ExT...>::template create<S, &S::start>(this).stub()));
        //     map.emplace(PairT(message('~'), json_delegate<void, ExT...>::template create<S, &S::stop>(this).stub()));
        // }
        // return map.size() - startsize;
    }

};

#endif // __SERVERPROPERTY_H__