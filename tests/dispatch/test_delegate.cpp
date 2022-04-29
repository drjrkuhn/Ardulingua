/*
 * Copyright (c) 2022 MIT.  All rights reserved.
 */

#include <rdl/StringT.h>
#include <rdl/Delegate.h>
#include <tuple>

/**************************************************************************************
 * INCLUDE/MAIN
 **************************************************************************************/

#include <catch.hpp>

using namespace rdl;

int ifv(void) { return 100; }
int ifi(int cc) { return 2 * cc; }
int ifii(int cc, int b) { return cc + b; }

template <typename T>
struct C_base {
    T v;
    C_base(T _v) { v = _v; }
    virtual void set(int _v) { v = _v; }
    virtual T get() const { return v; }
    virtual void getr(T& _v) { _v = v; }

    virtual T ifv(void) { return v; }
    virtual T ifi(T cc) { return 2 * cc + v; }
    virtual T ifii(T cc, T b) { return cc + b + v; }
    virtual T ifv_c(void) const { return v; }
    virtual T ifi_c(T cc) const { return 2 * cc + v; }
    virtual T ifii_c(T cc, T b) const { return cc + b + v; }
};

template <typename T>
struct D_base : public C_base<T> {
    using BT = C_base<T>;
    using BT::v;
    D_base(T _v) : C_base<T>(_v) {}
    virtual T ifi(T cc) { return cc + 3 * v; }
};

TEST_CASE("delegate basic function", "[delegate-01]") {

    WHEN("functions returning int") {

        auto d_ifv = delegate<RetT<int>>::create<ifv>();
        auto s_ifv = d_ifv.stub();
        REQUIRE(d_ifv() == 100);
        REQUIRE(s_ifv.call<RetT<int>>() == 100);
        REQUIRE(delegate<int>(s_ifv)() == 100);

        auto d_ifi = delegate<RetT<int>, int>::create<ifi>();
        auto s_ifi = d_ifi.stub();
        REQUIRE(d_ifi(50) == 100);
        REQUIRE(s_ifi.call<RetT<int>, int>(50) == 100);
        REQUIRE(delegate<RetT<int>, int>(s_ifi)(25) == 50);

        auto d_ifii = delegate<RetT<int>, int, int>::create<ifii>();
        auto s_ifii = d_ifii.stub();
        REQUIRE(d_ifii(50, 10) == 60);
        REQUIRE(s_ifii.call<RetT<int>, int, int>(40, 10) == 50);
        REQUIRE(delegate<RetT<int>, int, int>(s_ifii)(30, 10) == 40);
    }

    WHEN("class set/get int") {
        using C = C_base<int>;
        C cc(10);
        auto da_set  = delegate<RetT<void>, int>::create<C, &C::set>(&cc);
        auto da_get  = delegate<RetT<int>>::create<C, &C::get>(&cc);
        auto da_getr = delegate<RetT<void>, int&>::create<C, &C::getr>(&cc);
        auto sa_set  = da_set.stub();
        auto sa_get  = da_get.stub();
        da_set(100);
        REQUIRE(da_get() == 100);
        (delegate<RetT<void>,int>(sa_set)) (50);
        REQUIRE(delegate<RetT<int>>(sa_get)() == 50);
        int v = 0;
        da_getr(v);
        REQUIRE(v == 50);
    }

    WHEN("class non-const methods returning int") {
        using C = C_base<int>;
        C cc(10);
        using D = D_base<int>;
        D dd(20);

        // class C
        auto d_ccifv = delegate<RetT<int>>::create<C, &C::ifv>(&cc);
        auto s_ccifv = d_ccifv.stub();
        REQUIRE(d_ccifv() == 10);
        REQUIRE(delegate<RetT<int>>(s_ccifv)() == 10);

        // class D
        auto d_ddifv = delegate<RetT<int>>::create<C, &C::ifv>(&dd);
        auto s_ddifv = d_ddifv.stub();
        REQUIRE(d_ddifv() == 20);
        REQUIRE(delegate<RetT<int>>(s_ddifv)() == 20);

        // class C
        auto d_ccifi = delegate<RetT<int>, int>::create<C, &C::ifi>(&cc);
        auto s_ccifi = d_ccifi.stub();
        REQUIRE(d_ccifi(50) == 110);
        REQUIRE(delegate<RetT<int>, int>(s_ccifi)(25) == 60);

        // class D
        auto d_ddifi = delegate<RetT<int>, int>::create<D, &D::ifi>(&dd);
        auto s_ddifi = d_ddifi.stub();
        REQUIRE(d_ddifi(60) == 120);
        REQUIRE(delegate<RetT<int>, int>(s_ddifi)(30) == 90);

        // class C
        auto d_ccifii = delegate<RetT<int>, int, int>::create<C, &C::ifii>(&cc);
        auto s_ccifii = d_ccifii.stub();
        REQUIRE(d_ccifii(80, 20) == 110);
        REQUIRE(delegate<RetT<int>, int, int>(s_ccifii)(10, 20) == 40);

        // class D
        auto d_ddifii = delegate<RetT<int>, int, int>::create<C, &C::ifii>(&dd);
        auto s_ddifii = d_ddifii.stub();
        REQUIRE(d_ddifii(80, 20) == 120);
        REQUIRE(delegate<RetT<int>, int, int>(s_ddifii)(10, 20) == 50);
    }

    WHEN("class const methods returning int") {
        using C = C_base<int>;
        C cc(10);
        auto d_ifv = delegate<RetT<int>>::create<C, &C::ifv_c>(&cc);
        auto s_ifv = d_ifv.stub();
        REQUIRE(d_ifv() == 10);
        REQUIRE(delegate<RetT<int>>(s_ifv)() == 10);

        auto d_ifi = delegate<RetT<int>, int>::create<C, &C::ifi_c>(&cc);
        auto s_ifi = d_ifi.stub();
        REQUIRE(d_ifi(50) == 110);
        REQUIRE(delegate<RetT<int>, int>(s_ifi)(25) == 60);

        auto d_ifii = delegate<RetT<int>, int, int>::create<C, &C::ifii_c>(&cc);
        auto s_ifii = d_ifii.stub();
        REQUIRE(d_ifii(80, 20) == 110);
        REQUIRE(delegate<RetT<int>, int, int>(s_ifii)(10, 20) == 40);
    }

    WHEN("lambdas returning int") {
        int capture = 200;
        auto d_ifv  = delegate<RetT<int>>::create([&capture]() { return capture; });
        auto s_ifv  = d_ifv.stub();
        REQUIRE(d_ifv() == 200);
        REQUIRE(delegate<RetT<int>>(s_ifv)() == 200);

        auto d_ifi = delegate<RetT<int>, int>::create([](int cc) { return 2 * cc; });
        auto s_ifi = d_ifi.stub();
        REQUIRE(d_ifi(50) == 100);
        REQUIRE(delegate<RetT<int>, int>(s_ifi)(25) == 50);

        auto d_ifii = delegate<RetT<int>, int, int>::create([](int cc, int b) { return cc + b; });
        auto s_ifii = d_ifii.stub();
        REQUIRE(d_ifii(80, 20) == 100);
        REQUIRE(delegate<RetT<int>, int, int>(s_ifii)(10, 20) == 30);
    }
}

