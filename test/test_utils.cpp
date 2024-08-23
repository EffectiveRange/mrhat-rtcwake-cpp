#include "irtc.hpp"
#include <catch2/catch.hpp>

#include <rtc_utils.hpp>

#include <iostream>

auto get_tm_day(int year, int month, int day) {
  std::tm t{};
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  return t;
}

TEST_CASE("relative date parsing", "[utils]") {

  SECTION("seconds") {
    SECTION("when s") {
      const auto res = parse_time("+1s");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::seconds{1});
    }
    SECTION("when sec") {
      const auto res = parse_time("+2sec");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::seconds{2});
    }
    SECTION("when second") {
      const auto res = parse_time("+3second");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::seconds{3});
    }
    SECTION("when seconds") {
      const auto res = parse_time("+4seconds");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::seconds{4});
    }
    SECTION("when invalid") {
      REQUIRE_THROWS_AS(parse_time("+4secondsus"), std::runtime_error);
    }
  }
  SECTION("minutes") {
    SECTION("when m") {
      const auto res = parse_time("+1m");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::minutes{1});
    }
    SECTION("when min") {
      const auto res = parse_time("+2min");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::minutes{2});
    }
    SECTION("when minute") {
      const auto res = parse_time("+3minute");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::minutes{3});
    }
    SECTION("when minutes") {
      const auto res = parse_time("+4minutes");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::minutes{4});
    }
    SECTION("when invalid") {
      REQUIRE_THROWS_AS(parse_time("+4minutesus"), std::runtime_error);
    }
  }
  SECTION("hours") {
    SECTION("when h") {
      const auto res = parse_time("+1h");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::hours{1});
    }
    SECTION("when hour") {
      const auto res = parse_time("+3hour");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::hours{3});
    }
    SECTION("when hours") {
      const auto res = parse_time("+4hours");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::hours{4});
    }
    SECTION("when invalid") {
      REQUIRE_THROWS_AS(parse_time("+4hourssda"), std::runtime_error);
    }
  }
  SECTION("days") {
    SECTION("when d") {
      const auto res = parse_time("+1d");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::days{1});
    }
    SECTION("when day") {
      const auto res = parse_time("+3day");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::days{3});
    }
    SECTION("when days") {
      const auto res = parse_time("+4days");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::days{4});
    }
    SECTION("when invalid") {
      REQUIRE_THROWS_AS(parse_time("+4dayssadaw"), std::runtime_error);
    }
  }
  SECTION("weeks") {
    SECTION("when w") {
      const auto res = parse_time("+1w");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::weeks{1});
    }
    SECTION("when week") {
      const auto res = parse_time("+3week");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::weeks{3});
    }
    SECTION("when weeks") {
      const auto res = parse_time("+4weeks");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::weeks{4});
    }
    SECTION("when invalid") {
      REQUIRE_THROWS_AS(parse_time("+4weekssadaw"), std::runtime_error);
    }
  }
  SECTION("months") {
    SECTION("when month") {
      const auto res = parse_time("+3month");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::months{3});
    }
    SECTION("when months") {
      const auto res = parse_time("+4months");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::months{4});
    }
    SECTION("when invalid") {
      REQUIRE_THROWS_AS(parse_time("+4monthssadaw"), std::runtime_error);
    }
  }
  SECTION("years") {
    SECTION("when y") {
      const auto res = parse_time("+1y");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::years{1});
    }
    SECTION("when year") {
      const auto res = parse_time("+3year");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::years{3});
    }
    SECTION("when years") {
      const auto res = parse_time("+4years");
      REQUIRE(std::get<sys_duration>(res) == std::chrono::years{4});
    }
    SECTION("when invalid") {
      REQUIRE_THROWS_AS(parse_time("+4yearssadaw"), std::runtime_error);
    }
  }
  SECTION("invalid relative spec") {
    REQUIRE_THROWS_AS(parse_time("+3futtyfurutty"), std::runtime_error);
  }
}
TEST_CASE("tomorrow parsing", "[utils]") {
  const auto res = parse_time("tomorrow");
  REQUIRE_NOTHROW(std::get<Tomorrow>(res));
}
TEST_CASE("absolute date parsing", "[utils]") {
  SECTION("when long format") {
    using namespace date;
    using namespace date::literals;
    using namespace std::chrono_literals;
    const auto res = parse_time_abs("20240819015411");
    auto syst = res.get_local_time();
    auto const dp = date::floor<date::days>(syst);
    auto const hms = make_time(syst - dp);
    auto ymd = date::year_month_day(dp);
    REQUIRE(ymd.year() == 2024_y);
    REQUIRE(ymd.month() == month{8});
    REQUIRE(ymd.day() == 19_d);
    REQUIRE(hms.hours() == 1h);
    REQUIRE(hms.minutes() == 54min);
    REQUIRE(hms.seconds() == 11s);
  }
  SECTION("when long pretty format") {
    using namespace date;
    using namespace date::literals;
    using namespace std::chrono_literals;
    const auto res = parse_time_abs("2024-08-19 01:54:11");
    auto syst = res.get_local_time();
    auto const dp = date::floor<date::days>(syst);
    auto const hms = make_time(syst - dp);
    auto ymd = date::year_month_day(dp);
    REQUIRE(ymd.year() == 2024_y);
    REQUIRE(ymd.month() == month{8});
    REQUIRE(ymd.day() == 19_d);
    REQUIRE(hms.hours() == 1h);
    REQUIRE(hms.minutes() == 54min);
    REQUIRE(hms.seconds() == 11s);
  }
  SECTION("when long pretty format no seconds") {
    using namespace date;
    using namespace date::literals;
    using namespace std::chrono_literals;
    const auto res = parse_time_abs("2024-08-19 01:54");
    auto syst = res.get_local_time();
    auto const dp = date::floor<date::days>(syst);
    auto const hms = make_time(syst - dp);
    auto ymd = date::year_month_day(dp);
    REQUIRE(ymd.year() == 2024_y);
    REQUIRE(ymd.month() == month{8});
    REQUIRE(ymd.day() == 19_d);
    REQUIRE(hms.hours() == 1h);
    REQUIRE(hms.minutes() == 54min);
    REQUIRE(hms.seconds() == 0s);
  }
  SECTION("when long pretty format day only") {
    using namespace date;
    using namespace date::literals;
    using namespace std::chrono_literals;
    const auto res = parse_time_abs("2024-08-19");
    auto syst = res.get_local_time();
    auto const dp = date::floor<date::days>(syst);
    auto const hms = make_time(syst - dp);
    auto ymd = date::year_month_day(dp);
    REQUIRE(ymd.year() == 2024_y);
    REQUIRE(ymd.month() == month{8});
    REQUIRE(ymd.day() == 19_d);
    REQUIRE(hms.hours() == 0h);
    REQUIRE(hms.minutes() == 0min);
    REQUIRE(hms.seconds() == 0s);
  }
  SECTION("when pretty hms format ") {
    using namespace date;
    using namespace date::literals;
    using namespace std::chrono_literals;
    const auto tm_now = get_tm_day(2024, 8, 19);
    const auto res = parse_time_abs("01:54:11", tm_now);
    auto syst = res.get_local_time();
    auto const dp = date::floor<date::days>(syst);
    auto const hms = make_time(syst - dp);
    auto ymd = date::year_month_day(dp);
    REQUIRE(ymd.year() == 2024_y);
    REQUIRE(ymd.month() == month{8});
    REQUIRE(ymd.day() == 19_d);
    REQUIRE(hms.hours() == 1h);
    REQUIRE(hms.minutes() == 54min);
    REQUIRE(hms.seconds() == 11s);
  }
  SECTION("when pretty hms format no seconds") {
    using namespace date;
    using namespace date::literals;
    using namespace std::chrono_literals;
    const auto tm_now = get_tm_day(2024, 8, 19);
    const auto res = parse_time_abs("01:54", tm_now);
    auto syst = res.get_local_time();
    auto const dp = date::floor<date::days>(syst);
    auto const hms = make_time(syst - dp);
    auto ymd = date::year_month_day(dp);
    REQUIRE(ymd.year() == 2024_y);
    REQUIRE(ymd.month() == month{8});
    REQUIRE(ymd.day() == 19_d);
    REQUIRE(hms.hours() == 1h);
    REQUIRE(hms.minutes() == 54min);
    REQUIRE(hms.seconds() == 0s);
  }
}

