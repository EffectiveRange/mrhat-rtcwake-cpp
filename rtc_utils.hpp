#pragma once

#include <iterator>
#include <linux/rtc.h>

#include <charconv>
#include <chrono>
#include <ctime>
#include <initializer_list>
#include <regex>
#include <stdexcept>
#include <system_error>
#include <variant>

#include <date/date.h>
#include <date/tz.h>
#include <fmt/format.h>

#include <irtc.hpp>

template <typename T, std::input_iterator It, std::sentinel_for<It> Sen>
inline auto parse_chars(It first, Sen last) {
  T val{};
  if (const auto [ptr, ec] = std::from_chars(first, last, val);
      ec != std::errc()) {
    throw std::system_error(std::make_error_code(ec));
  }
  return val;
}

inline std::chrono::system_clock::time_point rtc_to_sys(rtc_time const &tm,
                                                        IRTC const &rtc) {

  struct tm time {};
  std::memcpy(&time, &tm, std::min(sizeof(tm), sizeof(time)));
  const auto ts =
      rtc.type() == IRTC::Clock::LOCAL ? mktime(&time) : timegm(&time);
  return std::chrono::system_clock::from_time_t(ts);
}

inline rtc_time sys_to_rtc(std::chrono::system_clock::time_point tp,
                           IRTC const &rtc) {
  struct tm time {};
  rtc_time rtc_tm{};
  const auto timep = std::chrono::system_clock::to_time_t(tp);
  auto func = rtc.type() == IRTC::Clock::UTC ? gmtime_r : localtime_r;
  if (func(&timep, &time) == nullptr) {
    throw std::system_error(errno, std::generic_category());
  }
  std::memcpy(&rtc_tm, &time, std::min(sizeof(rtc_tm), sizeof(time)));
  return rtc_tm;
}

template <typename TZPtr>
inline auto rtc_to_zoned(rtc_time const &tm, IRTC const &rtc, TZPtr zone) {

  auto ztime = date::make_zoned(zone, rtc_to_sys(tm, rtc));

  return ztime;
}

inline auto rtc_to_zoned(rtc_time const &tm, IRTC const &rtc) {
  return rtc_to_zoned(tm, rtc, date::current_zone());
}

using sys_duration = std::chrono::system_clock::duration;
struct Tomorrow {};

using parsed_time =
    std::variant<sys_duration, date::zoned_time<sys_duration>, Tomorrow>;

inline sys_duration parse_relative_time(std::string_view date_in) {
  constexpr auto &rel_time_re_str =
      R"-(\+(\d+)(?:(s|sec|second|seconds)|(m|min|minute|minutes)|(h|hour|hours)|(d|day|days)|(w|week|weeks)|(month|months)|(y|year|years))\b)-";
  static const std::regex rel_time_re(rel_time_re_str);
  std::cmatch match;
  if (!std::regex_match(date_in.begin(), date_in.end(), match, rel_time_re)) {
    throw std::runtime_error(
        fmt::format("invalid relative date spec:{}", date_in));
  }
  std::string_view val(match[1].first, match[1].second);
  const auto rel_val = parse_chars<unsigned long>(val.begin(), val.end());
  if (match[2].matched) {
    return sys_duration{std::chrono::seconds{rel_val}};
  }
  if (match[3].matched) {
    return sys_duration{std::chrono::minutes{rel_val}};
  }
  if (match[4].matched) {
    return sys_duration{std::chrono::hours{rel_val}};
  }
  if (match[5].matched) {
    return sys_duration{std::chrono::days{rel_val}};
  }
  if (match[6].matched) {
    return sys_duration{std::chrono::weeks{rel_val}};
  }
  if (match[7].matched) {
    return sys_duration{std::chrono::months{rel_val}};
  }
  if (match[8].matched) {
    return sys_duration{std::chrono::years{rel_val}};
  }
  throw std::logic_error{"shouldn't reach this line"};
}

inline std::tm get_tm_now() {
  struct tm tmnow {};
  const auto tnow = time(nullptr);
  if (localtime_r(&tnow, &tmnow) == nullptr) {
    throw std::runtime_error("failed to get current time");
  }
  return tmnow;
}

inline date::zoned_time<sys_duration>
parse_time_abs(std::string_view date_in, std::tm tm_now = get_tm_now()) {
  using namespace std::string_literals;
  static const auto specstrs = {"%Y%m%d%H%M%S"s,   "%Y-%m-%d %H:%M:%S"s,
                                "%Y-%m-%d %H:%M"s, "%Y-%m-%d"s,
                                "%H:%M:%S"s,       "%H:%M"s};
  const std::string datestr(date_in);
  struct tm t {};
  for (auto spec : specstrs) {
    if (const auto endp = strptime(datestr.c_str(), spec.c_str(), &t);
        endp == datestr.c_str() + datestr.size()) {
      t.tm_isdst = -1;
      if (t.tm_year == 0 && t.tm_mon == 0 && t.tm_mday == 0) {
        t.tm_year = tm_now.tm_year;
        t.tm_mon = tm_now.tm_mon;
        t.tm_mday = tm_now.tm_mday;
      }
      auto timeval = std::mktime(&t);
      if (timeval < 0) {
        throw std::runtime_error(
            fmt::format("failed to do mktime on {}", date_in));
      }
      const auto systime = std::chrono::system_clock::from_time_t(timeval);
      return date::make_zoned(date::current_zone(), systime);
    }
    t = {};
  }
  throw std::runtime_error{
      fmt::format("unrecognized date specifier:{}", date_in)};
}

inline parsed_time parse_time(std::string_view date_in) {
  using namespace std::string_view_literals;
  if (date_in == "tomorrow"sv) {
    return Tomorrow{};
  }
  if (date_in.starts_with("+")) {
    return parse_relative_time(date_in);
  }
  return parse_time_abs(date_in);
}

inline rtc_time resolve_parsed_time(parsed_time const &tm, IRTC const &_rtc) {
  struct {
    IRTC const &rtc;
    rtc_time operator()(sys_duration const &d) const {
      const auto curr_time = rtc_to_sys(rtc.get_time(), rtc);
      const auto wakeup = curr_time + d;
      return sys_to_rtc(wakeup, rtc);
    }
    rtc_time operator()(date::zoned_time<sys_duration> const &dt) const {
      return sys_to_rtc(dt.get_sys_time(), rtc);
    }
    rtc_time operator()(Tomorrow const &) const {
      using namespace date;
      const auto local = rtc_to_zoned(rtc.get_time(), rtc).get_local_time();
      const auto tomorrow = local + days{1};
      const auto midnight = floor<days>(tomorrow);
      return sys_to_rtc(make_zoned(current_zone(), midnight).get_sys_time(),
                        rtc);
    }
  } resolver{_rtc};
  return std::visit(resolver, tm);
}
