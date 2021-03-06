#include <rdl/sys_StringT.h>
#include <rdl/sys_StreamT.h>
#include <rdl/ServerProperty.h>
#include <rdl/JsonDelegate.h>
#include <rdl/JsonProtocol.h>
#include <rdl/JsonClient.h>
#include <rdl/JsonServer.h>
#include <rdl/Logger.h>
#include <rdl/std_utility.h>
#include <rdl/sys_timing.h>
#include <unordered_map>
// #include <WString.h>
// #include <Print.h>
// #include <Print_std.h>
// #include <Stream.h>
// #include <Stream_std.h>
#include <thread>
#include <future>
#include <iostream>


// #include <string>
// #include <sstream>

using namespace rdl;

sys::Print_ostream<std::ostream> clientlogger(std::cout);
sys::Print_ostream<std::ostream> serverlogger(std::cout);

using MapT = std::unordered_map<sys::StringT, json_stub, sys::string_hash>;

MapT dispatch_map;

rdl::simple_prop<int> foo = rdl::static_simple_prop<int,32>("foo",1);

rdl::simple_prop<double> bar0 = rdl::static_simple_prop<double,32>("bar0", 1.1f);
rdl::simple_prop<double> bar1= rdl::static_simple_prop<double,32>("bar1", 2.2f);
rdl::simple_prop<double> bar2= rdl::static_simple_prop<double,32>("bar2", 3.3f);
rdl::simple_prop<double> bar3= rdl::static_simple_prop<double,32>("bar3", 4.4f);

decltype(bar0)::RootT* all_bars[] = {&bar0, &bar1, &bar2, &bar3};

rdl::channel_prop<double> bars("bar", all_bars, 4);


// std::stringstream ss_toserver;
// std::stringstream ss_fromserver;

sys::Stream_StringT toserver;
sys::Stream_StringT fromserver;

////////// SERVER CODE /////////////

#define SERVER_COL "\t\t\t\t"

json_server<MapT, jsonrpc_default_keys> server = static_json_server<MapT, jsonrpc_default_keys, 512> (toserver, fromserver, dispatch_map);

// rdl::debug_type<ClientT> __;
// rdl::debug_type<decltype(server)>(server);

int setup_server() {
    server.logger(&serverlogger);
    foo.logger(&serverlogger);
    bar0.logger(&serverlogger);
    bar1.logger(&serverlogger);
    bar2.logger(&serverlogger);
    bar3.logger(&serverlogger);
    bars.logger(&serverlogger);

// #if 1
//     // add as list
//     decltype(barA)::RootT* barlist[3] = {&barA, &barB, &barC};
//     bars.add(barlist, 3);
// #else
//     // add individually
//     bars.add(&barA);
//     bars.add(&barB);
//     bars.add(&barC);
// #endif

    add_to<MapT,decltype(foo)::RootT>(dispatch_map, foo, foo.sequencable(), foo.read_only());
    add_to<MapT,decltype(bars)::RootT>(dispatch_map, bars, bars.sequencable(-1), bars.read_only(-1));

    bars.logger(&serverlogger);

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
        sys::yield();
    }
    std::cout << SERVER_COL "SERVER Thread End" << std::endl;
}

int start_server() {
    std::cout << "CLIENT start server" << std::endl;
    std::future<void> stopFuture = exitSignal.get_future();
    server_thread = std::thread(&server_thread_fn, std::move(stopFuture));
    sys::delay(20);
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

json_client<jsonrpc_default_keys> client = static_json_client<jsonrpc_default_keys, 512>(fromserver, toserver);

int main() {

    using namespace std;

    setup_server();
    start_server();
    client.logger(&clientlogger);

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

    double barval;
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


    sys::delay(500);
    stop_server();

    return 0;
}