#include <iostream>
#include <map>
#include <string>

#if 1
    #include <JsonDelegate.h>
    #include <ServerProperty.h>

using namespace rdl;

using MapT = std::map<std::string, rdl::json_stub>;

// template <class DT, typename T, class StrT, class MapT, long MAX_SIZE>
// class simple_sequencable_base : public simple_sequencable_base<simple_prop<DT,T,StrT,MapT,MAX_SIZE>, T, StrT, MapT> {

template <typename T, long MAX_SIZE>
class simple_prop : public simple_prop_base<simple_prop<T, MAX_SIZE>, T, std::string, MapT, MAX_SIZE> {
 public:
    using BaseT = simple_prop_base<simple_prop<T, MAX_SIZE>, T, std::string, MapT, MAX_SIZE>;

    simple_prop(const std::string& brief_name, const T initial, bool sequencable = false)
        : BaseT(brief_name, initial, sequencable) {}
    using BaseT::get;
    using BaseT::set;
    using BaseT::max_size;
    using BaseT::clear;
    using BaseT::add;
    using BaseT::start;
    using BaseT::sequencable;
    using BaseT::message;
    using BaseT::add_to;
};

int main() {

    using namespace std;

    // MapT map;

    using S = simple_prop<int, 64>;

    S foo("foo", 10);
    json_delegate<void, int> jd;
    typedef void (S::*TMethod)(int);
    TMethod meth = &simple_prop<int, 64>::set;
    jd           = json_delegate<void,int>::template create_m<S, meth>(&foo);

    // foo.add_to(map);

    foo.set(10);
    cout << foo.get() << endl;

    return 0;
}

#else

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

#endif