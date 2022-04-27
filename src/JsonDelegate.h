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
    #include "Polyfills/std_utility.h"
    #include <ArduinoJson.h>

namespace rdl {

    constexpr int ERROR_OK                    = 0;
    constexpr int ERROR_JSON_PARSE_ERROR      = -32700;
    constexpr int ERROR_JSON_INVALID_REQUEST  = -32600;
    constexpr int ERROR_JSON_METHOD_NOT_FOUND = -32601;
    constexpr int ERROR_JSON_INVALID_PARAMS   = -32602;
    constexpr int ERROR_JSON_INTERNAL_ERROR   = -32603;

    constexpr int ERROR_JSON_RET_NOT_SET    = -32000;
    constexpr int ERROR_JSON_ENCODING_ERROR = -32001;
    constexpr int ERROR_JSON_SEND_ERROR     = -32002;
    constexpr int ERROR_JSON_TIMEOUT        = -32003;
    constexpr int ERROR_JSON_INVALID_REPLY  = -32004;
    constexpr int ERROR_SLIP_ENCODING_ERROR = -32005;
    constexpr int ERROR_SLIP_DECODING_ERROR = -32006;

    constexpr int ERROR_JSON_DESER_ERROR_0          = -32090;
    constexpr int ERROR_JSON_DESER_EMPTY_INPUT      = ERROR_JSON_DESER_ERROR_0 - ArduinoJson::DeserializationError::EmptyInput;
    constexpr int ERROR_JSON_DESER_INCOMPLETE_INPUT = ERROR_JSON_DESER_ERROR_0 - ArduinoJson::DeserializationError::IncompleteInput;
    constexpr int ERROR_JSON_DESER_INVALID_INPUT    = ERROR_JSON_DESER_ERROR_0 - ArduinoJson::DeserializationError::InvalidInput;
    constexpr int ERROR_JSON_DESER_NO_MEMORY        = ERROR_JSON_DESER_ERROR_0 - ArduinoJson::DeserializationError::NoMemory;
    constexpr int ERROR_JSON_DESER_TOO_DEEP         = ERROR_JSON_DESER_ERROR_0 - ArduinoJson::DeserializationError::TooDeep;

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
        json_stub(json_stub& other) 
        : stub_base(other.object_, other.fnstub_), returns_void_(other.returns_void_) {
        }

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
        json_delegate() 
        : json_delegate(nullptr, reinterpret_cast<json_stub::FnStubT>(error_stub), std::is_void<RTYPE>::value) {}

        json_delegate(json_delegate& other) = default;

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
        template <class C, RTYPE(C::*TMethod)(PARAMS...)>
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

        template <typename R, typename... P>
        friend json_delegate<R, P...> as(class json_stub& stub);

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
            // std::cout << "[method_jsonstub_impl:ret_is_void]";
            C* p = static_cast<C*>(this_ptr);
            (p->*TMethod)(args[I].as<PARAMS>()...);
            return ERROR_OK;
        }

        template <class C, RTYPE (C::*TMethod)(PARAMS...), size_t... I>
        inline static int method_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_not_void) {
            // std::cout << "[method_jsonstub_impl:ret_not_void]";
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
            // std::cout << "[const_method_jsonstub_impl:ret_is_void]";
            C* const p = static_cast<C*>(this_ptr);
            (p->*TMethod)(args[I].as<PARAMS>()...);
            return ERROR_OK;
        }

        template <class C, RTYPE (C::*TMethod)(PARAMS...) const, size_t... I>
        inline static int const_method_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_not_void) {
            // std::cout << "[const_method_jsonstub_impl:ret_not_void]";
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
            // std::cout << "[function_jsonstub_impl:ret_is_void]";
            (TMethod)(args[I].as<PARAMS>()...);
            return ERROR_OK;
        }

        template <RTYPE (*TMethod)(PARAMS...), size_t... I>
        inline static int function_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_not_void) {
            // std::cout << "[function_jsonstub_impl:ret_not_void]";
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
            // std::cout << "[lambda_jsonstub_impl:ret_is_void]";
            LAMBDA* p = static_cast<LAMBDA*>(this_ptr);
            (p->operator())(args[I].as<PARAMS>()...);
            return ERROR_OK;
        }

        template <typename LAMBDA, size_t... I>
        inline static int lambda_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_not_void) {
            // std::cout << "[lambda_jsonstub_impl:ret_not_void]";
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

    template <typename RTYPE, typename... PARAMS>
    json_delegate<RTYPE, PARAMS...> as(json_stub& stub) {
        return json_delegate<RTYPE, PARAMS...>(stub);
    }

} /* namespace rdl */

#endif // __JSONDELEGATE_H__