#include <rdl/sys_StringT.h>
#include <rdl/ServerProperty.h>
#include <rdl/JsonDelegate.h>
#include <rdl/JsonDispatch.h>
#include <rdl/Logger.h>
#include <rdl/Polyfills/sys_timing.h>
#include <unordered_map>
#include <WString.h>
#include <Print.h>
#include <Print_std.h>
#include <Stream.h>
#include <Stream_std.h>
#include <thread>
#include <future>


#include <string>
#include <sstream>

using namespace rdl;
using namespace arduino;

Print_ostream<std::ostream> cout_Print(std::cout);
using LoggerT = logger_base<Print>;
LoggerT clientlogger(&cout_Print);
LoggerT serverlogger(&cout_Print);

using MapT = std::unordered_map<StringT, json_stub, string_hash>;

MapT dispatch_map;

template <typename T>
using simple_prop = simple_prop_base<T, 64>;

template <typename T>
using channel_prop = channel_prop_base<T, 10>;

simple_prop<int> foo("foo", 1);
simple_prop<float> barA("barA", 1.0);
simple_prop<float> barB("barB", 2.0);
simple_prop<float> barC("barC", 3.0);

channel_prop<float> bars("bar");

template <typename T>
struct TYPEDEBUG;


std::stringstream ss_toserver;
std::stringstream ss_fromserver;
using StreamT = Stream_iostream<std::stringstream>;

StreamT toserver(ss_toserver);
StreamT fromserver(ss_fromserver);

using ServerT = json_server<StreamT, StreamT, MapT, 512>;
using ClientT = json_client<StreamT, StreamT, 512>;

////////// SERVER CODE /////////////

#define SERVER_COL "\t\t\t\t\t\t"

ServerT server(toserver, fromserver, dispatch_map);

int setup_server() {
    server.logger(serverlogger);

#if 1
    // add as list
    decltype(barA)::RootT* barlist[3] = {&barA, &barB, &barC};
    bars.add(barlist, 3);
#else
    // add individually
    bars.add(&barA);
    bars.add(&barB);
    bars.add(&barC);
#endif

    add_to<MapT,decltype(foo)::RootT>(dispatch_map, foo, foo.sequencable(), foo.read_only());
    add_to<MapT,decltype(bars)::RootT>(dispatch_map, bars, bars.sequencable(-1), bars.read_only(-1));

    return 0;
}

std::promise<void> exitSignal;
std::thread server_thread;

void server_thread_fn (std::future<void> stopFuture) {
    std::cout << SERVER_COL "SERVER Thread Start" << std::endl;
    while (stopFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
        int ret = server.check_messages();
        if (ret != ERROR_OK) {
            std::cerr << "SERVER ERROR: " << ret << std::endl;
        }
        sys_yield();
    }
    std::cout << SERVER_COL "SERVER Thread End" << std::endl;
}

int start_server() {
    std::cout << "CLIENT start server" << std::endl;
    std::future<void> stopFuture = exitSignal.get_future();
    server_thread = std::thread(&server_thread_fn, std::move(stopFuture));
    sys_delay(20);
    return 0;
}

int stop_server() {
    std::cout << "CLIENT stop server" << std::endl;
    // stop_server_thread = true;
    exitSignal.set_value();
    server_thread.join();
    std::cout << "Done" << std::endl;
    return 0;
}

////////// CLIENT CODE /////////////

ClientT client(fromserver, toserver);

int main() {

    using namespace std;

    setup_server();
    start_server();
    client.logger(clientlogger);

    int fooval;
    client.call_get("?foo", fooval);
    cout << "foo = " << fooval << endl;
    cout << "foo.set(120)\n";
    client.call("!foo", 120);
    client.call_get("?foo", fooval);
    cout << "foo = " << fooval << endl;

    long numbars;

    client.call_get("^bar", numbars, -1);
    cout << "sizeof(bar) = " << numbars << endl;

    float barval;
    for (int i=0; i<numbars; i++) {
        client.call_get("?bar", barval, i);
        cout << "bar[" << i << "] = " << barval << endl;
    }

    cout << "bar.set(3.14,0)\n";
    client.call("!bar", 3.14, 0);

    cout << "bar.set(6.28,1)\n";
    client.call("!bar", 6.28, 1);

    for (int i=0; i<numbars; i++) {
        client.call_get("?bar", barval, i);
        cout << "bar[" << i << "] = " << barval << endl;
    }


    delay(500);
    stop_server();

    return 0;
}