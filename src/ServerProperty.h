#pragma once

#ifndef __SERVERPROPERTY_H__
    #define __SERVERPROPERTY_H__

namespace rdl {

    template <class DT, typename T, class StrT, class MapT, typename... ExT>
    class sequenceable_base {
     public:
        sequenceable_base(StrT& brief_name) : brief_(brief_name) {}

        // TODO: use ATOMIC_BLOCK found in avr-libc <util/atomic.h> or mutex
        T get(ExT... ex) const { return static_cast<const DT*>(this)->get_impl(ex...); }
        void set(const T value, ExT... ex) { static_cast<DT*>(this)->set_impl(value, ex...); }
        long max_size(ExT... ex) const { return static_cast<const DT*>(this)->max_size_impl(ex...); }
        long size(ExT... ex) const { return static_cast<const DT*>(this)->size_impl(ex...); }
        long clear(ExT... ex) { return static_cast<DT*>(this)->clear_impl(ex...); }
        void add(const T value, ExT... ex) { static_cast<DT*>(this)->add_impl(value, ex...); }
        void start(ExT... ex) { static_cast<DT*>(this)->start_impl(ex...); }
        void stop(ExT... ex) { static_cast<DT*>(this)->stop_impl(ex...); }
        StrT message(const char opcode) { return opcode + brief_; }
        int add_to(MapT& map);

     protected:
        StrT brief_;
    };

    template <class DT, typename T, class StrT, class MapT, typename... ExT>
    int sequenceable_base<DT, T, StrT, MapT, ExT...>::add_to(MapT& map) {
        using PairT   = typename MapT::value_type;
        using S       = sequenceable_base<T, DT, StrT, MapT, ExT...>;
        int startsize = map.size();
        map.emplace(PairT(message('?'), json_delegate<T, ExT...>::template create<S, &S::get>(this).stub()));
        map.emplace(PairT(message('!'), json_delegate<void, T, ExT...>::template create<S, &S::set>(this).stub()));
        map.emplace(PairT(message('^'), json_delegate<long, ExT...>::template create<S, &S::max_size>(this).stub()));
        map.emplace(PairT(message('#'), json_delegate<long, ExT...>::template create<S, &S::size>(this).stub()));
        map.emplace(PairT(message('0'), json_delegate<long, ExT...>::template create<S, &S::clear>(this).stub()));
        map.emplace(PairT(message('+'), json_delegate<void, T, ExT...>::template create<S, &S::add>(this).stub()));
        map.emplace(PairT(message('*'), json_delegate<void, ExT...>::template create<S, &S::start>(this).stub()));
        map.emplace(PairT(message('~'), json_delegate<void, ExT...>::template create<S, &S::stop>(this).stub()));
        return map.size() - startsize;
    }

    template <class DT, typename T, class StrT, class MapT, long MAX_SIZE>
    class simple_sequencable_base : sequenceable_base<simple_sequencable_base<DT>, T, StrT, MapT, void> {
     public:
        using BaseT = sequenceable_base<simple_sequencable_base<DT>, T, StrT, MapT, void>;

        simple_sequencable_base(StrT& brief_name, const T initial)
            : BaseT(brief_name), value_(initial), max_size_(MAX_SIZE),
              next_index_(0), size_(0), started_(false) {
        }

     protected:
        T get_impl() const { return value_; }
        void set_impl(const T value) { value_ = value; }
        long max_size_impl() const { return max_size_; }
        long size_impl() const { return size_; }
        void clear_impl() {
            size_       = 0;
            mext_index_ = 0;
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

        StrT brief_;
        T value_;
        const long max_size_;
        volatile long next_index_;
        long size_;
        volatile bool started_;
        T values_[MAX_SIZE];
    };

};

#endif // __SERVERPROPERTY_H__