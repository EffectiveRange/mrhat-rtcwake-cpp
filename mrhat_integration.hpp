#pragma once

#include <cstdint>

struct MrHatIntegration {

  MrHatIntegration() = default;
  explicit MrHatIntegration(uint16_t p) : port{p} {}
  MrHatIntegration(uint16_t p, unsigned rst_act_reg, unsigned rst_action_b)
      : port{p}, rst_action_reg{rst_act_reg}, rst_action_bit{rst_action_b} {}
  bool signal_reset_on_halt();
  bool clear_reset_on_halt();

private:
  bool api_impl(bool set);
  uint16_t port = 9000;
  unsigned rst_action_reg = 8;
  unsigned rst_action_bit = 0;
};
