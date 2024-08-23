#include "irtc.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <system_error>
#include <unistd.h>
#include <utility>

#include <fmt/format.h>

#include <rtc-rx8130.h>

class RTC : public IRTC {
public:
  explicit RTC(std::string_view name, bool m_is_utc = true)
      : m_name{name}, m_is_utc{m_is_utc} {
    const auto &dev = fmt::format("/dev/{}", name);
    if (m_fd = open(dev.c_str(), O_RDWR); m_fd < 0) {
      throw std::system_error(errno, std::generic_category(),
                              static_cast<std::string>(name));
    }
  }
  std::string_view name() const noexcept override { return m_name; }

  RTC(const RTC &) = delete;
  RTC &operator=(const RTC &) = delete;
  RTC(RTC &&other) : m_fd{std::exchange(other.m_fd, -1)} {}
  RTC &operator=(RTC &&other) {
    if (this != &other) {
      if (m_fd >= 0)
        close(m_fd);
      m_fd = std::exchange(other.m_fd, -1);
    }
    return *this;
  }
  ~RTC() override {
    if (m_fd >= 0)
      close(m_fd);
  }
  rtc_time get_time() const override {
    rtc_time rtc_tm{};
    auto retval = ioctl(m_fd, RTC_RD_TIME, &rtc_tm);
    if (retval == -1) {
      throw std::system_error(errno, std::generic_category(),
                              "RTC_RD_TIME ioctl");
    }
    return rtc_tm;
  }
  void set_wakeup(rtc_time const &time) override {
    if (ioctl(m_fd, SE_RTC_WKTIMER_SET, &time) != 0) {
      throw std::system_error(
          errno, std::generic_category(),
          static_cast<std::string>("SE_RTC_WKTIMER_SET ioctl"));
    }
  }
  void clear_wakeup() override {
    if (ioctl(m_fd, SE_RTC_WKTIMER_SET, nullptr) != 0) {
      throw std::system_error(
          errno, std::generic_category(),
          static_cast<std::string>("SE_RTC_WKTIMER_SET clear ioctl"));
    }
  }

  rtc_wkalrm get_wakeup() const override {
    rtc_wkalrm rtc_tm{};
    if (ioctl(m_fd, SE_RTC_WKTIMER_GET, &rtc_tm)) {
      throw std::system_error(
          errno, std::generic_category(),
          static_cast<std::string>("SE_RTC_WKTIMER_GET ioctl"));
    }
    return rtc_tm;
  }

  Clock type() const noexcept override {
    return m_is_utc ? IRTC::Clock::UTC : IRTC::Clock::LOCAL;
  }

private:
  int m_fd = -1;
  bool m_is_utc = true;
  std::string m_name;
};

std::unique_ptr<IRTC> IRTC::get(std::string_view name, std::string_view adj) {
  return std::make_unique<RTC>(name);
};

std::unique_ptr<MockRTC> get(std::string_view name, std::string_view adj) {
  throw std::runtime_error("Not Implemented");
}