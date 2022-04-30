#include <Arduino.h>

// Define a few things in the global namespace
static int g_foo = 10;

struct BarT {
    int bar;
};

template <typename T, typename U>
struct BazT {
    T baz;
    U buz;
};

template <typename T>
T timestwo(const T v) {
    return 2 * v;
}

namespace sys {
    //// redefine functions ////

    constexpr auto& delay = ::delay;
    constexpr auto& yield = ::yield;

    //// redefine globals ////

    constexpr auto& Serial = ::Serial;
    constexpr auto& g_foo  = ::g_foo;

    //// redefine types ////

    using BarT   = ::BarT;
    using String = ::String;

    //// redefine templated types ////

    // Must use template arguments for aliases
    template <typename A, typename B>
    using BazT = ::BazT<A, B>;

    // ERROR: Can't redefine templated methods this way
    // using BazT = ::BazT;

    //// redefine templated functions ////

    // C++11 ERROR: Variable templates are only available in C++14
    // template <typename A>
    // constexpr auto& timestwo = ::timestwo<A>;

    // C++11 workaround 1: redefine by calling
    template <typename A>
    inline A timestwo(A v) { return ::timestwo(v); }

}

void setup() {
    sys::Serial.begin(9600);
    while (!sys::Serial)
        ;
    sys::delay(100);
    sys::yield();
    sys::Serial.println(sys::g_foo);

    sys::BarT bar{20};
    sys::Serial.println(bar.bar);

    sys::BazT<float, long> baz{3.14, 100000};
    sys::Serial.println(baz.baz);
    sys::Serial.println(baz.buz);

    sys::String hello("Hello World!");
    sys::Serial.println(hello);

    sys::Serial.println(sys::timestwo<int>(10));
}

void loop() {
    // put your main code here, to run repeatedly:
}
