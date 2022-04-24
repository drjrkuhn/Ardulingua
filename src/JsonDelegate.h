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

    /**
     * Json stubs base.
     *
     * Stubs return an integer error code or ERROR_OK
     * Take a single JsonVariant reference to fill with delegate return value
     * Take a JsonArray reference containing an array of parameters
     *
     */
    class json_delegate : public delegate_base {
     public:
        using FnStubT   = int (*)(void* this_ptr, JsonArray&, JsonVariant&);
        json_delegate() : delegate_base(), returns_void_(true) {}

        template <typename RTYPE, typename... PARAMS>
        class of;
        
        template <typename RTYPE, typename... PARAMS>
        of<RTYPE, PARAMS...> as() {
            return of<RTYPE, PARAMS...>(this);
        }

        int call(JsonArray& args, JsonVariant& ret) const {
            return (*reinterpret_cast<FnStubT>(fnstub_))(object_, args, ret);
        }

        bool returns_void() const { return returns_void_; }

     protected:

        json_delegate(void* object, FnStubT fnstub, bool returns_void)
            : delegate_base(object, reinterpret_cast<delegate_base::FnStubT>(fnstub)),
              returns_void_(returns_void) {
        }
        const bool returns_void_;

     public:
        /************************************************************************
         * Typed jsondelegate stubs
         ***********************************************************************/
        template <typename RTYPE, typename... PARAMS>
        class of {
         public:
            // no default constructor
            of() = delete;
            of(json_delegate* pstub) : pstub_(pstub) {}

            const json_delegate& stub() const { return *pstub_; }

            bool operator==(const of& other) const { return pstub_ == other.pstub_; }
            bool operator!=(const of& other) const { return pstub_ != other.pstub_; }

            /********************************************************************
             * FACTORIES
             *******************************************************************/
            template <class T, RTYPE (T::*TMethod)(PARAMS...)>
            static json_delegate create(T* instance) {
                auto jsonstub = method_jsonstub<T, TMethod>;
                json_delegate del(instance, reinterpret_cast<json_delegate::FnStubT>(jsonstub), std::is_void<RTYPE>::value);
                return del;
            }

            template <class T, RTYPE (T::*TMethod)(PARAMS...) const>
            static json_delegate create(T const* instance) {
                auto jsonstub = const_method_jsonstub<T, TMethod>;
                json_delegate del(const_cast<T*>(instance), reinterpret_cast<json_delegate::FnStubT>(jsonstub), std::is_void<RTYPE>::value);
                return del;
            }

            // template <typename TMethod>
            template <RTYPE (*TMethod)(PARAMS...)>
            static json_delegate create() {
                auto jsonstub = function_jsonstub<TMethod>;
                json_delegate del(nullptr, reinterpret_cast<json_delegate::FnStubT>(jsonstub), std::is_void<RTYPE>::value);
                return del;
            }

            template <typename LAMBDA>
            static json_delegate create(const LAMBDA& instance) {
                auto jsonstub = lambda_jsonstub<LAMBDA>;
                json_delegate del((void*)(&instance), reinterpret_cast<json_delegate::FnStubT>(jsonstub), std::is_void<RTYPE>::value);
                return del;
            }

            /********************************************************************
             * CALL OPERATOR
             *******************************************************************/
            int operator()(JsonArray& args, JsonVariant& ret) const {
                assert(pstub_ != nullptr);
                assert(pstub_->fnstub() != nullptr);
                using FunctionT = json_delegate::FnStubT; // RTYPE (*)(void*, JsonVariant&, JsonArray&);
                FunctionT fn    = reinterpret_cast<FunctionT>(pstub_->fnstub());
                return (*fn)(pstub_->object(), args, ret);
            }

         protected:
            friend json_delegate;

            /********************************************************************
             * STUB IMPLEMENTATIONS
             * note pointer to member function in template list
             *
             * @see https://godbolt.org/z/QDyx3H for example of unpacking using
             * std::index_sequence
             *******************************************************************/

            //// class method stubs ////

            template <class T, RTYPE (T::*TMethod)(PARAMS...), size_t... I>
            inline static int method_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_is_void) {
                // std::cout << "[method_jsonstub_impl:ret_is_void]";
                T* p = static_cast<T*>(this_ptr);
                (p->*TMethod)(args[I].as<PARAMS>()...);
                return ERROR_OK;
            }

            template <class T, RTYPE (T::*TMethod)(PARAMS...), size_t... I>
            inline static int method_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_not_void) {
                // std::cout << "[method_jsonstub_impl:ret_not_void]";
                T* p = static_cast<T*>(this_ptr);
                if (ret.set(static_cast<RTYPE>((p->*TMethod)(args[I].as<PARAMS>()...))))
                    return ERROR_OK;
                else
                    return ERROR_JSON_RET_NOT_SET;
            }

            template <class T, RTYPE (T::*TMethod)(PARAMS...)>
            static int method_jsonstub(void* this_ptr, JsonArray& args, JsonVariant& ret) {
                if (args.size() < sizeof...(PARAMS)) return ERROR_JSON_INVALID_PARAMS;
                return method_jsonstub_impl<T, TMethod>(this_ptr, args, ret, std::index_sequence_for<PARAMS...>{}, svc::is_void_tag<RTYPE>{});
            }

            //// class constant method stubs ////

            template <class T, RTYPE (T::*TMethod)(PARAMS...) const, size_t... I>
            inline static int const_method_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_is_void) {
                // std::cout << "[const_method_jsonstub_impl:ret_is_void]";
                T* const p = static_cast<T*>(this_ptr);
                (p->*TMethod)(args[I].as<PARAMS>()...);
                return ERROR_OK;
            }

            template <class T, RTYPE (T::*TMethod)(PARAMS...) const, size_t... I>
            inline static int const_method_jsonstub_impl(void* this_ptr, JsonArray& args, JsonVariant& ret, std::index_sequence<I...>, svc::ret_not_void) {
                // std::cout << "[const_method_jsonstub_impl:ret_not_void]";
                T* const p = static_cast<T*>(this_ptr);
                if (ret.set(static_cast<RTYPE>((p->*TMethod)(args[I].as<PARAMS>()...))))
                    return ERROR_OK;
                else
                    return ERROR_JSON_RET_NOT_SET;
            }

            template <class T, RTYPE (T::*TMethod)(PARAMS...) const>
            static int const_method_jsonstub(void* this_ptr, JsonArray& args, JsonVariant& ret) {
                if (args.size() < sizeof...(PARAMS)) return ERROR_JSON_INVALID_PARAMS;
                return const_method_jsonstub_impl<T, TMethod>(this_ptr, args, ret, std::index_sequence_for<PARAMS...>(), svc::is_void_tag<RTYPE>{});
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

         protected:
            json_delegate* pstub_;
        }; // jsondelegate
    };     // json_delegate

} /* namespace rdl */

#endif // __JSONDELEGATE_H__