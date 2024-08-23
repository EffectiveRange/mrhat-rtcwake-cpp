#include <catch2/catch.hpp>

#include <irtc.hpp>

TEST_CASE("adjustment file parsing", "[tools]") {

  SECTION("rtc is in UTC") {
    const auto &adjfile = "0.000000 1723331760 0.000000\n"
                          "1723331760\n"
                          "UTC\n";
    REQUIRE(IRTC::parse_adjfile(adjfile) == IRTC::Clock::UTC);
  }
  SECTION("rtc is in LOCAL") {
    const auto &adjfile = "0.000000 1723331760 0.000000\n"
                          "1723331760\n"
                          "LOCAL\n";
    REQUIRE(IRTC::parse_adjfile(adjfile) == IRTC::Clock::LOCAL);
  }

  SECTION("rtc TZ is invalid") {
    const auto &adjfile = "0.000000 1723331760 0.000000\n"
                          "1723331760\n"
                          "ERROR LITERAL\n";
    REQUIRE_THROWS_AS(IRTC::parse_adjfile(adjfile), std::runtime_error);
  }
  SECTION("adjfile is malformed") {
    const auto &adjfile = "0.000000 1723331760 0.000000\n"
                          "1723331760\n";
    REQUIRE_THROWS_AS(IRTC::parse_adjfile(adjfile), std::runtime_error);
  }
}