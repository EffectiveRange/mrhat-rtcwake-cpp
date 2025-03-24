#include <httplib.h>

#include <fmt/format.h>

#include "mrhat_integration.hpp"

#include <iostream>

bool MrHatIntegration::signal_reset_on_halt() { return api_impl(true); }

bool MrHatIntegration::clear_reset_on_halt() { return api_impl(false); }

bool MrHatIntegration::api_impl(bool set) {
  httplib::Client cli("localhost", port);
  const auto endpoint = fmt::format("/api/register/{}/{}/{}", rst_action_reg,
                                    rst_action_bit, set ? 1 : 0);
  if (auto res = cli.Post(endpoint);
      res && (res->status >= 200 && res->status < 300)) {
    return true;
  } else {
    std::cerr << fmt::format("error sending reset on halt action to "
                             "http://localhost:{}{} status:{} code:{}\n",
                             port, endpoint, res ? res->status : -1,
                             static_cast<int>(res.error()));
    return false;
  }
}
