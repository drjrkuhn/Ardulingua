/*
 * Copyright (c) 2022 MIT.  All rights reserved.
 */

#include <Delegate.h>
#include <string>

/**************************************************************************************
 * INCLUDE/MAIN
 **************************************************************************************/

#include <catch.hpp>

using namespace rdl;

int ifv(void) { return 100; }
int ifi(int a) { return 2 * a; }
int ifii(int a, int b) { return a + b; }

struct C {
    int v;
    C(int _v) { v = _v; }
    void set(int _v) { v = _v; }
    int get() const { return v; }

    int ifv(void) { return v; }
    int ifi(int a) { return 2 * a + v; }
    int ifii(int a, int b) { return a + b + v; }
    int ifv_c(void) const { return v; }
    int ifi_c(int a) const { return 2 * a + v; }
    int ifii_c(int a, int b) const { return a + b + v; }
};

TEST_CASE("delegate basic function", "[delegate-01]") {

    WHEN("functions returning int") {
        auto d_ifv = delegate::of<RetT<int>>::create<ifv>();
        REQUIRE(d_ifv.call<RetT<int>>() == 100);
        REQUIRE(d_ifv.as<RetT<int>>()() == 100);

        auto d_ifi = delegate::of<RetT<int>, int>::create<ifi>();
        REQUIRE(d_ifi.call<RetT<int>, int>(50) == 100);
        REQUIRE(d_ifi.as<RetT<int>, int>()(25) == 50);

        auto d_ifii = delegate::of<RetT<int>, int, int>::create<ifii>();
        REQUIRE(d_ifii.call<RetT<int>, int, int>(80, 20) == 100);
        REQUIRE(d_ifii.as<RetT<int>, int, int>()(10, 20) == 30);
    }
    WHEN("class set/get int") {
        C a(10);
        auto da_set = delegate::of<RetT<void>, int>::create<C, &C::set>(&a);
        auto da_get = delegate::of<RetT<int>>::create<C, &C::get>(&a);
        da_set.call<RetT<void>>(100);
        REQUIRE(da_get.call<RetT<int>>() == 100);
        da_set.as<RetT<void>, int>()(50);
        REQUIRE(da_get.as<RetT<int>>()() == 50);
    }
    WHEN("class non-const methods returning int") {
        C a(10);
        auto d_ifv = delegate::of<RetT<int>>::create<C, &C::ifv>(&a);
        REQUIRE(d_ifv.call<RetT<int>>() == 10);
        REQUIRE(d_ifv.as<RetT<int>>()() == 10);

        auto d_ifi = delegate::of<RetT<int>, int>::create<C, &C::ifi>(&a);
        REQUIRE(d_ifi.call<RetT<int>, int>(50) == 110);
        REQUIRE(d_ifi.as<RetT<int>, int>()(25) == 60);

        auto d_ifii = delegate::of<RetT<int>, int, int>::create<C, &C::ifii>(&a);
        REQUIRE(d_ifii.call<RetT<int>, int, int>(80, 20) == 110);
        REQUIRE(d_ifii.as<RetT<int>, int, int>()(10, 20) == 40);
    }
    WHEN("class const methods returning int") {
        C a(10);
        auto d_ifv = delegate::of<RetT<int>>::create<C, &C::ifv_c>(&a);
        REQUIRE(d_ifv.call<RetT<int>>() == 10);
        REQUIRE(d_ifv.as<RetT<int>>()() == 10);

        auto d_ifi = delegate::of<RetT<int>, int>::create<C, &C::ifi_c>(&a);
        REQUIRE(d_ifi.call<RetT<int>, int>(50) == 110);
        REQUIRE(d_ifi.as<RetT<int>, int>()(25) == 60);

        auto d_ifii = delegate::of<RetT<int>, int, int>::create<C, &C::ifii_c>(&a);
        REQUIRE(d_ifii.call<RetT<int>, int, int>(80, 20) == 110);
        REQUIRE(d_ifii.as<RetT<int>, int, int>()(10, 20) == 40);
    }
    WHEN("lambdas returning int") {
        int capture = 200;
        auto d_ifv = delegate::of<RetT<int>>::create([&capture]() { return capture; });
        REQUIRE(d_ifv.call<RetT<int>>() == 200);
        REQUIRE(d_ifv.as<RetT<int>>()() == 200);

        auto d_ifi = delegate::of<RetT<int>, int>::create([](int a) { return 2 * a; });
        REQUIRE(d_ifi.call<RetT<int>, int>(50) == 100);
        REQUIRE(d_ifi.as<RetT<int>, int>()(25) == 50);

        auto d_ifii = delegate::of<RetT<int>, int, int>::create([](int a, int b) { return a + b; });
        REQUIRE(d_ifii.call<RetT<int>, int, int>(80, 20) == 100);
        REQUIRE(d_ifii.as<RetT<int>, int, int>()(10, 20) == 30);
    }
}
