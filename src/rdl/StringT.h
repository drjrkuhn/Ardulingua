/**
 * @file StringT.h
 * @author Jeffrey Kuhn (jrkuhn@mit.edu)
 * @brief Common Strings for Arduino and Host
 * @version 0.1
 * @date 2022-04-29
 * 
 * @copyright Copyright (c) 2022
 * 
 * 
 * Arduino and host have different string types. The compiler should
 * prefer arduino::String on microcontrollers and std::string on hosts
 * with a full OS. Microntrollers should prefer arduino::String for its
 * smaller heap footprint even if the library supports the C++ STL
 * (ie some ARM, STM32, and other boards).
 * 
 * Both string implementations share enough in common that Ardulingua \algorithms 
 * should be able to work with either type transparently. Constructors from 
 * const char*, copy, move, assignment, several operators, c_str, and length.
 * Where they differ, the rdl namespace provides a common set of accessors.
 * 
 * We define platform-dependent type alias to rdl::StringT below, and all
 * rdl algorithms and classes should use this common rdl::StringT and accessors
 * methods defined here. The name StringT is chosen to make it more obvious
 * that this is a type alias to an underlying implementation.
 */

#pragma once

#ifndef __STRINGT_H__
#define __STRINGT_H__


namespace rdl {

#ifdef ARDUINO
    #ifdef __has_include
        #if __has_include(<String.h>)
            #include <String.h>
        #elif __has_include(<WString.h>)
            #include <WString.h>
        #else
            #error Could not find Arduino String definition 
        #endif
    #endif
    using StringT = String;

#else
    #include <string>
    using StringT = std::string;

#endif

}; // namespace rdl


#endif // __STRINGT_H__