TEST_CASE("delegate stubs", "[delegate-02]") {

    WHEN("functions returning int") {
        stub temp  = delegate<RetT<int>>::create<ifv>().stub();
        auto f_ifv = delegate<RetT<int>>(temp);
        REQUIRE(f_ifv() == 100);
        REQUIRE(f_ifv.stub().call<RetT<int>>() == 100);

        temp       = delegate<RetT<int>, int>::create<ifi>().stub();
        auto f_ifi = delegate<RetT<int>, int>(temp);
        REQUIRE(f_ifi(50) == 100);
        REQUIRE(f_ifi.stub().call<RetT<int>, int>(50) == 100);

        temp        = delegate<RetT<int>, int, int>::create<ifii>().stub();
        auto f_ifii = delegate<RetT<int>, int, int>(temp);
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
        REQUIRE(d_ifii.call_tuple<RetT<int>>(std::make_tuple<int, int>(90, 10)) == 100);
    }

    WHEN("class set/get int") {
        using C = C_base<int>;
        C cc(10);
        stub da_set  = delegate<RetT<void>, int>::create<C, &C::set>(&cc).stub();
        stub da_get  = delegate<RetT<int>>::create<C, &C::get>(&cc).stub();
        stub da_getr = delegate<RetT<void>, int&>::create<C, &C::getr>(&cc).stub();
        da_set.call_tuple<RetT<void>>(std::tuple<int>(100));
        REQUIRE(da_get.call_tuple<RetT<int>>(std::tuple<>()) == 100);
    }
}
