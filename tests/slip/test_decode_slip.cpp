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
using test_decoder = slip_decoder;

TEST_CASE("decoder_slip out-of-place large buffer", "[decoder_slip-oop-01]") {
    size_t dc_size;

    WHEN("empty input") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<slip_decoder_hrnull, test_decoder>("");
        REQUIRE(0 == srcstr.length());
        REQUIRE(0 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE(dc_size == 0);
    }

    WHEN("single end at input") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<slip_decoder_hr, test_decoder>("#");
        REQUIRE(1 == srcstr.length());
        REQUIRE(0 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        std::string dest = recode<test_decoder, slip_decoder_hr>(buf, dc_size);
        REQUIRE("" == recode<test_decoder, slip_decoder_hrnull>(buf, dc_size));
    }

    WHEN("no specials") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<slip_decoder_hr, test_decoder>("Lorus#");
        REQUIRE(6 == srcstr.length());
        REQUIRE(5 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(5 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE("Lorus" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus" == std::string(buf, dc_size));
    }

    WHEN("bad encoding") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<slip_decoder_hr, test_decoder>("Lo^_rus#");
        REQUIRE(8 == srcstr.length());
        REQUIRE(6 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        // decoded_size does not detect bad encoding!
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
    }

    WHEN("consecutive specials") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<slip_decoder_hr, test_decoder>("Lo^[^Drus#");
        REQUIRE(10 == srcstr.length());
        REQUIRE(7 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(7 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE("Lo^#rus" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lo\333\300rus" == std::string(buf, dc_size));
    }

    WHEN("ESC at end") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<slip_decoder_hr, test_decoder>("Lorus^[#");
        REQUIRE(8 == srcstr.length());
        REQUIRE(6 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(6 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE("Lorus^" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\333" == std::string(buf, dc_size));
    }

    WHEN("END at end") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<slip_decoder_hr, test_decoder>("Lorus^D#");
        REQUIRE(8 == srcstr.length());
        REQUIRE(6 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(6 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE("Lorus#" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\300" == std::string(buf, dc_size));
    }

    WHEN("consecutive specials at end") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string srcstr = recode<slip_decoder_hr, test_decoder>("Lorus^[^D^D#");
        REQUIRE(12 == srcstr.length());
        REQUIRE(8 == test_decoder::decoded_size(srcstr.c_str(), srcstr.length()));
        REQUIRE(8 == (dc_size = test_decoder::decode(buf, bsize, srcstr.c_str(), srcstr.length())));
        REQUIRE("Lorus^##" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\333\300\300" == std::string(buf, dc_size));
    }
}
