#pragma once

namespace rdl {

    class Print;

    /** The Printable class provides a way for new classes to allow themselves to be printed.
    By deriving from Printable and implementing the printTo method, it will then be possible
    for users to print out instances of this class by passing them into the usual
    Print::print and Print::println methods.
*/

    class Printable {
     public:
        virtual size_t printTo(Print& p) const = 0;
        virtual ~Printable()                   = default;
    };

} // namespace rdl

#include <ostream>

inline std::ostream& operator<<(std::ostream& os, arduino::Printable const& that) {
    arduino::Print_stdstring buf;
    that.printTo(buf);
    os << buf.str();
    return os;
}
