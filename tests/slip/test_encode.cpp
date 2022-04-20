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
using test_encoder = encoder_hr;

TEST_CASE("encode_hr large buffer", "[encoder_hr-01]") {
    size_t ec_size;
    std::string srcstr;
    const char* src;

    bool INPLACE = GENERATE(false, true);

    WHEN("empty input") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<encoder_hr, test_encoder>("");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(0 == srcstr.length());
        REQUIRE(1 == test_encoder::encoded_size(src, srcstr.length()));
        REQUIRE(1 == (ec_size = test_encoder::encode(buf, bsize, "", 0)));
        REQUIRE("#" == recode<test_encoder, encoder_hr>(buf, ec_size));
    }

    WHEN("no specials") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<encoder_hr, test_encoder>("Lorus");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(5 == srcstr.length());
        REQUIRE(6 == test_encoder::encoded_size(src, srcstr.length()));
        REQUIRE(6 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus#" == recode<test_encoder, encoder_hr>(buf, ec_size));
    }

    WHEN("consecutive specials") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<encoder_hr, test_encoder>("Lo^#rus");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(7 == srcstr.length());
        REQUIRE(10 == test_encoder::encoded_size(src, srcstr.length()));
        REQUIRE(10 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lo^[^Drus#" == recode<test_encoder, encoder_hr>(buf, ec_size));
    }

    WHEN("ESC at end") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<encoder_hr, test_encoder>("Lorus^");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(6 == srcstr.length());
        REQUIRE(8 == test_encoder::encoded_size(src, srcstr.length()));
        REQUIRE(8 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus^[#" == recode<test_encoder, encoder_hr>(buf, ec_size));
    }

    WHEN("END at end") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<encoder_hr, test_encoder>("Lorus#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(6 == srcstr.length());
        REQUIRE(8 == test_encoder::encoded_size(src, srcstr.length()));
        REQUIRE(8 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus^D#" == recode<test_encoder, encoder_hr>(buf, ec_size));
    }

    WHEN("consecutive specials at end") {
        const size_t bsize = 20;
        char buf[bsize];
        srcstr = recode<encoder_hr, test_encoder>("Lorus^##");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(8 == srcstr.length());
        REQUIRE(12 == test_encoder::encoded_size(src, srcstr.length()));
        REQUIRE(12 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus^[^D^D#" == recode<test_encoder, encoder_hr>(buf, ec_size));
    }
}

TEST_CASE("encode_hr exact buffer", "[encoder_hr-02]") {
    size_t ec_size;
    std::string srcstr;
    const char* src;
    const int MAXBUF = 30;

    bool INPLACE = GENERATE(false, true);

    WHEN("empty input") {
        const size_t bsize = 1;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<encoder_hr, test_encoder>("");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(0 == srcstr.length());
        REQUIRE(1 == (ec_size = test_encoder::encode(buf, bsize, "", 0)));
        REQUIRE("#" == recode<test_encoder, encoder_hr>(buf, ec_size));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("no specials") {
        const size_t bsize = 6;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<encoder_hr, test_encoder>("Lorus");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(5 == srcstr.length());
        REQUIRE(6 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus#" == recode<test_encoder, encoder_hr>(buf, ec_size));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("consecutive specials") {
        const size_t bsize = 10;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<encoder_hr, test_encoder>("Lo^#rus");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(7 == srcstr.length());
        REQUIRE(10 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lo^[^Drus#" == recode<test_encoder, encoder_hr>(buf, ec_size));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("ESC at end") {
        const size_t bsize = 8;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<encoder_hr, test_encoder>("Lorus^");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(6 == srcstr.length());
        REQUIRE(8 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus^[#" == recode<test_encoder, encoder_hr>(buf, ec_size));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("END at end") {
        const size_t bsize = 8;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<encoder_hr, test_encoder>("Lorus#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(6 == srcstr.length());
        REQUIRE(8 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus^D#" == recode<test_encoder, encoder_hr>(buf, ec_size));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("consecutive specials at end") {
        const size_t bsize = 12;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<encoder_hr, test_encoder>("Lorus^##");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(8 == srcstr.length());
        REQUIRE(12 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE("Lorus^[^D^D#" == recode<test_encoder, encoder_hr>(buf, ec_size));
        REQUIRE('!' == buf[bsize]);
    }
}

TEST_CASE("encode_hr buffer overrun", "[encoder_hr-03]") {
    size_t ec_size;
    std::string srcstr;
    const char* src;
    const int MAXBUF = 30;

    bool INPLACE = GENERATE(false, true);

    WHEN("no specials") {
        const size_t bsize = 5;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<encoder_hr, test_encoder>("Lorus");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(0 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("consecutive specials") {
        const size_t bsize = 9;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<encoder_hr, test_encoder>("Lo^#rus");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(0 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("ESC at end") {
        const size_t bsize = 7;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<encoder_hr, test_encoder>("Lorus^");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(0 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("END at end") {
        const size_t bsize = 7;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<encoder_hr, test_encoder>("Lorus#");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(0 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE('!' == buf[bsize]);
    }

    WHEN("consecutive specials at end") {
        const size_t bsize = 11;
        char buf[MAXBUF];
        memset(buf, '!', MAXBUF);
        srcstr = recode<encoder_hr, test_encoder>("Lorus^##");
        src    = (INPLACE) ? (const char*)memcpy(buf, srcstr.c_str(), srcstr.length()) : srcstr.c_str();
        REQUIRE(0 == (ec_size = test_encoder::encode(buf, bsize, src, srcstr.length())));
        REQUIRE('!' == buf[bsize]);
    }
}

TEST_CASE("encode_hr bad inputs", "[encoder_hr-04]") {
    const size_t bsize = 20;
    char buf[bsize];
    size_t ec_size;
    std::string srcstr = recode<decoder_hr, test_encoder>("Lorus");
    const char* src    = srcstr.c_str();

    WHEN("NULL buffer") {
        REQUIRE(0 == (ec_size = test_encoder::encode(NULL, bsize, src, srcstr.length())));
    }

    WHEN("zero buffer size") {
        REQUIRE(0 == (ec_size = test_encoder::encode(buf, 0, src, srcstr.length())));
    }

    WHEN("NULL input") {
        REQUIRE(0 == (ec_size = test_encoder::encode(buf, bsize, NULL, srcstr.length())));
    }
}
