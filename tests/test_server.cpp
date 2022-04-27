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

int process_message(StringT json) {
    return server.process(json.c_str(), json.length());
}

int main() {

    cout_Print.println("Hello from cout_Stream");
    logger.println("Hello from logger");

    setup_server();

    process_message("{\"m\":\"?foo\", \"i\":1}");
    

    return 0;
}