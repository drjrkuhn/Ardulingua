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

    #include "std_utility.h"
    #include <assert.h>
    #include <tuple>

namespace rdl {

    /** Tag for template argument */
    template <typename R>
    struct Ret { using type = R; };

    /** pass-through marker tag for template argument */
    template <class R>
    using RetT = typename Ret<R>::type;

    namespace svc {
        // C++11 compile-time helpers to determine which _impl to call
        // in c++14 or later, one could use constexpr if(std::is_void<RTYPE>::type) to switch
        // ALSO see https://stackoverflow.com/questions/43587405/constexpr-if-alternative
        using ret_is_void  = std::true_type;
        using ret_not_void = std::false_type;
        template <typename R>
        using is_void_tag = std::integral_constant<bool, std::is_void<R>::value>;
    };



    /************************************************************************
     * Generic function call stub with delegate type erased.
     * 
     *                          ** WARNING **
     * Stubs and delegates do not use smart_pointers and have no destructors. 
     * They do not check if an object or function has gone out-of-context. Use
     * them for permanent (top-level) or semi-permanent objects and functions 
     * that last the lifetime of the program. 
     * If you need function delegates with delete, move, etc, use std::function
     * found in the STL header <functional>
     * 
     ***********************************************************************/
    class stub_base {
     public:
        using FnStubT = void (*)(void* this_ptr);

        stub_base() : object_(nullptr), fnstub_(nullptr) {}
        stub_base(void* object, FnStubT fnstub) : object_(object), fnstub_(fnstub) {}
        // assume default copy constructor and assignment operator

        bool operator==(const stub_base& other) const {
            return object_ == other.object_ && fnstub_ == other.fnstub_;
        }
        bool operator!=(const stub_base& other) const {
            return object_ != other.object_ || fnstub_ != other.fnstub_;
        }

        void* object() const { return object_; }
        FnStubT fnstub() const { return fnstub_; }

     protected:
        void* object_;
        FnStubT fnstub_;
    }; // class stub_base

    /************************************************************************
     * Generic stub with delegate type erased.
     * 
     *                          ** WARNING **
     * Stubs and delegates do not use smart_pointers and have no destructors. 
     * They do not check if an object or function has gone out-of-context. Use
     * them for permanent (top-level) or semi-permanent objects and functions 
     * that last the lifetime of the program. 
     * If you need function delegates with delete, move, etc, use std::function
     * found in the STL header <functional>
     ***********************************************************************/

    class stub : public stub_base {
     public:
        stub() : stub_base() {}
        stub(void* object, FnStubT fnstub) : stub_base(object, fnstub) {}
        // assume default copy constructor and assignment operator

        template <typename RTYPE, typename... PARAMS>
        RTYPE call(PARAMS... arg) const {
            assert(fnstub_ != nullptr);
            using FunctionT = RTYPE (*)(void* this_ptr, PARAMS...);
            return (*reinterpret_cast<FunctionT>(fnstub_))(object_, arg...);
        }

        template <typename RTYPE, typename TUPLE>
        inline RTYPE call_tuple(const TUPLE& args) const {
            assert(fnstub_ != nullptr);
            return call_tuple_impl<RTYPE>(args, std::make_index_sequence<std::tuple_size<TUPLE>{}>{});
        }

     protected:
        template <typename RTYPE, typename TUPLE, size_t... I>
        inline RTYPE call_tuple_impl(const TUPLE& args, std::index_sequence<I...>) const {
            return call<RTYPE>(std::get<I>(args)...);
        }

    }; // class stub

    /************************************************************************
     * Typed delegates
     * 
     *                          ** WARNING **
     * Stubs and delegates do not use smart_pointers and have no destructors. 
     * They do not check if an object or function has gone out-of-context. Use
     * them for permanent (top-level) or semi-permanent objects and functions 
     * that last the lifetime of the program. 
     * If you need function delegates with delete, move, etc, use std::function
     * found in the STL header <functional>
     ***********************************************************************/
    template <typename RTYPE, typename... PARAMS>
    class delegate {
     public:
        delegate() : delegate(nullptr, reinterpret_cast<stub::FnStubT>(error_stub)) {}
        delegate(const class stub& _stub) : stub_(_stub) {}
        // assume default copy constructor and assignment operator

