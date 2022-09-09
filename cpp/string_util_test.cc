#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "string_util.h"

TEST_CASE("split") {
    string a = "  aa bb cc dd  ";
    auto res = split(a);
    REQUIRE(res[0] == "aa");
    REQUIRE(res[1] == "bb");
    REQUIRE(res[2] == "cc");
    REQUIRE(res[3] == "dd");
}