auto get_mock_rtc(std::string_view adjtype = "UTC") {
  using namespace std::string_view_literals;
  auto adjfile = R"-(0.000000 1723331760 0.000000
1723331760
{})-"sv;
  return MockRTC::get("rtc0", fmt::format(adjfile, adjtype));
}

auto get_rtc_time(int year, int month, int day, int hour, int min, int sec,
                  bool dst = false) {
  rtc_time t{};
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = min;
  t.tm_sec = sec;
  t.tm_isdst = dst ? 1 : 0;
  auto ymd = date::year{year} / month / day;
  namespace ch = std::chrono;
  auto hms =
      date::hh_mm_ss(ch::hours{hour} + ch::minutes{min} + ch::seconds{sec});
  return t;
}

TEST_CASE("resolve parsed time", "[utils]") {
  using namespace date;
  using namespace date::literals;
  using namespace std::chrono_literals;
  namespace ch = std::chrono;

  SECTION("with duration seconds") {
    auto rtc = get_mock_rtc();
    rtc->set_time(get_rtc_time(2024, 8, 18, 21, 22, 32));
    SECTION("within minute") {
      const auto rtc_time = resolve_parsed_time(5s, *rtc);
      REQUIRE(rtc_time.tm_year == 124);
      REQUIRE(rtc_time.tm_mon == 7);
      REQUIRE(rtc_time.tm_mday == 18);
      REQUIRE(rtc_time.tm_hour == 21);
      REQUIRE(rtc_time.tm_min == 22);
      REQUIRE(rtc_time.tm_sec == 37);
    }
    SECTION("cross minute") {
      const auto rtc_time = resolve_parsed_time(28s, *rtc);
      REQUIRE(rtc_time.tm_year == 124);
      REQUIRE(rtc_time.tm_mon == 7);
      REQUIRE(rtc_time.tm_mday == 18);
      REQUIRE(rtc_time.tm_hour == 21);
      REQUIRE(rtc_time.tm_min == 23);
      REQUIRE(rtc_time.tm_sec == 0);
    }
    SECTION("cross hour") {
      const auto rtc_time = resolve_parsed_time(ch::seconds{38 * 60}, *rtc);
      REQUIRE(rtc_time.tm_year == 124);
      REQUIRE(rtc_time.tm_mon == 7);
      REQUIRE(rtc_time.tm_mday == 18);
      REQUIRE(rtc_time.tm_hour == 22);
      REQUIRE(rtc_time.tm_min == 0);
      REQUIRE(rtc_time.tm_sec == 32);
    }
    SECTION("cross day") {
      const auto rtc_time = resolve_parsed_time(ch::seconds{3 * 60 * 60}, *rtc);
      REQUIRE(rtc_time.tm_year == 124);
      REQUIRE(rtc_time.tm_mon == 7);
      REQUIRE(rtc_time.tm_mday == 19);
      REQUIRE(rtc_time.tm_hour == 0);
      REQUIRE(rtc_time.tm_min == 22);
      REQUIRE(rtc_time.tm_sec == 32);
    }
    SECTION("cross month") {
      const auto rtc_time =
          resolve_parsed_time(ch::seconds{14 * 24 * 60 * 60}, *rtc);
      REQUIRE(rtc_time.tm_year == 124);
      REQUIRE(rtc_time.tm_mon == 8);
      REQUIRE(rtc_time.tm_mday == 1);
      REQUIRE(rtc_time.tm_hour == 21);
      REQUIRE(rtc_time.tm_min == 22);
      REQUIRE(rtc_time.tm_sec == 32);
    }
    SECTION("cross year") {
      rtc->set_time(get_rtc_time(2024, 12, 31, 21, 22, 32));
      const auto rtc_time = resolve_parsed_time(ch::seconds{3 * 60 * 60}, *rtc);
      REQUIRE(rtc_time.tm_year == 125);
      REQUIRE(rtc_time.tm_mon == 0);
      REQUIRE(rtc_time.tm_mday == 1);
      REQUIRE(rtc_time.tm_hour == 0);
      REQUIRE(rtc_time.tm_min == 22);
      REQUIRE(rtc_time.tm_sec == 32);
    }
  }
  SECTION("with absolute local time") {
    auto rtc = get_mock_rtc();
    rtc->set_time(get_rtc_time(2024, 8, 18, 21, 22, 32));
  }
  SECTION("tomorrow") {
    auto rtc = get_mock_rtc();
    const auto curr_rtc_time = get_rtc_time(2024, 8, 18, 23, 22, 32);
    const auto zrtc = rtc_to_zoned(curr_rtc_time, *rtc);
    rtc->set_time(curr_rtc_time);
    const auto rtc_time = resolve_parsed_time(Tomorrow{}, *rtc);
    const auto rtc_local = zrtc.get_local_time();
    const auto today = date::floor<date::days>(rtc_local);
    const auto tomorrow = today + date::days{1};
    date::year_month_day ymd_tmrw(tomorrow);
    const auto exp_year = (static_cast<int>(ymd_tmrw.year()) - 1900);
    const auto exp_month = (static_cast<unsigned>(ymd_tmrw.month()) - 1);
    const auto exp_day = (static_cast<unsigned>(ymd_tmrw.day()));
    REQUIRE(rtc_time.tm_year == exp_year);
    REQUIRE(rtc_time.tm_mon == exp_month);
    REQUIRE(rtc_time.tm_mday == exp_day);
    REQUIRE(rtc_time.tm_hour == 0);
    REQUIRE(rtc_time.tm_min == 0);
    REQUIRE(rtc_time.tm_sec == 0);
  }

  // local.get_info().offset
}