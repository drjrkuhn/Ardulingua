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

template <typename T>
struct C_base {
    T v;
    C_base(T _v) { v = _v; }
    void set(int _v) { v = _v; }
    T get() const { return v; }
    void getr(T& _v) { _v = v; }

    T ifv(void) { return v; }
    T ifi(T a) { return 2 * a + v; }
    T ifii(T a, T b) { return a + b + v; }
    T ifv_c(void) const { return v; }
    T ifi_c(T a) const { return 2 * a + v; }
    T ifii_c(T a, T b) const { return a + b + v; }
};

TEST_CASE("delegate basic function", "[delegate-01]") {

    WHEN("functions returning int") {

        auto d_ifv = delegate<RetT<int>>::create<ifv>();
        auto s_ifv = d_ifv.stub();
        REQUIRE(d_ifv() == 100);
        REQUIRE(s_ifv.call<RetT<int>>() == 100);
        REQUIRE(as<int>(s_ifv)() == 100);

        auto d_ifi = delegate<RetT<int>, int>::create<ifi>();
        auto s_ifi = d_ifi.stub();
        REQUIRE(d_ifi(50) == 100);
        REQUIRE(s_ifi.call<RetT<int>, int>(50) == 100);
        REQUIRE(as<RetT<int>, int>(s_ifi)(25) == 50);

        auto d_ifii = delegate<RetT<int>, int, int>::create<ifii>();
        auto s_ifii = d_ifii.stub();
        REQUIRE(d_ifii(50, 10) == 60);
        REQUIRE(s_ifii.call<RetT<int>, int, int>(40, 10) == 50);
        REQUIRE(as<RetT<int>, int, int>(s_ifii)(30, 10) == 40);
    }

    WHEN("class set/get int") {
        using C = C_base<int>;
        C a(10);
        auto da_set = delegate<RetT<void>, int>::create<C, &C::set>(&a);
        auto da_get = delegate<RetT<int>>::create<C, &C::get>(&a);
        auto da_getr = delegate<RetT<void>,int&>::create<C, &C::getr>(&a);
        auto sa_set = da_set.stub();
        auto sa_get = da_get.stub();
        da_set(100);
        REQUIRE(da_get() == 100);
        as<RetT<void>, int>(sa_set)(50);
        REQUIRE(as<RetT<int>>(sa_get)() == 50);
        int v = 0;
        da_getr(v);
        REQUIRE(v == 50);
    }

    WHEN("class non-const methods returning int") {
        using C = C_base<int>;
        C a(10);
        auto d_ifv = delegate<RetT<int>>::create<C, &C::ifv>(&a);
        auto s_ifv = d_ifv.stub();
        REQUIRE(d_ifv() == 10);
        REQUIRE(as<RetT<int>>(s_ifv)() == 10);

        auto d_ifi = delegate<RetT<int>, int>::create<C, &C::ifi>(&a);
        auto s_ifi = d_ifi.stub();
        REQUIRE(d_ifi(50) == 110);
        REQUIRE(as<RetT<int>, int>(s_ifi)(25) == 60);

        auto d_ifii = delegate<RetT<int>, int, int>::create<C, &C::ifii>(&a);
        auto s_ifii = d_ifii.stub();
        REQUIRE(d_ifii(80, 20) == 110);
        REQUIRE(as<RetT<int>, int, int>(s_ifii)(10, 20) == 40);
    }

    WHEN("class const methods returning int") {
        using C = C_base<int>;
        C a(10);
        auto d_ifv = delegate<RetT<int>>::create<C, &C::ifv_c>(&a);
        auto s_ifv = d_ifv.stub();
        REQUIRE(d_ifv() == 10);
        REQUIRE(as<RetT<int>>(s_ifv)() == 10);

        auto d_ifi = delegate<RetT<int>, int>::create<C, &C::ifi_c>(&a);
        auto s_ifi = d_ifi.stub();
        REQUIRE(d_ifi(50) == 110);
        REQUIRE(as<RetT<int>, int>(s_ifi)(25) == 60);

        auto d_ifii = delegate<RetT<int>, int, int>::create<C, &C::ifii_c>(&a);
        auto s_ifii = d_ifii.stub();
        REQUIRE(d_ifii(80, 20) == 110);
        REQUIRE(as<RetT<int>, int, int>(s_ifii)(10, 20) == 40);
    }

    WHEN("lambdas returning int") {
        int capture = 200;
        auto d_ifv = delegate<RetT<int>>::create([&capture]() { return capture; });
        auto s_ifv = d_ifv.stub();
        REQUIRE(d_ifv() == 200);
        REQUIRE(as<RetT<int>>(s_ifv)() == 200);

        auto d_ifi = delegate<RetT<int>, int>::create([](int a) { return 2 * a; });
        auto s_ifi = d_ifi.stub();
        REQUIRE(d_ifi(50) == 100);
        REQUIRE(as<RetT<int>, int>(s_ifi)(25) == 50);

        auto d_ifii = delegate<RetT<int>, int, int>::create([](int a, int b) { return a + b; });
        auto s_ifii = d_ifii.stub();
        REQUIRE(d_ifii(80, 20) == 100);
        REQUIRE(as<RetT<int>, int, int>(s_ifii)(10, 20) == 30);
    }
}

TEST_CASE("delegate stubs", "[delegate-02]") {

    WHEN("functions returning int") {
        stub temp = delegate<RetT<int>>::create<ifv>().stub();
        auto f_ifv = as<RetT<int>>(temp);
        REQUIRE(f_ifv() == 100);
        REQUIRE(f_ifv.stub().call<RetT<int>>() == 100);

        temp = delegate<RetT<int>, int>::create<ifi>().stub();
        auto f_ifi = as<RetT<int>,int>(temp);
        REQUIRE(f_ifi(50) == 100);
        REQUIRE(f_ifi.stub().call<RetT<int>, int>(50) == 100);

        temp = delegate<RetT<int>, int, int>::create<ifii>().stub();
        auto f_ifii = as<RetT<int>, int, int>(temp);
        REQUIRE(f_ifii(10, 20) == 30);
        REQUIRE(f_ifii.stub().call<RetT<int>, int, int>(80, 20) == 100);
    }
}

TEST_CASE("delegate stub tuple call", "[delegate-03]") {

    WHEN("functions returning int") {
        stub d_ifv = delegate<RetT<int>>::create<ifv>().stub();
        REQUIRE(d_ifv.call_tuple<RetT<int>>(std::make_tuple()) == 100);

        stub d_ifi = delegate<RetT<int>, int>::create<ifi>().stub();
        REQUIRE(d_ifi.call_tuple<RetT<int>>(std::make_tuple<int>(50)) == 100);

        stub d_ifii = delegate<RetT<int>, int, int>::create<ifii>().stub();
        REQUIRE(d_ifii.call_tuple<RetT<int>>(std::make_tuple<int,int>(90,10)) == 100);
    }

    WHEN("class set/get int") {
        using C = C_base<int>;
        C a(10);
        stub da_set = delegate<RetT<void>, int>::create<C, &C::set>(&a).stub();
        stub da_get = delegate<RetT<int>>::create<C, &C::get>(&a).stub();
        stub da_getr = delegate<RetT<void>, int&>::create<C, &C::getr>(&a).stub();
        da_set.call_tuple<RetT<void>>(std::tuple<int>(100));
        REQUIRE(da_get.call_tuple<RetT<int>>(std::tuple<>()) == 100);
    }
}