        bool operator==(const delegate& other) const { return stub_ == other.stub_; }
        bool operator!=(const delegate& other) const { return stub_ != other.stub_; }

        const class stub& stub() const { return stub_; }

        /********************************************************************
         * FACTORIES
         * 
         * NOTE on virtual functions
         * -------------------------
         * Class method function pointers are tricky when dealing with virtual
         * functions and derived classes. In create<Class,&Class::method>
         * Class::method must point to the exact overriden method you want
         * to call.
         * 
         * @code {.cpp}
         *   struct A { int val;
         *       virtual int foo() { return val; }
         *       virtual int bar() { return -val; }
         *   };
         *
         *   struct B : public A {
         *       virtual int bar() { return -4 * val; }
         *   };
         * 
         *   int main() {
         *       A aa; aa.val = 2;
         *       B bb; bb.val = 10;
         * 
         *       delegate<int>::create<A, &A::foo>(&aa)(); // aa.A::foo -> 2 
         *       delegate<int>::create<A, &A::bar>(&aa)(); // aa.A::bar -> -2
         * 
         *       delegate<int>::create<B,&B::foo>(&bb)(); // compiler error: not found
         *       delegate<int>::create<A, &A::foo>(&bb)(); // bb.A::foo -> 10
         *       delegate<int>::create<B, &B::bar>(&bb)(); // bb.B::bar -> -40
         *       return 0;
         *   }
         * @endcode
         *******************************************************************/
        
        /** Create from class method */
        template <class T, RTYPE (T::*TMethod)(PARAMS...)>
        static delegate create(T* instance) {
            auto mstub = method_stub<T, TMethod>;
            return delegate(instance, reinterpret_cast<stub::FnStubT>(mstub));
        }

        /** Create from const class method */
        template <class T, RTYPE (T::*TMethod)(PARAMS...) const>
        static delegate create(T const* instance) {
            auto mstub = const_method_stub<T, TMethod>;
            return delegate(const_cast<T*>(instance), reinterpret_cast<stub::FnStubT>(mstub));
        }

        /** Create from static function */
        template <RTYPE (*TMethod)(PARAMS...)>
        static delegate create() {
            auto fstub = function_stub<TMethod>;
            return delegate(nullptr, reinterpret_cast<stub::FnStubT>(fstub));
        }

        /** Create from lambda */
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
        class stub stub_;

        delegate(void* object, stub_base::FnStubT fnstub) : stub_(object, fnstub) {}

        // template <typename R, typename... P>
        // friend delegate<R, P...> as(class stub& stub);

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

        //// class constant method stub ////

        template <class T, RTYPE (T::*TMethod)(PARAMS...) const>
        static RTYPE const_method_stub(void* this_ptr, PARAMS... params) {
            T* const p = static_cast<T*>(this_ptr);
            return (p->*TMethod)(params...);
        }

        //// free stub ////

        template <RTYPE (*TMethod)(PARAMS...)>
        static RTYPE function_stub(void* this_ptr, PARAMS... params) {
            return (TMethod)(params...);
        }

        //// lambda stub ////

        template <typename LAMBDA>
        static RTYPE lambda_stub(void* this_ptr, PARAMS... arg) {
            LAMBDA* p = static_cast<LAMBDA*>(this_ptr);
            return (p->operator())(arg...);
        }

        //// error stub ////

        inline static RTYPE error_stub_impl(svc::ret_is_void) {
            assert(false);
            return;
        }

        inline static RTYPE error_stub_impl(svc::ret_not_void) {
            assert(false);
            return RTYPE();
        }
        static RTYPE error_stub(void*, PARAMS...) {
            return error_stub_impl(svc::is_void_tag<RTYPE>{});
        }


        
    }; // class delegate

    // /** 
    //  * `as` converts a stub back into a typed delegate 
    //  */
    // template <typename RTYPE, typename... PARAMS>
    // inline delegate<RTYPE, PARAMS...> as(stub& stub) {
    //     return delegate<RTYPE, PARAMS...>(stub);
    // }

} /* namespace rdl */

#endif // __DELEGATE_H__