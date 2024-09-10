#include <chrono>
#include <concepts>
#include <fcntl.h>
#include <iterator>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <system_error>
#include <unistd.h>

#include <argparse/argparse.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>

#include <irtc.hpp>
#include <mrhat_integration.hpp>
#include <rtc_utils.hpp>
namespace fs = std::filesystem;

enum class Verbosity { ERROR = 0, INFO = 1, DEBUG = 2, MAX = DEBUG };

template <std::integral T> Verbosity verbosity(T val) {
  return static_cast<Verbosity>(
      std::clamp(val, 0, static_cast<T>(Verbosity::MAX)));
}

struct AugmentedParser {
  argparse::ArgumentParser parser;
  int verbosity = 0;
};

std::unique_ptr<AugmentedParser> get_parser() {
  using namespace std::string_literals;
  std::unique_ptr<AugmentedParser> parser(new AugmentedParser{

      argparse::ArgumentParser{"mrhat-rtcwake", MRHATRTCWAKE_VER,
                               argparse::default_arguments::all},
      0});

  auto *program = &parser->parser;

  program->add_argument("-v", "--verbose")
      .action([verbose = std::addressof(parser->verbosity)](const auto &) {
        *verbose += 1;
      })
      .append()
      .nargs(0)
      .help("print more information about the operations")
      .default_value(false)
      .implicit_value(true);

  program->add_argument("-A", "--adjfile")
      .default_value("/etc/adjtime"s)
      .help("Specify an alternative path to the adjust file.");
  program->add_argument("-d", "--device")
      .help("Use the specified device instead of rtc0 as realtime clock.")
      .default_value("rtc0"s);
  program->add_argument("--list-modes")
      .help("List available --mode option arguments.")
      .flag();
  program->add_argument("--mode")
      .help("Go into the given standby state.")
      .choices("standby"s, "no"s, "disable"s, "show"s)
      .default_value("standby"s);
  program->add_argument("-f", "--force")
      .help("use --force flag when entering the specified mode")
      .flag();
  program->add_argument("--mrhat-daemon-port")
      .help("Listen port of mrhat-daemon to signal reset on halt at.")
      .default_value(9000);
  program->add_argument("--rst-action-register")
      .help("Reset action register on the MrHat device.")
      .default_value(8);
  program->add_argument("--rst-action-bit")
      .help(
          "Reset action bit in the reset action register on the MrHat device.")
      .default_value(0);
  auto &date_group = program->add_mutually_exclusive_group();
  date_group.add_argument("--date").help(
      "Set the wakeup time to the value of the timestamp.");
  date_group.add_argument("-s", "--seconds")
      .help("Set the wakeup time to seconds in the future from now.");
  date_group.add_argument("-t", "--time")
      .help("Set the wakeup time to the absolute time time_t.");
  return std::move(parser);
}

std::string read_adjfile(fs::path adjfile) {
  if (!fs::exists(adjfile) || !fs::is_regular_file(adjfile)) {
    throw std::runtime_error(fmt::format(
        "adjustment file {} non-existent or non-regular", adjfile.c_str()));
  }
  std::ifstream ifs(adjfile);
  if (!ifs) {
    throw std::runtime_error(
        fmt::format("failed to read adjustment file {}", adjfile.c_str()));
  }

  std::stringstream buff;
  buff << ifs.rdbuf();
  return buff.str();
}

std::optional<rtc_time> get_date_spec(argparse::ArgumentParser const &parser,
                                      IRTC const &rtc) {
  if (parser.is_used("--date")) {
    const auto d = parser.get<std::string>("--date");
    return resolve_parsed_time(parse_time(d), rtc);
  }
  if (parser.is_used("--seconds")) {
    const auto secs = parser.get<std::string>("--seconds");
    std::string_view s(secs);
    const auto val = parse_chars<unsigned long>(s.begin(), s.end());
    return resolve_parsed_time(std::chrono::seconds{val}, rtc);
  }
  if (parser.is_used("--time")) {
    const auto t = parser.get<std::string>("--time");
    std::string_view tsv(t);
    const auto val = parse_chars<std::time_t>(tsv.begin(), tsv.end());
    return sys_to_rtc(std::chrono::system_clock::from_time_t(val), rtc);
  }
  return {};
}

