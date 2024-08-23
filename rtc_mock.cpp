#include "irtc.hpp"

#include <map>
#include <string>
#include <utility>

namespace {

struct MockRTCImpl : MockRTC {
  MockRTCImpl(std::string_view adj) : m_clock(IRTC::parse_adjfile(adj)) {}
  rtc_time get_time() const override { return m_tm; }
  void set_wakeup(rtc_time const &time) override {
    // TODO check time for past
    m_wakeup.time = time;
    m_wakeup.enabled = 1;
    m_wakeup.pending = 0;
  }
  rtc_wkalrm get_wakeup() const override { return m_wakeup; }
  void clear_wakeup() override {}
  Clock type() const noexcept override { return m_clock; }
  void set_time(rtc_time const &time) override { m_tm = time; }
  void wakeup_occured() override {
    if (!m_wakeup.enabled) {
      throw std::logic_error("rtc wakeup was not armed");
    }
    m_wakeup.pending = 1;
  }
  std::string_view name() const noexcept override { return "mock"; }

private:
  rtc_time m_tm{};
  rtc_wkalrm m_wakeup{};
  Clock m_clock{};
};

} // namespace

std::unique_ptr<MockRTC> MockRTC::get(std::string_view name,
                                      std::string_view adj) {
  return std::make_unique<MockRTCImpl>(adj);
}

std::unique_ptr<IRTC> IRTC::get(std::string_view name, std::string_view adj) {
  return std::make_unique<MockRTCImpl>(adj);
}