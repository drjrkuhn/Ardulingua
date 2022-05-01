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

#ifndef __JSONDELEGATE_H__
    #define __JSONDELEGATE_H__

    #include "Delegate.h"
    #include "JsonError.h"
    #include "std_utility.h"
    #include <ArduinoJson.h>

namespace rdl {


    /************************************************************************
     * Json stubs with type erased
     *
     * Stubs return an integer error code or ERROR_OK
     * Take a single JsonVariant reference to fill with delegate return value
     * Take a JsonArray reference containing an array json_delegate parameters
     *
     *                          ** WARNING **
     * Stubs and delegates do not use smart_pointers and have no destructors.
     * They do not check if an object or function has gone out-of-context. Use
     * them for permanent (top-level) or semi-permanent objects and functions
     * that last the lifetime of the program.
     * If you need function delegates with delete, move, etc, use std::function
     * found in the STL header <functional>
     ***********************************************************************/
    class json_stub : public stub_base {
     public:
        using FnStubT = int (*)(void* this_ptr, JsonArray&, JsonVariant&);
        json_stub() : stub_base(), returns_void_(true) {}
        // assume default copy constructor and assignment operator

        json_stub(void* object, FnStubT fnstub, bool returns_void)
            : stub_base(object, reinterpret_cast<stub_base::FnStubT>(fnstub)),
              returns_void_(returns_void) {}

        int call(JsonArray& args, JsonVariant& ret) const {
            assert(fnstub_ != nullptr);
            return (*reinterpret_cast<FnStubT>(fnstub_))(object_, args, ret);
        }

        bool returns_void() const { return returns_void_; }

     protected:
        bool returns_void_;
    }; // json_stub

    /************************************************************************
     * Typed jsondelegate stubs
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
    class json_delegate {
     public:
        // no default constructor
        json_delegate() : json_delegate(nullptr, reinterpret_cast<json_stub::FnStubT>(error_stub),
                                        std::is_void<RTYPE>::value) {}

        // assume default copy constructor and assignment operator

        bool operator==(const json_delegate& other) const { return stub_ == other.stub_; }
        bool operator!=(const json_delegate& other) const { return stub_ != other.stub_; }

        const json_stub& stub() const { return stub_; }

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
         *       json_delegate<int>::create<A, &A::foo>(&aa)(); // aa.A::foo -> 2
         *       json_delegate<int>::create<A, &A::bar>(&aa)(); // aa.A::bar -> -2
         *
         *       json_delegate<int>::create<B,&B::foo>(&bb)(); // compiler error: not found
         *       json_delegate<int>::create<A, &A::foo>(&bb)(); // bb.A::foo -> 10
         *       json_delegate<int>::create<B, &B::bar>(&bb)(); // bb.B::bar -> -40
         *       return 0;
         *   }
         * @endcode
         *******************************************************************/

        /** Create from class method */
        template <class C, RTYPE (C::*TMethod)(PARAMS...)>
        static json_delegate create(C* instance) {
            auto jsonstub = method_jsonstub<C, TMethod>;
            return json_delegate(const_cast<C*>(instance), reinterpret_cast<json_stub::FnStubT>(jsonstub), std::is_void<RTYPE>::value);
        }

        /** Create from const class method */
        template <class C, RTYPE (C::*TMethod)(PARAMS...) const>
        static json_delegate create(C const* instance) {
            auto jsonstub = const_method_jsonstub<C, TMethod>;
            return json_delegate(const_cast<C*>(instance), reinterpret_cast<json_stub::FnStubT>(jsonstub), std::is_void<RTYPE>::value);
        }

        /** Create from static function */
        template <RTYPE (*TMethod)(PARAMS...)>
        static json_delegate create() {
            auto jsonstub = function_jsonstub<TMethod>;
            return json_delegate(nullptr, reinterpret_cast<json_stub::FnStubT>(jsonstub), std::is_void<RTYPE>::value);
        }

        /** Create from lambda */
        template <typename LAMBDA>
        static json_delegate create(const LAMBDA& instance) {
            auto jsonstub = lambda_jsonstub<LAMBDA>;
            return json_delegate((void*)(&instance), reinterpret_cast<json_stub::FnStubT>(jsonstub), std::is_void<RTYPE>::value);
        }

        /********************************************************************
         * CALL OPERATOR
         *******************************************************************/
        int operator()(JsonArray& args, JsonVariant& ret) const {
            assert(stub_.fnstub() != nullptr);
            using FunctionT = json_stub::FnStubT;
            FunctionT fn    = reinterpret_cast<FunctionT>(stub_.fnstub());
            return (*fn)(stub_.object(), args, ret);
        }

     protected:
        class json_stub stub_;

        json_delegate(class json_stub mainstub) : stub_(mainstub) {}
        json_delegate(void* object, json_stub::FnStubT fnstub, bool returns_void) : stub_(object, fnstub, returns_void) {}

        // NO NEED for as. Just use the json_delegate stub constructor
        // template <typename R, typename... P>
        // friend json_delegate<R, P...> as(class json_stub& stub);

        /********************************************************************
         * STUB IMPLEMENTATIONS
         * note pointer to member function in template list
         *
         * @see https://godbolt.org/z/QDyx3H for example json_delegate unpacking using
         * std::index_sequence
         *******************************************************************/

        //// class method stubs ////

