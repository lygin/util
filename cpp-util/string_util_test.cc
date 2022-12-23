#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "string_util.h"
#include "timer.h"

TEST_CASE("split") {
    
    string a = "  aa bb cc dd  ";
    Timer timer;
    auto res = split(a);
    printf("total time: %lfus\n", timer.GetDurationUs());
    
    REQUIRE(res[0] == "aa");
    REQUIRE(res[1] == "bb");
    REQUIRE(res[2] == "cc");
    REQUIRE(res[3] == "dd");
}