#pragma once

#include <linux/rtc.h>

#include <memory>
#include <string_view>

struct IRTC {
  enum class Clock { LOCAL, UTC, INVALID };

  virtual rtc_time get_time() const = 0;
  virtual void set_wakeup(rtc_time const &time) = 0;
  virtual rtc_wkalrm get_wakeup() const = 0;
  virtual void clear_wakeup() = 0;
  virtual Clock type() const noexcept = 0;
  virtual std::string_view name() const noexcept = 0;

  static std::unique_ptr<IRTC> get(std::string_view name,
                                   std::string_view adj = {});

  static Clock parse_adjfile(std::string_view adj);
  virtual ~IRTC() = default;
};

struct MockRTC : IRTC {

  virtual void set_time(rtc_time const &time) = 0;
  virtual void wakeup_occured() = 0;
  static std::unique_ptr<MockRTC> get(std::string_view name,
                                      std::string_view adj = {});
};