#pragma once

#ifndef __PROPINFO_H__
    #define __PROPINFO_H__

    #include <ostream>
    #include <string>
    #include <type_traits>
    #include <vector>

namespace rdlmm {

    /**
     * Property creation information using builder pattern.
     * 
     * @tparam PropT    Property type (int, float, etc)
     */
    template <typename PropT>
    class PropInfo {
     public:
        /*******************************************************************
         * Setters
         *******************************************************************/

        /** 
        * Factory method to creating a PropInfo with an initial value. 
        * This templated version is neccessary to catch potential problems such as
        * @code{.cpp} PropInfo<std::string> build("foo",0); @endcode 
        * which tries to inialize a std::string from a null pointer. 
        */
        template <typename U, typename std::enable_if<std::is_convertible<U, PropT>::value, bool>::type = true>
        static PropInfo build(const char* name, const U& initialValue) {
            return PropInfo(name, static_cast<const PropT>(initialValue));
        }

        /** Factory method to creating a PropInfo without an initial value. */
        static PropInfo build(const char* name) {
            return PropInfo(name);
        }

        /** Add brief name (only for remote properties). */
        PropInfo& withBrief(const char* brief) {
            brief_ = brief;
            return *this;
        }

        /** Add min and max value limits to the PropInfo. Sets hasLimits to true. */
        PropInfo& withLimits(double minval, double maxval) {
            minValue_  = minval;
            maxValue_  = maxval;
            hasLimits_ = true;
            return *this;
        }

        /** Add a single allowed value to the PropInfo. */
        PropInfo& withAllowedValue(const PropT& val) {
            allowedValues_.push_back(val);
            return *this;
        }

        /** Add an array of allowed values to the PropInfo using an initializer list.
         * For example: @code{.cpp} PropInfo<int>build("foo",1).withAllowedValues({1, 2, 3, 4});@endcode */
        PropInfo& withAllowedValues(const std::initializer_list<PropT>& vals) {
            allowedValues_.insert(allowedValues_.end(), vals.begin(), vals.end());
            return *this;
        }

        /** Add several allowed values to the PropInfo from a vector. */
        PropInfo& withAllowedValues(const std::vector<PropT>& vals) {
            allowedValues_.insert(allowedValues_.end(), vals.begin(), vals.end());
            return *this;
        }

        /** This is a pre-init property. */
        PropInfo& preInit() {
            isPreInit_ = true;
            return *this;
        }

        /** __DEFAULT__ This isn't a pre-init property. */
        PropInfo& notPreInit() {
            isPreInit_ = false;
            return *this;
        }

        /** This is a sequencable property (only for remote properties). */
        PropInfo& sequencable() {
            isSequencable_ = true;
            return *this;
        }

        /** DEFAULT This isn't a sequencable property (only for remote properties). */
        PropInfo& notSequencable() {
            isSequencable_ = false;
            return *this;
        }

        /** This property is read-only. */
        PropInfo& readOnly() {
            isReadOnly_ = true;
            return *this;
        }

        /** __DEFAULT__ The property can be read and written. */
        PropInfo& notReadOnly() {
            isReadOnly_ = false;
            return *this;
        }

        /** This property is volatile (not cached). */
        PropInfo& volatileValue() {
            isVolatile_ = true;
            return *this;
        }

        /** __DEFAULT__ This property is volatile (not cached). */
        PropInfo& notVolatileValue() {
            isVolatile_ = false;
            return *this;
        }

        /** Remove any initial value */
        PropInfo& witoutInitialValue() {
            hasInitialValue_ = false;
            return *this;
        }

        /*******************************************************************
         * Getters
         *******************************************************************/
        /** Get the property name. */
        const std::string& name() const { return name_; }

        /** Get the property brief name (for remote). */
        const std::string& brief() const { return brief_; }

        /** Get the initial property value. */
        PropT initialValue() const { return initialValue_; }

        /**  Was an initial value specified? */
        bool hasInitialValue() const { return hasInitialValue_; }

        /** Was this a pre-init property? */
        bool isPreInit() const { return isPreInit_; }

        /** Was this a read-only property? */
        bool isReadOnly() const { return isReadOnly_; }

        /** Was this a sequencable property? */
        bool isSequencable() const { return isSequencable_; }

        /** Was this a voilatile property? */
        bool isVolatileValue() const { return isVolatile_; }


        /** Has the withLimits() been added? */
        bool hasLimits() const { return hasLimits_; }

        /** minimum limit value. */
        double minValue() const { return minValue_; }

        /** maximum limit value. */
        double maxValue() const { return maxValue_; }

        /** has one of the withAllowedValues() been added? */
        bool hasAllowedValues() const { return !allowedValues_.empty(); }

        /** get all of the allowed values. */
        std::vector<PropT> allowedValues() const { return allowedValues_; }

        friend std::ostream& operator<<(std::ostream& os, const PropInfo& p) {
            os << "PropInfo.name=" << p.name_;
            os << " .brief=" << p.brief_;
            os << " .initialValue=" << p.initialValue;
            os << " .hasInitialValue=" << p.hasInitialValue_;
            os << " .isPreInit=" << std::boolalpha << p.isPreInit_;
            os << " .isReadOnly=" << std::boolalpha << p.isReadOnly_;
            os << " .isSequencable=" << std::boolalpha << p.isSequencable_;
            os << " .isVolatile=" << std::boolalpha << p.isVolatile_;
            os << " .hasLimits=" << std::boolalpha << p.hasLimits;
            os << " .minValue=" << p.minValue_;
            os << " .maxValue=" << p.maxValue_;
            os << " .allowedValues={";
            std::copy(p.allowedValues.begin(), p.alowedValue.end(), std::ostream_iterator<PropT>(os, " "));
            os << "\b}";
            return os;
        }

     protected:
        PropInfo(const char* name, const PropT& initialValue) 
            : name_(name), initialValue_(initialValue), hasInitialValue_(true) {}
        PropInfo(const char* name) : name_(name), hasInitialValue_(false) {}

        std::string name_;
        std::string brief_;
        PropT initialValue_;
        bool hasInitialValue_ = false;
        bool isPreInit_       = false;
        bool isReadOnly_      = false;
        bool isSequencable_   = false;
        bool isVolatile_      = false;
        bool hasLimits_       = false;
        double minValue_      = 0;
        double maxValue_      = 0;
        std::vector<PropT> allowedValues_;
    };

    inline PropInfo<std::string> ErrorPropInfo(const char* name, std::string& error) {
        return PropInfo<std::string>::build(name, error).readOnly().preInit();
    }

}; // namespace rdl

#endif // __PROPINFO_H__