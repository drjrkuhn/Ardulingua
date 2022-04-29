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
using test_decoder = slip_decoder_hr;

TEST_CASE("slip_decoder_hr large buffer", "[slip_decoder_hr-01]") {
    size_t dc_size;
    std::string srcstr;
    const char* src;

    bool INPLACE = GENERATE(false, true);

    WHEN("empty input") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<slip_decoder_hr, test_decoder>("");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(0 == srcstr.length());
        REQUIRE(0 == test_decoder::decoded_size(src, srcstr.length()));
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
    }

    WHEN("single end at input") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<slip_decoder_hr, test_decoder>("#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(1 == srcstr.length());
        REQUIRE(0 == test_decoder::decoded_size(src, srcstr.length()));
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE("" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
    }

    WHEN("no specials") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<slip_decoder_hr, test_decoder>("Lorus#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(6 == srcstr.length());
        REQUIRE(5 == test_decoder::decoded_size(src, srcstr.length()));
        REQUIRE(5 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
    }

    WHEN("bad encoding") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<slip_decoder_hr, test_decoder>("Lo^_rus#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(8 == srcstr.length());
        // decoded_size does not detect bad encoding!
        REQUIRE(6 == test_decoder::decoded_size(src, srcstr.length()));
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
    }

    WHEN("consecutive specials") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<slip_decoder_hr, test_decoder>("Lo^[^Drus#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(10 == srcstr.length());
        REQUIRE(7 == test_decoder::decoded_size(src, srcstr.length()));
        REQUIRE(7 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lo^#rus" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
    }

    WHEN("ESC at end") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<slip_decoder_hr, test_decoder>("Lorus^[#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(8 == srcstr.length());
        REQUIRE(6 == test_decoder::decoded_size(src, srcstr.length()));
        REQUIRE(6 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus^" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
    }

    WHEN("END at end") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<slip_decoder_hr, test_decoder>("Lorus^D#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(8 == srcstr.length());
        REQUIRE(6 == test_decoder::decoded_size(src, srcstr.length()));
        // INPLACE
        dc_size = test_decoder::decode(buf, bsize, src, srcstr.length());
        REQUIRE(dc_size == 6);
        std::string dest = recode<test_decoder, slip_decoder_hr>(buf, dc_size);
        REQUIRE(dest == "Lorus#");
    }

    WHEN("consecutive specials at end") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<slip_decoder_hr, test_decoder>("Lorus^[^D^D#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(12 == srcstr.length());
        REQUIRE(8 == test_decoder::decoded_size(src, srcstr.length()));
        REQUIRE(8 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus^##" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
    }
}

TEST_CASE("slip_decoder_hr exact buffer", "[slip_decoder_hr-02]") {
    size_t dc_size;
    std::string srcstr;
    const char* src;
    const int MAXBUF = 30;

    bool INPLACE = GENERATE(false, true);

    WHEN("single end at input") {
        const size_t bsize = 1;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<slip_decoder_hr, test_decoder>("#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(1 == srcstr.length());
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE("" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("no specials") {
        const size_t bsize = INPLACE ? 6 : 5;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<slip_decoder_hr, test_decoder>("Lorus#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(6 == srcstr.length());
        REQUIRE(5 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("consecutive specials") {
        const size_t bsize = INPLACE ? 10 : 7;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<slip_decoder_hr, test_decoder>("Lo^[^Drus#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(10 == srcstr.length());
        REQUIRE(7 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lo^#rus" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("ESC at end") {
        const size_t bsize = INPLACE ? 8 : 6;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<slip_decoder_hr, test_decoder>("Lorus^[#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(8 == srcstr.length());
        REQUIRE(6 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus^" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("END at end") {
        const size_t bsize = INPLACE ? 8 : 6;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<slip_decoder_hr, test_decoder>("Lorus^D#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(8 == srcstr.length());
        REQUIRE(6 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus#" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("consecutive specials at end") {
        const size_t bsize = INPLACE ? 12 : 8;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<slip_decoder_hr, test_decoder>("Lorus^[^D^D#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(12 == srcstr.length());
        REQUIRE(8 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus^##" == recode<test_decoder, slip_decoder_hr>(buf, dc_size));
        REQUIRE('!' == buf[bsize]);
    }
}

TEST_CASE("slip_decoder_hr buffer overrun", "[slip_decoder_hr-02]") {
    size_t dc_size;
    std::string srcstr;
    const char* src;
    const int MAXBUF = 30;

    // ONLY test out-of-place decoding. In-place decoding always shrinks the message

    WHEN("no specials") {
        const size_t bsize = 4;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<slip_decoder_hr, test_decoder>("Lorus#");
        src    = srcstr.c_str();
        REQUIRE(6 == srcstr.length());
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("consecutive specials") {
        const size_t bsize = 6;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<slip_decoder_hr, test_decoder>("Lo^[^Drus#");
        src    = srcstr.c_str();
        REQUIRE(10 == srcstr.length());
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("ESC at end") {
        const size_t bsize = 5;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<slip_decoder_hr, test_decoder>("Lorus^[#");
        src    = srcstr.c_str();
        REQUIRE(8 == srcstr.length());
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("END at end") {
        const size_t bsize = 5;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<slip_decoder_hr, test_decoder>("Lorus^D#");
        src    = srcstr.c_str();
        REQUIRE(8 == srcstr.length());
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("consecutive specials at end") {
        const size_t bsize = 7;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<slip_decoder_hr, test_decoder>("Lorus^[^D^D#");
        src    = srcstr.c_str();
        REQUIRE(12 == srcstr.length());
        REQUIRE(0 == (dc_size = test_decoder::decode(buf, bsize, src, srcstr.length())));
        REQUIRE('!' == buf[bsize]);
    }
}

TEST_CASE("slip_decoder_hr bad inputs", "[slip_decoder_hr-04]") {
    const size_t bsize = 20;
    char buf[bsize];
    size_t dc_size;
    std::string srcstr = recode<slip_decoder_hr, test_decoder>("Lorus");

    bool INPLACE = GENERATE(false, true);

    WHEN("NULL buffer") {
        dc_size = test_decoder::decode(NULL, bsize, srcstr.c_str(), srcstr.length());
        REQUIRE(dc_size == 0);
    }
    WHEN("zero buffer size") {
        dc_size = test_decoder::decode(buf, 0, srcstr.c_str(), srcstr.length());
        REQUIRE(dc_size == 0);
    }
    WHEN("NULL input") {
        dc_size = test_decoder::decode(buf, bsize, NULL, srcstr.length());
        REQUIRE(dc_size == 0);
    }
}