        template <class C, RTYPE (C::*TMethod)(PARAMS...), size_t... I>
        inline static int method_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_is_void) {
            // SerialUSB1.print("void method_jsonstub_impl #param ");
            // SerialUSB1.println(sizeof...(I));
            C* p = static_cast<C*>(this_ptr);
            (p->*TMethod)(args[I].as<PARAMS>()...);
            return ERROR_OK;
        }

        template <class C, RTYPE (C::*TMethod)(PARAMS...), size_t... I>
        inline static int method_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_not_void) {
            // SerialUSB1.print("not-void method_jsonstub_impl #param ");
            // SerialUSB1.println(sizeof...(I));
            C* p = static_cast<C*>(this_ptr);
            if (ret.set(static_cast<RTYPE>((p->*TMethod)(args[I].as<PARAMS>()...))))
                return ERROR_OK;
            else
                return ERROR_JSON_RET_NOT_SET;
        }

        template <class C, RTYPE (C::*TMethod)(PARAMS...)>
        static int method_jsonstub(void* this_ptr, JsonArray& args, JsonVariant& ret) {
            if (args.size() < sizeof...(PARAMS)) return ERROR_JSON_INVALID_PARAMS;
            return method_jsonstub_impl<C, TMethod>(this_ptr, args, ret, std::index_sequence_for<PARAMS...>{}, svc::is_void_tag<RTYPE>{});
        }

        //// class constant method stubs ////

        template <class C, RTYPE (C::*TMethod)(PARAMS...) const, size_t... I>
        inline static int const_method_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_is_void) {
            // SerialUSB1.print("void const_method_jsonstub_impl #param ");
            // SerialUSB1.println(sizeof...(I));
            C* const p = static_cast<C*>(this_ptr);
            (p->*TMethod)(args[I].as<PARAMS>()...);
            return ERROR_OK;
        }

        template <class C, RTYPE (C::*TMethod)(PARAMS...) const, size_t... I>
        inline static int const_method_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_not_void) {
            // SerialUSB1.print("not-void const_method_jsonstub_impl #param ");
            // SerialUSB1.println(sizeof...(I));
            C* const p = static_cast<C*>(this_ptr);
            if (ret.set(static_cast<RTYPE>((p->*TMethod)(args[I].as<PARAMS>()...))))
                return ERROR_OK;
            else
                return ERROR_JSON_RET_NOT_SET;
        }

        template <class C, RTYPE (C::*TMethod)(PARAMS...) const>
        static int const_method_jsonstub(void* this_ptr, JsonArray& args, JsonVariant& ret) {
            if (args.size() < sizeof...(PARAMS)) return ERROR_JSON_INVALID_PARAMS;
            return const_method_jsonstub_impl<C, TMethod>(this_ptr, args, ret, std::index_sequence_for<PARAMS...>(), svc::is_void_tag<RTYPE>{});
        }

        //// free jsondelegate stubs ////

        template <RTYPE (*TMethod)(PARAMS...), size_t... I>
        inline static int function_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_is_void) {
            // SerialUSB1.print("void function_jsonstub_impl #param ");
            // SerialUSB1.println(sizeof...(I));
            (TMethod)(args[I].as<PARAMS>()...);
            return ERROR_OK;
        }

        template <RTYPE (*TMethod)(PARAMS...), size_t... I>
        inline static int function_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_not_void) {
            // SerialUSB1.print("not-void function_jsonstub_impl #param ");
            // SerialUSB1.println(sizeof...(I));
            if (ret.set(static_cast<RTYPE>((TMethod)(args[I].as<PARAMS>()...))))
                return ERROR_OK;
            else
                return ERROR_JSON_RET_NOT_SET;
        }

        template <RTYPE (*TMethod)(PARAMS...)>
        static int function_jsonstub(void* this_ptr, JsonArray& args, JsonVariant& ret) {
            constexpr size_t nparams = sizeof...(PARAMS);
            size_t nargs             = args.size();
            if (nargs < nparams) return ERROR_JSON_INVALID_PARAMS;
            return function_jsonstub_impl<TMethod>(this_ptr, args, ret, std::index_sequence_for<PARAMS...>{}, svc::is_void_tag<RTYPE>{});
        }

        //// lambda jsondelegate stubs ////

        template <typename LAMBDA, size_t... I>
        inline static int lambda_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_is_void) {
            // SerialUSB1.print("void lambda_jsonstub_impl #param ");
            // SerialUSB1.println(sizeof...(I));
            LAMBDA* p = static_cast<LAMBDA*>(this_ptr);
            (p->operator())(args[I].as<PARAMS>()...);
            return ERROR_OK;
        }

        template <typename LAMBDA, size_t... I>
        inline static int lambda_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_not_void) {
            // SerialUSB1.print("not-void lambda_jsonstub_impl #param ");
            // SerialUSB1.println(sizeof...(I));
            LAMBDA* p = static_cast<LAMBDA*>(this_ptr);
            if (ret.set(static_cast<RTYPE>((p->operator())(args[I].as<PARAMS>()...))))
                return ERROR_OK;
            else
                return ERROR_JSON_RET_NOT_SET;
        }

        template <typename LAMBDA>
        static int lambda_jsonstub(void* this_ptr, JsonArray& args, JsonVariant& ret) {
            if (args.size() < sizeof...(PARAMS)) return ERROR_JSON_INVALID_PARAMS;
            return lambda_jsonstub_impl<LAMBDA>(this_ptr, args, ret, std::index_sequence_for<PARAMS...>(), svc::is_void_tag<RTYPE>{});
        }

        //// error stub ////
        inline static int error_stub(void* this_ptr, JsonArray& args, JsonVariant& ret) {
            assert(false);
            return ERROR_JSON_METHOD_NOT_FOUND;
        }

    }; // jsondelegate

    // template <typename RTYPE, typename... PARAMS>
    // json_delegate<RTYPE, PARAMS...> as(json_stub& stub) {
    //     return json_delegate<RTYPE, PARAMS...>(stub);
    // }

} /* namespace rdl */

#endif // __JSONDELEGATE_H__