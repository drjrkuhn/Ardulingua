#include <iomanip>
#include <iostream>
#include <string>

#if 0

    #include <JsonDelegate.h>
    #include <ServerProperty.h>

using namespace rdl;

using MapT = std::map<sys::StringT, rdl::json_stub>;

// template <class DT, typename T, class StrT, class MapT, long MAX_SIZE>
// class simple_sequencable_base : public simple_sequencable_base<simple_prop<DT, T, StrT, MapT, MAX_SIZE>, T, StrT, MapT> {

template <typename T, long MAX_SIZE>
using simple_prop = simple_prop<T, sys::StringT, MAX_SIZE>;

// class simple_prop : public simple_prop<T, sys::StringT, MAX_SIZE> {
//  public:
//     using BaseT    = simple_prop<T, sys::StringT, MAX_SIZE>;
//     using typename BaseT::PropAnyT;

//     simple_prop(const sys::StringT& brief_name, const T initial, bool sequencable = false)
//         : BaseT(brief_name, initial, sequencable) {}
// };

template <typename T, long MAX_CHANNELS>
using channel_prop = channel_prop<T, sys::StringT, MAX_CHANNELS>;

template <typename T>
struct TD;

int main() {

    using namespace std;

    MapT dmap;

    typedef simple_prop<int, sys::StringT, 64> SP;
    // TD<SP> td_sp;
    using SBase = typename SP::PropAnyT;
    // TD<SBase> td_sbase;
    // TD<SBase::json_delegates> td_jsigs;

    SP foo("foo", 10, true);
    foo.set(10);
    cout << foo.get() << endl;

    add_to<MapT,SBase>(dmap, foo, foo.sequencable());

    cout << "map keys\n";
    for (auto p : dmap) {
        cout << p.first << endl;
    }

    // TD<decltype(SBase::signatures)> td2;

    // json_delegate<void, int> jd;

    // typedef void (SP::*TMethod)(int);
    // TMethod meth = &SP::set;

    // TD<decltype(foo)> td1;
    // TD<decltype(meth)> td2;
    // typename decltype( simple_prop<int, sys::StringT, 64> )::_;
    // typename decltype(&simple_prop<int, sys::StringT, 64>::set)::_;

    // auto jd2 = json_delegate<void, int>::create_m<SP, &SP::set>(&foo);

    // foo.add_to(map);

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

    sys::StringT ss;
    ss += 10;

    Derived<int, sys::StringT> d(10);

    cout << d.get() << endl;
    cout << d.get_s() << endl;

    return 0;
}

#elif 0

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

#elif 0

// HASH functions. See Stack overflow
// https://stackoverflow.com/questions/7666509/hash-function-for-string

    #include <WString.h>
    #include <iomanip>
    #include <iostream>
    #include <map>
    #include <unordered_map>
    #include <utility> // for pair

/**
 * unordered_map hash function for arduino::String
 * Jenkins one-at-a-time 32-bits hash function
 * see https://stackoverflow.com/questions/7666509/hash-function-for-string
 */
class String_hash {
 public:
    size_t operator()(const String& s) const {
        size_t len      = s.length();
        const char* key = s.c_str();
        size_t hash, i;
        for (hash = i = 0; i < len; ++i) {
            hash += key[i];
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);
        return hash;
    }
};

void print_hash(String& s) {
    using namespace std;
    String_hash hasher;
    cout << s.c_str() << (s.length() < 8 ? "\t\t" : "\t") << setfill('0') << setw(16) << hex << hasher(s) << endl;
}

String stringArray[] = {
    "Lorem", "ipsum", "dolor", "sit", "amet", "consectetur", "adipiscing", "elit",
    "sed", "do", "eiusmod", "tempor", "incididunt", "ut", "labore", "et", "dolore", "magna", "aliqua"};

