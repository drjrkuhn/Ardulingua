/*
 * Copyright (c) 2022 MIT.  All rights reserved.
 */

#include "hrslip.h"
#include <catch.hpp>
#include <rdl/SlipInPlace.h>
#include <string>

/**************************************************************************************
 * INCLUDE/MAIN
 **************************************************************************************/

#include <catch.hpp>

using namespace rdl;
using test_encoder = slip_encoder;

TEST_CASE("encoder_slip out-of-place large buffer", "[encoder_slip-oop-01]") {
    size_t ec_size;

    WHEN("empty input") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string src = recode<slip_encoder_hr, test_encoder>("");
        REQUIRE(0 == src.length());
        REQUIRE(1 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE(1 == (ec_size = test_encoder::encode(buf, bsize, src.c_str(), src.length())));
        REQUIRE("#" == recode<test_encoder, slip_encoder_hr>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("\300" == std::string(buf, ec_size));
    }

    WHEN("no specials") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string src = recode<slip_encoder_hr, test_encoder>("Lorus");
        REQUIRE(5 == src.length());
        REQUIRE(6 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE(6 == (ec_size = test_encoder::encode(buf, bsize, src.c_str(), src.length())));
        REQUIRE("Lorus#" == recode<test_encoder, slip_encoder_hr>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\300" == std::string(buf, ec_size));
    }

    WHEN("consecutive specials") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string src = recode<slip_encoder_hr, test_encoder>("Lo^#rus");
        REQUIRE(7 == src.length());
        REQUIRE(10 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE(10 == (ec_size = test_encoder::encode(buf, bsize, src.c_str(), src.length())));
        REQUIRE("Lo^[^Drus#" == recode<test_encoder, slip_encoder_hr>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lo\333\335\333\334rus\300" == std::string(buf, ec_size));
    }

    WHEN("ESC at end") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string src = recode<slip_encoder_hr, test_encoder>("Lorus^");
        REQUIRE(6 == src.length());
        REQUIRE(8 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE(8 == (ec_size = test_encoder::encode(buf, bsize, src.c_str(), src.length())));
        REQUIRE("Lorus^[#" == recode<test_encoder, slip_encoder_hr>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\333\335\300" == std::string(buf, ec_size));
    }

    WHEN("END at end") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string src = recode<slip_encoder_hr, test_encoder>("Lorus#");
        REQUIRE(6 == src.length());
        REQUIRE(8 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE(8 == (ec_size = test_encoder::encode(buf, bsize, src.c_str(), src.length())));
        REQUIRE("Lorus^D#" == recode<test_encoder, slip_encoder_hr>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\333\334\300" == std::string(buf, ec_size));
    }

    WHEN("consecutive specials at end") {
        const size_t bsize = 20;
        char buf[bsize];
        std::string src = recode<slip_encoder_hr, test_encoder>("Lorus^##");
        REQUIRE(8 == src.length());
        REQUIRE(12 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE(12 == (ec_size = test_encoder::encode(buf, bsize, src.c_str(), src.length())));
        REQUIRE("Lorus^[^D^D#" == recode<test_encoder, slip_encoder_hr>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\333\335\333\334\333\334\300" == std::string(buf, ec_size));
    }
}
