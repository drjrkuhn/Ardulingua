/*
 * Copyright (c) 2022 MIT.  All rights reserved.
 */

#include "hrslip.h"
#include <catch.hpp>
#include <SlipInPlace.h>
#include <string>

/**************************************************************************************
 * INCLUDE/MAIN
 **************************************************************************************/

#include <catch.hpp>

using namespace rdl;
using test_decoder = null_decoder;

TEST_CASE("decoder_null out-of-place large buffer", "[decoder_null-oop-01]") {
    size_t dc_size;

    WHEN("empty input") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<decoder_hrnull, test_decoder>("");
        REQUIRE(0 == srcstr.length());
        REQUIRE(0 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE(dc_size == 0);
    }

    WHEN("null char input") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<decoder_hrnull, test_decoder>("^@#");
        REQUIRE(3 == srcstr.length());
        REQUIRE(1 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(1 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE("0" == recode<test_decoder, decoder_hrnull>(buf, dc_size));
    }

    WHEN("single end at input") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<decoder_hrnull, test_decoder>("#");
        REQUIRE(1 == srcstr.length());
        REQUIRE(0 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE("" == recode<test_decoder, decoder_hrnull>(buf, dc_size));
    }

    WHEN("no specials") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<decoder_hrnull, test_decoder>("Lorus#");
        REQUIRE(6 == srcstr.length());
        REQUIRE(5 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(5 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE("Lorus" == recode<test_decoder, decoder_hrnull>(buf, dc_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus" == std::string(buf, dc_size));
    }

    WHEN("consecutive specials") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<decoder_hrnull, test_decoder>("Lo^[^D^@rus#");
        REQUIRE(12 == srcstr.length());
        REQUIRE(8 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(8 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE("Lo^#0rus" == recode<test_decoder, decoder_hrnull>(buf, dc_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        char zbuf[]{'L', 'o', '\333', '\300', '\0', 'r', 'u', 's'};
        REQUIRE(std::string(zbuf, 8) == std::string(buf, dc_size));
    }

    WHEN("ESC at end") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<decoder_hrnull, test_decoder>("Lorus^[#");
        REQUIRE(8 == srcstr.length());
        REQUIRE(6 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(6 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE("Lorus^" == recode<test_decoder, decoder_hrnull>(buf, dc_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\333" == std::string(buf, dc_size));
    }

    WHEN("END at end") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<decoder_hrnull, test_decoder>("Lorus^D#");
        REQUIRE(8 == srcstr.length());
        REQUIRE(6 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(6 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE("Lorus#" == recode<test_decoder, decoder_hrnull>(buf, dc_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\300" == std::string(buf, dc_size));
    }

    WHEN("NULL at end") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<decoder_hrnull, test_decoder>("Lorus^@#");
        REQUIRE(8 == srcstr.length());
        REQUIRE(6 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(6 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE("Lorus0" == recode<test_decoder, decoder_hrnull>(buf, dc_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        char zbuf[]{'L', 'o', 'r', 'u', 's', '\0'};
        REQUIRE(std::string(zbuf, 6) == std::string(buf, dc_size));
    }

    WHEN("consecutive specials at end") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<decoder_hrnull, test_decoder>("Lorus^@^[^D^D#");
        REQUIRE(14 == srcstr.length());
        REQUIRE(9 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(9 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE("Lorus0^##" == recode<test_decoder, decoder_hrnull>(buf, dc_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        char zbuf[]{'L', 'o', 'r', 'u', 's', '\0', '\333', '\300', '\300'};
        REQUIRE(std::string(zbuf, 9) == std::string(buf, dc_size));
    }
}
