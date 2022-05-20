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

    #include <rdl/Arraybuf.h>
    #include <rdl/std_utility.h> // for std::move

using namespace rdl;

template <typename T>
class Test {
 public:
    // buffer uses rvalue move constructor. note: the static_cast is effectively std::move without
    Test(arraybuf<T>&& rval_buffer) : buffer_(std::move(rval_buffer)) {}
    // buffer uses copy constructor
    Test(arraybuf<T>& lval_buffer) : buffer_(lval_buffer) {}
    arraybuf<T>& buffer() const { return buffer_; }

    T& operator[](size_t idx) { return buffer_[idx]; }
    const T& operator[](size_t idx) const { return buffer_[idx]; }

 protected:
    Test() : buffer_() {}
    arraybuf<T> buffer_;
};

template <typename T, size_t BUFSIZE>
class StaticTest : public Test<T> {
 public:
    StaticTest() : Test<T>() {
        // sbuffer_ not instantiated until after the base class
        std::cout << "StaticTest() constructor\n";
        Test<T>::buffer_ = std::move(sbuffer_);
    }

    static_arraybuf<T, BUFSIZE> sbuffer_;
};

template <typename T>
class DynamicTest : public Test<T> {
 public:
    DynamicTest(size_t max_size) : Test<T>(std::move(dynamic_arraybuf<T>(max_size))) {
    }
};

    #define QPRINT_(line) std::cout << #line
    #define QPRINTLN_(line) std::cout << #line << std::endl
    #define QRUN(line)   \
        QPRINTLN_(line); \
        line;
    #define QRESULT(operation) \
        QPRINT_(operation);    \
        std::cout << " -> " << operation << std::endl;

int main() {

    using namespace std;

    cout << "##### Running sample arraybuf code ######" << endl;
    {
        using namespace rdl;
        QPRINTLN_(Primary use cases. Uses move constructor.);
        QPRINTLN_(Final arraybuf responsible for freeing memory.);
        {arraybuf<long> sample_1s = static_arraybuf<long, 100>();}
        {arraybuf<long> sample_1d = dynamic_arraybuf<long>(100);}

        QPRINTLN_(Alternative use case. Uses copy constructor.);
        QPRINTLN_(Original xxx_arraybuf responsible for freeing memory.);
        {static_arraybuf<long, 100> b2s;
        arraybuf<long> sample_2s = b2s;}
        {dynamic_arraybuf<long> b2d(100);
        arraybuf<long> sample_2d = b2d;}

        QPRINTLN_(Alternative use case. Uses move assignment.);
        QPRINTLN_(Final arraybuf responsible for freeing memory.);
        {arraybuf<long> sample_3s;
        static_arraybuf<long, 100> b3s;
        sample_3s = std::move(b3s);}
        {arraybuf<long> sample_3d;
        dynamic_arraybuf<long> b3d(100);
        sample_3d = std::move(b3d);}
    }

    cout << "=== Testing Dynamic and Static arraybuf constructors ===" << endl;
    {
        QRUN(rdl::arraybuf<int> a0);
        QRESULT(a0.valid());
    }
    {
        QRUN(rdl::arraybuf<int> a1 = (rdl::static_arraybuf<int, 11>()));
        QRESULT(a1.valid());
        QRUN(a1[0] = 11111);
        QRESULT(a1[0]);
    }
    {
        QRUN(rdl::arraybuf<int> a2 = rdl::dynamic_arraybuf<int>(12));
        QRESULT(a2.valid());
        QRUN(a2[0] = 22222);
        QRESULT(a2[0]);
    }
    cout << "=== Testing Dynamic and Static arraybuf assignment ===" << endl;
    {
        QRUN(rdl::arraybuf<int> a1);
        QRUN(a1 = std::move(rdl::static_arraybuf<int, 11>()));
        QRESULT(a1.valid());
        QRUN(a1[0] = 33333);
        QRESULT(a1[0]);
    }
    {
        QRUN(rdl::arraybuf<int> a2);
        QRUN(a2 = std::move(rdl::dynamic_arraybuf<int>(12)));
        QRESULT(a2.valid());
        QRUN(a2[0] = 44444);
        QRESULT(a2[0]);
    }

    // return 0;
    cout << "=== StaticTest ===" << endl;
    {
        QRUN(Test<int> t1a = (StaticTest<int, 10>{}));
        QRUN(t1a[0] = 11111);
        QRESULT(t1a[0]);
    }
    cout << "=== DynamicTest ===" << endl;
    {
        QRUN(Test<int> t1b = DynamicTest<int>(11));
        QRUN(t1b[0] = 22222);
        QRESULT(t1b[0]);
    }
    cout << "=== Base Test dynamic arrays as members ===" << endl;
    {
        QRUN(Test<int> t2a((dynamic_arraybuf<int>(10))));
        QRUN(t2a[0] = 33333);
        QRESULT(t2a[0]);
    }
    {
        QRUN(rdl::arraybuf<int> buf2 = dynamic_arraybuf<int>(10));
        QRUN(Test<int> t2b(buf2));
        QRUN(t2b[0] = 44444);
        QRESULT(t2b[0]);
    }

    cout << "=== Base Test static arrays as members ===" << endl;
    {
        QRUN(Test<int> t3a((static_arraybuf<int, 10>())));
        QRUN(t3a[0] = 55555);
        QRESULT(t3a[0]);
    }
    {
        cout << "rdl::static_arraybuf<int, 10> buf3;\n";
        rdl::static_arraybuf<int, 10> buf3;
        QRUN(Test<int> t3b(buf3));
        QRUN(t3b[0] = 66666);
        QRESULT(t3b[0]);
    }
    return 0;
}

#endif