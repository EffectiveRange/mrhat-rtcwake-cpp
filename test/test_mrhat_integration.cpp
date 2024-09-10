#include <catch2/catch.hpp>

#include <httplib.h>

#include <atomic>
#include <chrono>
#include <future>

#include <mrhat_integration.hpp>

namespace chr = std::chrono;
struct MockServer {
  std::unique_ptr<httplib::Server> svr;
  std::future<void> ft;
  int port{};
  std::atomic<int> reg_val{-1};
  std::atomic<bool> error{};

  void wait() {
    svr->stop();
    ft.wait();
  }

  ~MockServer() { wait(); }
};

std::unique_ptr<MockServer> get_mock_server(bool return_error = false) {
  auto svr = std::make_unique<httplib::Server>();
  if (!svr->is_valid()) {
    throw std::runtime_error("Failed to set up mock server");
  }
  auto mock = std::make_unique<MockServer>(std::move(svr));
  mock->svr->Get("/api/register/8/0/1",
                 [mck_ = mock.get(), return_error](const httplib::Request &req,
                                                   httplib::Response &res) {
                   if (return_error) {
                     res.status = 403;
                   } else {
                     mck_->reg_val = 1;
                   }
                 });
  mock->svr->Get("/api/register/8/0/0",
                 [mck_ = mock.get(), return_error](const httplib::Request &req,
                                                   httplib::Response &res) {
                   if (return_error) {
                     res.status = 403;
                   } else {
                     mck_->reg_val = 0;
                   }
                 });
  mock->svr->set_error_handler(
      [mck_ = mock.get()](const httplib::Request &req, httplib::Response &res) {
        mck_->error = true;
      });

  mock->port = mock->svr->bind_to_any_port("localhost");
  mock->ft = std::async(std::launch::async, [svr_ = mock->svr.get()]() {
    int a = 0;
    svr_->listen_after_bind();
  });

  return std::move(mock);
}

TEST_CASE("mrhat integration for rst action", "[mrhat-integration]") {

  auto mock = get_mock_server();
  MrHatIntegration mrhat(mock->port);
  const auto result = mrhat.signal_reset_on_halt();
  mock->wait();
  REQUIRE(result);
  REQUIRE(mock->reg_val == 1);
  REQUIRE(mock->error == false);
}

TEST_CASE("mrhat integration for rst action cancel", "[mrhat-integration]") {

  auto mock = get_mock_server();
  MrHatIntegration mrhat(mock->port);
  const auto result = mrhat.clear_reset_on_halt();
  mock->wait();
  REQUIRE(result);
  REQUIRE(mock->reg_val == 0);
  REQUIRE(mock->error == false);
}

TEST_CASE("when server not running", "[mrhat-integration]") {
  MrHatIntegration mrhat(666);
  const auto result = mrhat.signal_reset_on_halt();
  REQUIRE_FALSE(result);
}

TEST_CASE("when error is at server side", "[mrhat-integration]") {

  auto mock = get_mock_server(true);
  MrHatIntegration mrhat(mock->port);
  const auto result = mrhat.signal_reset_on_halt();
  mock->wait();
  REQUIRE_FALSE(result);
  REQUIRE(mock->reg_val == -1);
  REQUIRE(mock->error == true);
}