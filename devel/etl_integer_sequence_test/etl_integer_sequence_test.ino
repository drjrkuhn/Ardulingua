#include <Embedded_Template_Library.h>

#include <etl/utility.h>


template <typename T>
void print_T(T t) {
    Serial.print(t); 
    Serial.print(' ');
}

// debugging aid
template<typename T, T... ints>
void print_sequence(std::integer_sequence<T, ints...> int_seq)
{
    Serial.print( "The sequence of size ") << int_seq.size() << ": ";
    (print_T(ints),...);
     Serial.println();
}
 
// Convert array into a tuple
template<typename Array, std::size_t... I>
auto a2t_impl(const Array& a, std::index_sequence<I...>)
{
    return std::make_tuple(a[I]...);
}
 
template<typename T, std::size_t N, typename Indices = std::make_index_sequence<N>>
auto a2t(const std::array<T, N>& a)
{
    return a2t_impl(a, Indices{});
}
 
// pretty-print a tuple
 
template<class Ch, class Tr, class Tuple, std::size_t... Is>
void print_tuple_impl(const Tuple& t,
                      std::index_sequence<Is...>)
{
    ({Serial.print(Is == 0? "" : ", "); Serial.print(std::get<Is>(t))}, ...);
}
 
template<class Ch, class Tr, class... Args>
auto& operator<<(const std::tuple<Args...>& t)
{
    Serial.print(')');
    print_tuple_impl(t, std::index_sequence_for<Args...>{});
    Serial.print(')');
}
 
 
void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;


    print_sequence(std::integer_sequence<unsigned, 9, 2, 5, 1, 9, 1, 6>{});
    print_sequence(std::make_integer_sequence<int, 20>{});
    print_sequence(std::make_index_sequence<10>{});
    print_sequence(std::index_sequence_for<float, std::iostream, char>{});
 
    std::array<int, 4> array = {1,2,3,4};
 
    // convert an array into a tuple
    auto tuple = a2t(array);
    static_assert(std::is_same<decltype(tuple),
                               std::tuple<int, int, int, int>>::value, "");
 
    // print it to cout
    std::cout << tuple << '\n';
 

}

void loop() {
  // put your main code here, to run repeatedly:

}