int main() {

    using namespace std;
    using namespace arduino;

    String str("Hello world");
    cout << str.c_str() << endl;

    using MapT = unordered_map<String, int, String_hash>;

    MapT amap;
    amap.insert(MapT::value_type("Hello", 10));
    amap.insert(MapT::value_type("World", 20));

    cout << amap["Hello"] << endl;
    cout << amap["World"] << endl;

    cout << "=== String hashes ===\n";

    for (auto s : stringArray) {
        print_hash(s);
    }

    return 0;
}

#elif 1

    #include <rdl/Array.h>
    #include <rdl/std_utility.h> // for std::move

using namespace rdl;

template <typename T>
class Test {
 public:
    // buffer uses rvalue move constructor. note: the static_cast is effectively std::move without
    Test(array<T>&& buffer) : buffer_(std::move(buffer)) {}
    // buffer uses copy constructor
    Test(array<T>& buffer) : buffer_(buffer) {}
    array<T> buffer() const { return buffer_; }

    T& operator[](size_t idx) { return buffer_[idx]; }
    const T& operator[](size_t idx) const { return buffer_[idx]; }

 protected:
    Test() : buffer_() {}
    array<T> buffer_;
};

template <typename T, size_t BUFSIZE>
class StaticTest : public Test<T> {
 public:
    StaticTest() : Test<T>() {
        // sbuffer_ not instantiated until after the base class
        Test<T>::buffer_ = sbuffer_;
    }

    static_array<T, BUFSIZE> sbuffer_;
};

template <typename T>
class DynamicTest : public Test<T> {
 public:
    DynamicTest(size_t max_size) : Test<T>(std::move(dynamic_array<T>(max_size))) {
    }
};

int
main() {
    using namespace std;

    cout << "=== Testing Dynamic and Static arrays ===" << endl;

    {
        cout << "rdl::array<int> a0;\n";
        rdl::array<int> a0;
        cout << "a0.valid() = " << a0.valid() << "\n";
    }
    {
        cout << "rdl::array<int> a1 = rdl::static_array<int,11>();\n";
        rdl::array<int> a1 = rdl::static_array<int, 11>();
        cout << "a1.valid() = " << a1.valid() << "\n";
    }
    {
        cout << "rdl::array<int> a2 = rdl::dynamic_array<int>(12);\n";
        rdl::array<int> a2 = rdl::dynamic_array<int>(12);
        cout << "a2.valid() = " << a2.valid() << "\n";
    }
    cout << "=== StaticTest ===" << endl;
    {
        cout << "Test<int> t1a = StaticTest<int, 10>();\n";
        Test<int> t1a = StaticTest<int, 10>();
        t1a[0]        = 10;
        cout << t1a[0] << endl;
    }
    cout << "=== DynamicTest ===" << endl;
    {
        cout << "Test<int> t1b = DynamicTest<int>(11);\n";
        Test<int> t1b = DynamicTest<int>(11);
        t1b[0]        = 10;
        cout << t1b[0] << endl;
    }
    cout << "=== Base Test dynamic arrays as members ===" << endl;
    {
        cout << "Test<int> t2a((dynamic_array<int>(10)));\n";
        Test<int> t2a((dynamic_array<int>(10)));
        t2a[5] = 20;
        cout << t2a[5] << endl;
    }
    {
        cout << "rdl::array<int> buf2(10);\n";
        rdl::array<int> buf2 = dynamic_array<int>(10);
        cout << "Test<int> t2b((dynamic_array<int>(10)));\n";
        Test<int> t2b(buf2);
        t2b[5] = 20;
        cout << t2b[5] << endl;
    }

    cout << "=== Base Test static arrays as members ===" << endl;
    {
        cout << "Test<int> t3a((static_array<int, 10>()));\n";
        Test<int> t3a((static_array<int, 10>()));
        t3a[6] = 60;
        cout << t3a[6] << endl;
    }
    {
        cout << "static_array<int, 10> buf3;\n";
        static_array<int, 10> buf3;
        cout << "Test<int> t3b((static_array<int, 10>()));\n";
        Test<int> t3b(buf3);
        t3b[6] = 60;
        cout << t3b[6] << endl;
    }
    return 0;
}

#endif