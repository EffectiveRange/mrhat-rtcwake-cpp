#include "irtc.hpp"

#include <numeric>
#include <range/v3/algorithm.hpp>
#include <range/v3/view/common.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/split.hpp>
#include <range/v3/view/take.hpp>
#include <ranges>

#include <numeric>

#include <fmt/format.h>

#include <stdexcept>

namespace rg = ranges;
namespace rgv = ranges::views;

using std::literals::operator""sv;

auto IRTC::parse_adjfile(std::string_view adj) -> Clock {
  rg::split_view view{adj, "\n"sv};
  auto tz = view | rgv::drop(2) | rgv::take(1);
  IRTC::Clock res = IRTC::Clock::INVALID;
  constexpr auto UTC_literal = "UTC"sv;
  constexpr auto LOCAL_literal = "LOCAL"sv;

  for (const auto v : tz) {
    if (rg::equal(rg::begin(v), rg::end(v), UTC_literal.begin(),
                  UTC_literal.end())) {
      res = IRTC::Clock::UTC;
    } else if (rg::equal(rg::begin(v), rg::end(v), LOCAL_literal.begin(),
                         LOCAL_literal.end())) {
      res = IRTC::Clock::LOCAL;
    };
  }

  if (res == IRTC::Clock::INVALID)
    throw std::runtime_error("malformed adjustment file");
  return res;
}
