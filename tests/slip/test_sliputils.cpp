#include <SlipInPlace.h>

/**************************************************************************************
 * TESTS
 **************************************************************************************/

#include <catch.hpp>

TEST_CASE("escaped", "[slip_utils-01]") {
    using namespace rdl;
    using namespace std;

    const size_t bsize = 64;
    char buf[bsize];

    size_t size;
    string res, src("\'\"\?\\\a\b\f\n\r\t\vABCabc\xC0\xC1");
    //string src("abc\t");
    size = escaper::escape(buf, bsize, src.c_str(), src.length(), "[]");
    res = string(buf, size);
    REQUIRE("[\\'\\\"\\?\\\\\\a\\b\\f\\n\\r\\t\\vABCabc\\xC0\\xC1]" == res);
    // bracket pairs
    size = escaper::escape(buf, bsize, src.c_str(), src.length(), "\"\"");
    res = string(buf, size);
    REQUIRE("\"\\'\\\"\\?\\\\\\a\\b\\f\\n\\r\\t\\vABCabc\\xC0\\xC1\"" == res);
    // single bracket
    size = escaper::escape(buf, bsize, src.c_str(), src.length(), "\"");
    res = string(buf, size);
    REQUIRE("\"\\'\\\"\\?\\\\\\a\\b\\f\\n\\r\\t\\vABCabc\\xC0\\xC1\"" == res);
    // single bracket
    size = escaper::escape(buf, bsize, src.c_str(), src.length(), "'");
    res = string(buf, size);
    REQUIRE("\'\\'\\\"\\?\\\\\\a\\b\\f\\n\\r\\t\\vABCabc\\xC0\\xC1\'" == res);
    // no brackets
    size = escaper::escape(buf, bsize, src.c_str(), src.length(), "");
    res = string(buf, size);
    REQUIRE("\\'\\\"\\?\\\\\\a\\b\\f\\n\\r\\t\\vABCabc\\xC0\\xC1" == res);
    // NULL brackets
    size = escaper::escape(buf, bsize, src.c_str(), src.length(), NULL);
    res = string(buf, size);
    REQUIRE("\\'\\\"\\?\\\\\\a\\b\\f\\n\\r\\t\\vABCabc\\xC0\\xC1" == res);
}