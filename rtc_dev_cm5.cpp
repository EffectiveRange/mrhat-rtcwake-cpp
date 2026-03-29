#include "irtc.hpp"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <system_error>
#include <unistd.h>
#include <utility>

#include <fmt/format.h>

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
    struct rtc_wkalrm alarm{};
    alarm.time = time;
    alarm.enabled = 1;
    if (ioctl(m_fd, RTC_WKALM_SET, &alarm) != 0) {
      throw std::system_error(errno, std::generic_category(),
                              static_cast<std::string>("RTC_WKALM_SET ioctl"));
    }
  }
  void clear_wakeup() override {
    rtc_wkalrm alarm{};
    if (ioctl(m_fd, RTC_WKALM_RD, &alarm) != 0) {
      throw std::system_error(errno, std::generic_category(),
                              static_cast<std::string>("RTC_WKALM_RD ioctl"));
    }
    alarm.enabled = 0;
    if (ioctl(m_fd, RTC_WKALM_SET, &alarm) != 0) {
      throw std::system_error(
          errno, std::generic_category(),
          static_cast<std::string>("RTC_WKALM_SET clear ioctl"));
    }
  }

  rtc_wkalrm get_wakeup() const override {
    rtc_wkalrm rtc_tm{};
    if (ioctl(m_fd, RTC_WKALM_RD, &rtc_tm)) {
      throw std::system_error(errno, std::generic_category(),
                              static_cast<std::string>("RTC_WKALM_RD ioctl"));
    }
    return rtc_tm;
  }

  Clock type() const noexcept override {
    return m_is_utc ? IRTC::Clock::UTC : IRTC::Clock::LOCAL;
  }

  bool notify_listener(IntegrationInfo const &) const noexcept final {
    // nothing to do here;
    return true;
  }
  bool unnotify_listener(IntegrationInfo const &) const noexcept final {
    // no thing to do here;
    return true;
  }

private:
  int m_fd = -1;
  bool m_is_utc = true;
  std::string m_name;
};

std::unique_ptr<IRTC> IRTC::get(std::string_view name, std::string_view adj) {
  return std::make_unique<RTC>(name, parse_adjfile(adj) == Clock::UTC);
};

std::unique_ptr<MockRTC> get(std::string_view name, std::string_view adj) {
  throw std::runtime_error("Not Implemented");
}