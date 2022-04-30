/*
 * Copyright (c) 2022 MIT.  All rights reserved.
 */

#include "hrslip.h"
#include <catch.hpp>
#include <rdl/sys_StringT.h>
#include <rdl/SlipInPlace.h>

/**************************************************************************************
 * INCLUDE/MAIN
 **************************************************************************************/

#include <catch.hpp>

using namespace rdl;
using test_encoder = slip_null_encoder;

TEST_CASE("encoder_null out-of-place large buffer", "[encoder_null-oop-01]") {
    size_t ec_size;

    WHEN("empty input") {
        const size_t bsize = 20;
        char buf[bsize];
        sys::StringT src = recode<slip_encoder_hrnull, test_encoder>("");
        REQUIRE(0 == src.length());
        REQUIRE(1 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE(1 == (ec_size = test_encoder::encode(buf, bsize, "", 0)));
        REQUIRE("#" == recode<test_encoder, slip_encoder_hrnull>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("\300" == sys::StringT(buf, ec_size));
    }

    WHEN("null char input") {
        const size_t bsize = 20;
        char buf[bsize];
        sys::StringT src = recode<slip_encoder_hrnull, test_encoder>("0");
        REQUIRE(1 == src.length());
        REQUIRE( 3 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE( 3 == (ec_size = test_encoder::encode(buf, bsize, src.c_str(), src.length())));
        REQUIRE("^@#" == recode<test_encoder, slip_encoder_hrnull>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("\333\336\300" == sys::StringT(buf, ec_size));
    }

    WHEN("no specials") {
        const size_t bsize = 20;
        char buf[bsize];
        sys::StringT src = recode<slip_encoder_hrnull, test_encoder>("Lorus");
        REQUIRE(5 == src.length());
        REQUIRE( 6 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE( 6 == (ec_size = test_encoder::encode(buf, bsize, src.c_str(), src.length())));
        REQUIRE("Lorus#" == recode<test_encoder, slip_encoder_hrnull>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\300" == sys::StringT(buf, ec_size));
    }

    WHEN("consecutive specials") {
        const size_t bsize = 20;
        char buf[bsize];
        sys::StringT src = recode<slip_encoder_hrnull, test_encoder>("Lo^#0rus");
        REQUIRE(8 == src.length());
        REQUIRE( 12 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE( 12 == (ec_size = test_encoder::encode(buf, bsize, src.c_str(), src.length())));
        REQUIRE("Lo^[^D^@rus#" == recode<test_encoder, slip_encoder_hrnull>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lo\333\335\333\334\333\336rus\300" == sys::StringT(buf, ec_size));
    }
    WHEN("ESC at end") {
        const size_t bsize = 20;
        char buf[bsize];
        sys::StringT src = recode<slip_encoder_hrnull, test_encoder>("Lorus^");
        REQUIRE(6 == src.length());
        REQUIRE( 8 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE( 8 == (ec_size = test_encoder::encode(buf, bsize, src.c_str(), src.length())));
        REQUIRE("Lorus^[#" == recode<test_encoder, slip_encoder_hrnull>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\333\335\300" == sys::StringT(buf, ec_size));
    }

    WHEN("END at end") {
        const size_t bsize = 20;
        char buf[bsize];
        sys::StringT src = recode<slip_encoder_hrnull, test_encoder>("Lorus#");
        REQUIRE(6 == src.length());
        REQUIRE( 8 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE( 8 == (ec_size = test_encoder::encode(buf, bsize, src.c_str(), src.length())));
        REQUIRE("Lorus^D#" == recode<test_encoder, slip_encoder_hrnull>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\333\334\300"== sys::StringT(buf, ec_size));
    }

    WHEN("NULL at end") {
        const size_t bsize = 20;
        char buf[bsize];
        sys::StringT src = recode<slip_encoder_hrnull, test_encoder>("Lorus0");
        REQUIRE(6 == src.length());
        REQUIRE( 8 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE( 8 == (ec_size = test_encoder::encode(buf, bsize, src.c_str(), src.length())));
        REQUIRE("Lorus^@#" == recode<test_encoder, slip_encoder_hrnull>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\333\336\300" == sys::StringT(buf, ec_size));
    }

    WHEN("consecutive specials at end") {
        const size_t bsize = 20;
        char buf[bsize];
        sys::StringT src = recode<slip_encoder_hrnull, test_encoder>("Lorus0^##");
        REQUIRE(9 == src.length());
        REQUIRE( 14 == test_encoder::encoded_size(src.c_str(), src.length()));
        REQUIRE( 14 == (ec_size = test_encoder::encode(buf, bsize, src.c_str(), src.length())));
        REQUIRE("Lorus^@^[^D^D#" == recode<test_encoder, slip_encoder_hrnull>(buf, ec_size));
        /* Standard SLIP: END =\300 ESC =\333 ESCEND =\334 ESCESC =\335 */
        REQUIRE("Lorus\333\336\333\335\333\334\333\334\300" == sys::StringT(buf, ec_size));
    }
}
