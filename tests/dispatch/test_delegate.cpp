/*
 * Copyright (c) 2022 MIT.  All rights reserved.
 */

#include <Delegate.h>
#include <string>
#include <tuple>

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
    void getr(int& _v) { _v = v; }

    int ifv(void) { return v; }
    int ifi(int a) { return 2 * a + v; }
    int ifii(int a, int b) { return a + b + v; }
    int ifv_c(void) const { return v; }
    int ifi_c(int a) const { return 2 * a + v; }
    int ifii_c(int a, int b) const { return a + b + v; }
};

TEST_CASE("delegate basic function", "[delegate-01]") {

    WHEN("functions returning int") {

        delegate<int,int> bad;
        bad(10);

        auto d_ifv = delegate<RetT<int>>::create<ifv>();
        auto s_ifv = d_ifv.stub();
        REQUIRE(d_ifv() == 100);
        REQUIRE(s_ifv.call<RetT<int>>() == 100);
        auto ds_ifv = as<int>(s_ifv);
        REQUIRE(ds_ifv() == 100);

        // delegate d_ifi = delegate::of<RetT<int>, int>::create<ifi>();
        // REQUIRE(d_ifi.call<RetT<int>, int>(50) == 100);
        // REQUIRE(d_ifi.as<RetT<int>, int>()(25) == 50);

        // delegate d_ifii = delegate::of<RetT<int>, int, int>::create<ifii>();
        // REQUIRE(d_ifii.call<RetT<int>, int, int>(80, 20) == 100);
        // REQUIRE(d_ifii.as<RetT<int>, int, int>()(10, 20) == 30);
    }

#if 0
    WHEN("class set/get int") {
        C a(10);
        auto da_set = delegate::of<RetT<void>, int>::create<C, &C::set>(&a);
        auto da_get = delegate::of<RetT<int>>::create<C, &C::get>(&a);
        auto da_getr = delegate::of<RetT<void>,int&>::create<C, &C::getr>(&a);
        da_set.call<RetT<void>>(100);
        REQUIRE(da_get.call<RetT<int>>() == 100);
        da_set.as<RetT<void>, int>()(50);
        REQUIRE(da_get.as<RetT<int>>()() == 50);
        int v = 0;
        da_getr.call<RetT<void>,int&>(v);
        REQUIRE(v == 50);
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
#endif
}

#if 0
TEST_CASE("delegate conversion", "[delegate-02]") {

    WHEN("functions returning int") {
        delegate temp = delegate::of<RetT<int>>::create<ifv>();
        auto f_ifv = temp.as<RetT<int>>();
        REQUIRE(f_ifv() == 100);
        REQUIRE(f_ifv.stub().call<RetT<int>>() == 100);

        temp = delegate::of<RetT<int>, int>::create<ifi>();
        auto f_ifi = temp.as<RetT<int>,int>();
        REQUIRE(f_ifi(50) == 100);
        REQUIRE(f_ifi.stub().call<RetT<int>, int>(50) == 100);

        temp = delegate::of<RetT<int>, int, int>::create<ifii>();
        auto f_ifii = temp.as<RetT<int>, int, int>();
        REQUIRE(f_ifii(10, 20) == 30);
        REQUIRE(f_ifii.stub().call<RetT<int>, int, int>(80, 20) == 100);
    }
}

TEST_CASE("delegate tuple call", "[delegate-31]") {

    WHEN("functions returning int") {
        delegate d_ifv = delegate::of<RetT<int>>::create<ifv>();
        REQUIRE(d_ifv.call_tuple<RetT<int>>(std::make_tuple()) == 100);

        delegate d_ifi = delegate::of<RetT<int>, int>::create<ifi>();
        REQUIRE(d_ifi.call_tuple<RetT<int>>(std::make_tuple<int>(50)) == 100);

        delegate d_ifii = delegate::of<RetT<int>, int, int>::create<ifii>();
        REQUIRE(d_ifii.call_tuple<RetT<int>>(std::make_tuple<int,int>(90,10)) == 100);

        REQUIRE(d_ifii.call_tuple<RetT<int>>(std::make_tuple<int,int>(90,10)) == 100);
        
    }
    WHEN("class set/get int") {
        C a(10);
        auto da_set = delegate::of<RetT<void>, int>::create<C, &C::set>(&a);
        auto da_get = delegate::of<RetT<int>>::create<C, &C::get>(&a);
        auto da_getr = delegate::of<RetT<void>, int&>::create<C, &C::getr>(&a);
        da_set.call_tuple<RetT<void>>(std::tuple<int>(100));
        REQUIRE(da_get.call_tuple<RetT<int>>(std::tuple<>()) == 100);
    }
}
#endif