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

namespace rdl {

    /** Tag for template argument */
    template <typename R>
    struct Ret { using type = R; };

    /** pass-through marker tag for template argument */
    template <class R>
    using RetT = typename Ret<R>::type;

    /************************************************************************
     * Generic delegate stub with of type erased.
     ***********************************************************************/
    class delegate_base {
     public:
        using FnStubT = void (*)(void* this_ptr);

        delegate_base() = delete;

        bool operator==(const delegate_base& other) const {
            return object_ == other.object_ && fnstub_ == other.fnstub_;
        }
        bool operator!=(const delegate_base& other) const {
            return object_ != other.object_ || fnstub_ != other.fnstub_;
        }

        void* object() const { return object_; }
        FnStubT fnstub() const { return fnstub_; }

     protected:
        delegate_base(void* object, FnStubT fnstub) : object_(object), fnstub_(fnstub) {}

        void* object_   = nullptr;
        FnStubT fnstub_ = nullptr;

    }; // class delegate_base

    /************************************************************************
     * Generic delegate with of type erased.
     ***********************************************************************/

    class delegate : public delegate_base {
     public:
        delegate() = delete;

        template <class RTYPE, typename... PPARAMS>
        class of;

        template <typename RTYPE, typename... PARAMS>
        of<RTYPE, PARAMS...> as() {
            return of<RTYPE, PARAMS...>(this);
        }

        template <typename RTYPE, typename... PARAMS>
        RTYPE call(PARAMS... arg) const {
            using FunctionT = RTYPE (*)(void* this_ptr, PARAMS...);
            return (*reinterpret_cast<FunctionT>(fnstub_))(object_, arg...);
        }

     protected:
        delegate(void* object, FnStubT fnstub) : delegate_base(object, fnstub) {}

     public:
        /************************************************************************
         * Typed delegate stubs
         ***********************************************************************/
        template <typename RTYPE, typename... PARAMS>
        class of {
         public:
            // no default constructor
            of() = delete;
            of(delegate* pstub) : pstub_(pstub) {}

            const delegate& stub() const { return *pstub_; }

            bool operator==(const of& other) const { return pstub_ == other.pstub_; }
            bool operator!=(const of& other) const { return pstub_ != other.pstub_; }

            /********************************************************************
             * FACTORIES
             *******************************************************************/
            template <class T, RTYPE (T::*TMethod)(PARAMS...)>
            static delegate create(T* instance) {
                auto mstub = method_stub<T, TMethod>;
                delegate del(instance, reinterpret_cast<delegate::FnStubT>(mstub));
                return del;
            }

            template <class T, RTYPE (T::*TMethod)(PARAMS...) const>
            static delegate create(T const* instance) {
                auto mstub = const_method_stub<T, TMethod>;
                delegate del(const_cast<T*>(instance), reinterpret_cast<delegate::FnStubT>(mstub));
                return del;
            }

            template <RTYPE (*TMethod)(PARAMS...)>
            static delegate create() {
                auto fstub = function_stub<TMethod>;
                delegate del(nullptr, reinterpret_cast<delegate::FnStubT>(fstub));
                return del;
            }

            template <typename LAMBDA>
            static delegate create(const LAMBDA& instance) {
                auto lstub = lambda_stub<LAMBDA>;
                delegate del((void*)(&instance), reinterpret_cast<delegate::FnStubT>(lstub));
                return del;
            }

            /********************************************************************
             * CALL OPERATOR
             *******************************************************************/

            RTYPE operator()(PARAMS... arg) const {
                using FunctionT = RTYPE (*)(void*, PARAMS...);
                FunctionT fn    = reinterpret_cast<FunctionT>(pstub_->fnstub());
                return (*fn)(pstub_->object(), arg...);
            }

         protected:
            friend delegate;

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

            //// free delegate stubs ////

            template <RTYPE (*TMethod)(PARAMS...)>
            static RTYPE function_stub(void* this_ptr, PARAMS... params) {
                return (TMethod)(params...);
            }

            //// lambda delegate stubs ////

            template <typename LAMBDA>
            static RTYPE lambda_stub(void* this_ptr, PARAMS... arg) {
                LAMBDA* p = static_cast<LAMBDA*>(this_ptr);
                return (p->operator())(arg...);
            }

         protected:
            delegate* pstub_;
        }; // class delegate
    };     // class delegate stub

} /* namespace rdl */

#endif // __DELEGATE_H__