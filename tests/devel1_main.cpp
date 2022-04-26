#include <JsonDelegate.h>
#include <ServerProperty.h>

#include <iostream>
#include <map>
#include <string>

using MapT = std::map<std::string, rdl::json_stub>;

template <typename T, long MAX_SIZE>
struct simple_sequencable : rdl::sequenceable_base<T, simple_sequencable, std::string, MapT> {};


int main() {

    using namespace std;

    simple_sequencable foo("foo", 0);

    foo.set(10);
    cout << foo.get() << endl;

    return 0;
}
