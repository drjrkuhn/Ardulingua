#include <ServerProperty.h>
#include <JsonDelegate.h>
#include <JsonDispatch.h>
#include <unordered_map>
#include <WString.h>
#include <Print.h>
#include <Print_std.h>
#include <Stream.h>
#include <Stream_std.h>
#include <Logger.h>
#include <thread>
#include <future>
#include <Polyfills/sys_timing.h>


#include <string>
#include <sstream>

using namespace rdl;
using namespace arduino;

using StringT = std::string;

Print_stdostream<std::ostream> cout_Print(std::cout);
using LoggerT = logger_base<Print, StringT>;
LoggerT logger(&cout_Print);

using MapT = std::unordered_map<StringT, json_stub, string_hash<StringT>>;

MapT dispatch_map;

template <typename T>
using simple_prop = simple_prop_base<T, StringT, 64>;

template <typename T>
using channel_prop = channel_prop_base<T, StringT, 10>;

simple_prop<int> foo("foo", 1);
simple_prop<float> barA("barA", 1.0);
simple_prop<float> barB("barB", 2.0);
simple_prop<float> barC("barC", 3.0);

channel_prop<float> bars("bar");

template <typename T>
struct TYPEDEBUG;


std::stringstream ss_toserver;
std::stringstream ss_fromserver;
using StreamT = Stream_stdstream<std::stringstream>;

StreamT toserver(&ss_toserver);
StreamT fromserver(&ss_fromserver);

using ServerT = json_server<StreamT, StreamT, MapT, StringT, 512, LoggerT>;
using ClientT = json_client<StreamT, StreamT, StringT, 512, LoggerT>;

////////// SERVER CODE /////////////

ServerT server(toserver, fromserver, dispatch_map);

int setup_server() {
    server.logger(logger);

    bars.add(reinterpret_cast<decltype(bars)::ChanT*>(&barA));
    bars.add(reinterpret_cast<decltype(bars)::ChanT*>(&barB));
    bars.add(reinterpret_cast<decltype(bars)::ChanT*>(&barC));

    add_to<MapT,decltype(foo)::PropAnyT>(dispatch_map, foo, foo.sequencable());
    add_to<MapT,decltype(bars)::PropAnyT>(dispatch_map, bars, bars.all_sequencable());

    return 0;
}

std::promise<void> exitSignal;
std::thread server_thread;

void server_thread_fn (std::future<void> stopFuture) {
    std::cout << "SERVER Thread Start" << std::endl;
    while (stopFuture.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) {
        int ret = server.check_messages();
        if (ret != ERROR_OK) {
            std::cerr << "SERVER ERROR: " << ret << std::endl;
        }
        sys_yield();
    }
    std::cout << "SERVER Thread End" << std::endl;
}

int start_server() {
    std::future<void> stopFuture = exitSignal.get_future();
    server_thread = std::thread(&server_thread_fn, std::move(stopFuture));
    sys_delay(20);
    return 0;
}

int stop_server() {
    std::cout << "Asking Server Thread to Stop" << std::endl;
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
    client.logger(logger);

    int fooval;
    client.call_get("?foo", fooval);
    // cout << "toserver: " << ss_toserver.str() << "  fromserver: " << ss_fromserver.str() << endl;;
    sys_yield();
    cout << "foo = " << fooval << endl;
    client.call("!foo", 120);
    // cout << "toserver: " << ss_toserver.str() << "  fromserver: " << ss_fromserver.str() << endl;;
    sys_yield();
    client.call_get("?foo", fooval);
    // cout << "toserver: " << ss_toserver.str() << "  fromserver: " << ss_fromserver.str() << endl;;
    sys_yield();
    cout << "foo = " << fooval << endl;
    client.call_get("?foo", fooval);
    // cout << "toserver: " << ss_toserver.str() << "  fromserver: " << ss_fromserver.str() << endl;;
    sys_yield();
    cout << "foo = " << fooval << endl;

    delay(5000);
    stop_server();

    return 0;
}