template <typename T>
concept String = std::convertible_to<T, std::string>;

template <String Prog, String... Args>
auto do_exec(Prog &&prog, Args &&...args) {
  std::string ps(std::forward<Prog>(prog));
  std::array as = {std::string(std::forward<Args>(args))...};
  char *execargs[sizeof...(Args) + 1] = {};
  namespace rg = std::ranges;
  namespace rgv = std::ranges::views;
  auto cstr = as | rgv::transform([](auto &s) { return s.data(); });
  auto cr = rg::copy(rg::begin(cstr), rg::end(cstr), rg::begin(execargs));
  *cr.out = nullptr;
  execv(ps.c_str(), execargs);
  return errno;
}

auto format_date(auto d) { return date::format("%a %d %b %X %Z %Y", d); }

int main(int argc, char *argv[]) try {
  using namespace std::literals;
  auto pparser = get_parser();
  auto &aug_parser = *pparser;
  auto &parser = aug_parser.parser;
  parser.parse_args(argc, argv);
  const auto verbose = verbosity(aug_parser.verbosity);

  if (parser["--list-modes"] == true) {
    std::cout << "standby no disable show'\n";
    return 0;
  }

  auto rtc = IRTC::get(parser.get<std::string>("--device"),
                       read_adjfile(parser.get<std::string>("--adjfile")));

  const auto datespec = get_date_spec(parser, *rtc);

  const auto rtctime = rtc->get_time();
  if (pparser->verbosity) {
    std::cout << "Current RTC time is(local):"
              << format_date(rtc_to_zoned(rtctime, *rtc)) << '\n';
  }
  const auto mode = parser.get<std::string>("--mode");
  if (mode == "show"s) {
    auto wktime = rtc->get_wakeup();
    if (wktime.enabled) {
      std::cout << "alarm: on " << format_date(rtc_to_zoned(wktime.time, *rtc))
                << '\n';
    } else {
      std::cout << "alarm: off\n";
    }
    return 0;
  } else if (mode == "disable"s) {
    rtc->clear_wakeup();
    return 0;
  } else if (datespec.has_value() && (mode == "no"s || mode == "standby"s)) {
    rtc->set_wakeup(*datespec);
    std::cout << fmt::format("mrhat-rtcwake: wakeup using /dev/{} at ",
                             rtc->name())
              << format_date(rtc_to_zoned(*datespec, *rtc)) << '\n';
    if (mode != "no") {
      MrHatIntegration mrhat(parser.get<int>("--mrhat-daemon-port"),
                             parser.get<int>("--rst-action-register"),
                             parser.get<int>("--rst-action-bit"));
      const auto rst = mrhat.signal_reset_on_halt();
      if (!rst) {
        std::cerr << "!!!WARNING: could not set reset on halt bit!\n";
      }
      const auto err = parser["--force"] == true
                           ? do_exec("/usr/sbin/poweroff", "--halt", "--force")
                           : do_exec("/usr/sbin/poweroff", "--halt");

      if (rst && !mrhat.clear_reset_on_halt()) {
        std::cerr
            << "!!!WARNING: reset on halt bit active, must clean manually!\n";
      }
      throw std::system_error(err, std::generic_category());
    }
    return 0;
  }
  throw std::runtime_error(
      "must provide wake time (see --seconds, --time and --date options)");

} catch (std::system_error const &e) {
  std::cerr << "mrhat-rtcwake: " << e.what() << '\n';
  return e.code().value();
} catch (std::exception const &e) {
  std::cerr << "mrhat-rtcwake: " << e.what() << '\n';
  return -1;
} catch (...) {
  std::cerr << "mrhat-rtcwake: " << "unknown exception occured...\n";
  return -1;
}