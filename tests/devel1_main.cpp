#include <iostream>
#include <map>
#include <string>

#if 0
    #include <JsonDelegate.h>
    #include <ServerProperty.h>

using namespace rdl;

using MapT = std::map<std::string, rdl::json_stub>;

// template <class DT, typename T, class StrT, class MapT, long MAX_SIZE>
// class simple_sequencable_base : public simple_sequencable_base<simple_prop<DT,T,StrT,MapT,MAX_SIZE>, T, StrT, MapT> {

// template <typename T, long MAX_SIZE>
// class simple_prop : public simple_prop_base<T, std::string, MAX_SIZE> {
//  public:
//     using BaseT = simple_prop_base<T, std::string, MAX_SIZE>;

//     simple_prop(const std::string& brief_name, const T initial, bool sequencable = false)
//         : BaseT(brief_name, initial, sequencable) {}
// };

template<typename T> struct TD;

int main() {

    using namespace std;

    // MapT map;

    typedef simple_prop_base<int, std::string, 64> SP;

    SP foo("foo", 10);
    json_delegate<void, int> jd;
    typedef void (SP::*TMethod)(int);
    TMethod meth = &SP::set;

    TD<decltype(foo)> td1;
    TD<decltype(meth)> td2;
    // typename decltype( simple_prop_base<int, std::string, 64> )::_;
    // typename decltype(&simple_prop_base<int, std::string, 64>::set)::_;

    auto jd2 = json_delegate<void,int>::create_m<SP, &SP::set >(&foo);

    // foo.add_to(map);

    foo.set(10);
    cout << foo.get() << endl;

    return 0;
}

#elif 0

template <class D, typename T, typename S>
struct Base {
    T get() { return reinterpret_cast<D*>(this)->get_impl(); }
    S get_s() { return S(); }
};

template <typename T, typename S>
struct Derived : Base<Derived<T, S>, T, S> {
    Derived(T value) : value_(value) {}

    T get_impl() { return value_; }

    T value_;
};

int main() {
    using namespace std;

    std::string ss;
    ss += 10;

    Derived<int, std::string> d(10);

    cout << d.get() << endl;
    cout << d.get_s() << endl;

    return 0;
}

#else

    #include <Delegate.h>

struct A {
    int val;
    virtual int foo() {
        std::cout << "A::foo ";
        return val;
    }
    virtual int bar() {
        std::cout << "A::bar ";
        return -val;
    }
};

struct B : public A {
    virtual int bar() {
        std::cout << "B::bar ";
        return -4 * val;
    }
};

int main() {
    using namespace std;
    using namespace rdl;

    A aa;
    aa.val = 2;
    B bb;
    bb.val = 10;

    cout << "aa.A::foo  " << delegate<int>::create<A, &A::foo>(&aa)() << endl; // aa.A::foo -> 2
    cout << "aa.A::bar  " << delegate<int>::create<A, &A::bar>(&aa)() << endl; // aa.A::bar -> -2

    cout << "bb.A::foo  " << delegate<int>::create<A, &A::foo>(&bb)() << endl; // bb.A::foo -> 10
    cout << "bb.A::bar  " << delegate<int>::create<A, &A::bar>(&bb)() << endl; // bb.A::bar -> -40

    // cout << "bb.B::foo  " << delegate<int>::create<B, &B::foo>(&bb)() << endl; // compiler error
    cout << "bb.B::bar  " << delegate<int>::create<B, &B::bar>(&bb)() << endl; // bb.B::bar -> -40

    // cout << "aa.B::bar  " << delegate<int>::create<B, &B::bar>(&aa)() << endl; // compiler error
    return 0;
}

#endif