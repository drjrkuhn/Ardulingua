#pragma once

#ifndef __SERVERPROPERTY_H__
    #define __SERVERPROPERTY_H__

namespace rdl {

    template <typename T, class StrT, typename... ExT>
    class prop_any_base {
     public:
        prop_any_base(const StrT& brief_name) : brief_(brief_name) {}

        // TODO: use ATOMIC_BLOCK found in avr-libc <util/atomic.h> or mutex
        virtual T get(ExT... ex) const             = 0;
        virtual void set(const T value, ExT... ex) = 0;
        virtual long max_size(ExT... ex) const     = 0;
        virtual long size(ExT... ex) const         = 0;
        virtual long clear(ExT... ex)              = 0;
        virtual void add(const T value, ExT... ex) = 0;
        virtual void start(ExT... ex)              = 0;
        virtual void stop(ExT... ex)               = 0;
        virtual bool sequencable(ExT... ex)        = 0;

        StrT message(const char opcode) { return opcode + brief_; }

        template <class MapT>
        int add_to(MapT& map) {
            // using PairT   = typename MapT::value_type;
            using S   = prop_any_base<T, StrT, ExT...>;
            auto sget = &S::get;
            // int startsize = map.size();
            // auto jd = json_delegate<T, ExT...>::template create<S, &S::get>(this);
            json_delegate<void, T, ExT...> jd;
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

     protected:
        StrT brief_;
    };

    template <typename T, class StrT, long MAX_SIZE>
    class simple_prop_base : public prop_any_base<T, StrT> {
     public:
        using ThisT = simple_prop_base<T, StrT, MAX_SIZE>;
        using BaseT = prop_any_base<T, StrT>;

        simple_prop_base(const StrT& brief_name, const T initial, bool sequencable = false)
            : BaseT(brief_name),
              value_(initial), max_size_(MAX_SIZE), sequencable_(sequencable),
              next_index_(0), size_(0), started_(false) {
        }

        virtual T get() const override {
            return value_;
        }
        virtual void set(const T value) override {
            value_ = value;
        }
        virtual long max_size() const override {
            return max_size_;
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
            if (size_ >= max_size_)
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
        virtual bool sequencable() override {
            return sequencable_;
        }

     protected:
        StrT brief_;
        T value_;
        bool sequencable_;
        const long max_size_;
        volatile long next_index_;
        long size_;
        volatile bool started_;
        T values_[MAX_SIZE];
    };


};

#endif // __SERVERPROPERTY_H__