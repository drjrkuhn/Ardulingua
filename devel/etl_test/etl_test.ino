#include <Embedded_Template_Library.h>

#include "etl/numeric.h"
#include "etl/vector.h"

#include <numeric>

// #include <algorithm>
// #include <iostream>
// #include <iterator>

namespace std {

  template <typename T, const size_t MAX_SIZE_>
  using vector = etl::vector<T,MAX_SIZE_>;

  template <typename T>
  using ivector = etl::ivector<T>;
}

void print(const std::ivector<int>& v) {
    for (auto a : v) {
        Serial.print(a);
        Serial.print(' ');
    }
    Serial.println();
}

void setup() {
    Serial.begin(9600);
    while (!Serial)
        ;

    // Declare the vector instances.
    std::vector<int, 10> v1(10);
    std::vector<int, 20> v2(20);
    std::iota(v1.begin(), v1.end(), 0);  // Fill with 0 through 9
    std::iota(v2.begin(), v2.end(), 10); // Fill with 10 through 29
    print(v1);                           // Prints 0 through 9
    print(v2);                           // Prints 10 through 29
}

void loop() {
    // put your main code here, to run repeatedly:
}
