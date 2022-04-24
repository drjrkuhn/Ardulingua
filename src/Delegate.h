/*
 *
 *	Delegates based on CppDelegates.
 *		Copyright (C) 2017 by Sergey A Kryukov: derived work
 *		http://www.SAKryukov.org
 *		http://www.codeproject.com/Members/SAKryukov
 *		Based on original work by Sergey Ryazanov:
 *		"The Impossibly Fast C++ Delegates", 18 Jul 2005
 *		https://www.codeproject.com/articles/11015/the-impossibly-fast-c-delegates
 *		MIT license:
 *		http://en.wikipedia.org/wiki/MIT_License
 *		Original publication: https://www.codeproject.com/Articles/1170503/The-Impossibly-Fast-Cplusplus-Delegates-Fixed
 */

#pragma once

#ifndef __DELEGATE_H__
    #define __DELEGATE_H__

    #include "Polyfills/std_utility.h"
    #include <assert.h>
    #include <tuple>

namespace rdl {

    /** Tag for template argument */
    template <typename R>
    struct Ret { using type = R; };

    /** pass-through marker tag for template argument */
    template <class R>
    using RetT = typename Ret<R>::type;

    /************************************************************************
     * Generic stub stub with of type erased.
     ***********************************************************************/
    class stub_base {
     public:
        using FnStubT = void (*)(void* this_ptr);

        stub_base() : object_(nullptr), fnstub_(nullptr) {}

        bool operator==(const stub_base& other) const {
            return object_ == other.object_ && fnstub_ == other.fnstub_;
        }
        bool operator!=(const stub_base& other) const {
            return object_ != other.object_ || fnstub_ != other.fnstub_;
        }

        void* object() const { return object_; }
        FnStubT fnstub() const { return fnstub_; }

     protected:
        stub_base(void* object, FnStubT fnstub) : object_(object), fnstub_(fnstub) {}

        void* object_;
        FnStubT fnstub_;

    }; // class stub_base

    /************************************************************************
     * Generic stub with delegate type erased.
     ***********************************************************************/

    class stub : public stub_base {
     public:
        stub() : stub_base() {}

        template <typename RTYPE, typename... PARAMS>
        RTYPE call(PARAMS... arg) const {
            using FunctionT = RTYPE (*)(void* this_ptr, PARAMS...);
            assert(fnstub_ != nullptr);
            return (*reinterpret_cast<FunctionT>(fnstub_))(object_, arg...);
        }

        template <typename RTYPE, typename TUPLE>
        inline RTYPE call_tuple(const TUPLE& args) const {
            assert(fnstub_ != nullptr);
            return call_tuple_impl<RTYPE>(args, std::make_index_sequence<std::tuple_size<TUPLE>{}>{});
        }

        stub(void* object, FnStubT fnstub) : stub_base(object, fnstub) {}

     protected:

        template <typename RTYPE, typename TUPLE, size_t... I>
        inline RTYPE call_tuple_impl(const TUPLE& args, std::index_sequence<I...>) const {
            return call<RTYPE>(std::get<I>(args)...);
        }

     public:
    }; // class stub

    /************************************************************************
     * Typed stub stubs
     ***********************************************************************/
    template <typename RTYPE, typename... PARAMS>
    class delegate {
     protected:
        stub stub_;
        delegate(stub _stub) : stub_(_stub) {}
        delegate(void* object, stub_base::FnStubT fnstub) : stub_(object, fnstub) {}

     public:
        delegate() : delegate(nullptr, reinterpret_cast<stub::FnStubT>(error_stub)) {}

        const stub& stub() const { return stub_; }

        bool operator==(const delegate& other) const { return stub_ == other.stub_; }
        bool operator!=(const delegate& other) const { return stub_ != other.stub_; }

        /********************************************************************
         * FACTORIES
         *******************************************************************/
        template <class T, RTYPE (T::*TMethod)(PARAMS...)>
        static delegate create(T* instance) {
            auto mstub = method_stub<T, TMethod>;
            return delegate(instance, reinterpret_cast<stub::FnStubT>(mstub));
        }

        template <class T, RTYPE (T::*TMethod)(PARAMS...) const>
        static delegate create(T const* instance) {
            auto mstub = const_method_stub<T, TMethod>;
            return delegate(const_cast<T*>(instance), reinterpret_cast<stub::FnStubT>(mstub));
        }

        template <RTYPE (*TMethod)(PARAMS...)>
        static delegate create() {
            auto fstub = function_stub<TMethod>;
            return delegate(nullptr, reinterpret_cast<stub::FnStubT>(fstub));
        }

        template <typename LAMBDA>
        static delegate create(const LAMBDA& instance) {
            auto lstub = lambda_stub<LAMBDA>;
            return delegate((void*)(&instance), reinterpret_cast<stub::FnStubT>(lstub));
        }

        /********************************************************************
         * CALL OPERATOR
         *******************************************************************/

        RTYPE operator()(PARAMS... arg) const {
            assert(stub_.fnstub() != nullptr);
            using FunctionT = RTYPE (*)(void*, PARAMS...);
            FunctionT fn    = reinterpret_cast<FunctionT>(stub_.fnstub());
            return (*fn)(stub_.object(), arg...);
        }

     protected:
        friend class stub;

        static RTYPE error_stub(void* this_ptr, PARAMS... params) {
            assert(false);
            RTYPE ret;
            return ret;
        }

        /********************************************************************
         * STUB IMPLEMENTATIONS
         * note pointer to member function in template list
         *******************************************************************/

        //// class method stubs ////

        template <class T, RTYPE (T::*TMethod)(PARAMS...)>
        static RTYPE method_stub(void* this_ptr, PARAMS... params) {
            T* p = static_cast<T*>(this_ptr);
            return (p->*TMethod)(params...);
        }

        //// class constant method stubs ////

        template <class T, RTYPE (T::*TMethod)(PARAMS...) const>
        static RTYPE const_method_stub(void* this_ptr, PARAMS... params) {
            T* const p = static_cast<T*>(this_ptr);
            return (p->*TMethod)(params...);
        }

        //// free stub stubs ////

        template <RTYPE (*TMethod)(PARAMS...)>
        static RTYPE function_stub(void* this_ptr, PARAMS... params) {
            return (TMethod)(params...);
        }

        //// lambda stub stubs ////

        template <typename LAMBDA>
        static RTYPE lambda_stub(void* this_ptr, PARAMS... arg) {
            LAMBDA* p = static_cast<LAMBDA*>(this_ptr);
            return (p->operator())(arg...);
        }

        template <typename R, typename... P>
        friend delegate<R,P...> as(class stub& stub);


    }; // class delegate


    template <typename RTYPE, typename... PARAMS>
    inline delegate<RTYPE, PARAMS...> as(stub& stub) {
        return delegate<RTYPE, PARAMS...>(stub);
    }


} /* namespace rdl */

#endif // __DELEGATE_H__