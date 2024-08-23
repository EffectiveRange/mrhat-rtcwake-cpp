# mrhat-rtcwake-cpp

A drop in replacement for `rtcwake` utility to be used on the [Effective Range MrHat Raspberry Pi extension Hat](https://effective-range.com/hardware/mrhat/), featuring an Epson RX8130CE RTC module. Originally the tool was written in python, and this replaces the `python3-mrhat-rtcwake` package.

## Help

```bash
Usage: mrhat-rtcwake [--help] [--version] [--verbose]... [--adjfile VAR] [--device VAR] [--list-modes] [--mode VAR] [--force] [[--date VAR]|[--seconds VAR]|[--time VAR]]

Optional arguments:
  -h, --help     shows help message and exits 
  -v, --version  prints version information and exits 
  -v, --verbose  print more information about the operations [may be repeated]
  -A, --adjfile  Specify an alternative path to the adjust file. [nargs=0..1] [default: "/etc/adjtime"]
  -d, --device   Use the specified device instead of rtc0 as realtime clock. [nargs=0..1] [default: "rtc0"]
  --list-modes   List available --mode option arguments. 
  --mode         Go into the given standby state. [nargs=0..1] [default: "standby"]
  -f, --force    use --force flag when entering the specified mode 
  --date         Set the wakeup time to the value of the timestamp. 
  -s, --seconds  Set the wakeup time to seconds in the future from now. 
  -t, --time     Set the wakeup time to the absolute time time_t. 
```

## Examples
```bash
sudo rtcwake --date +1h  # go to sleep for 1 hour
sudo rtcwake --date "2024-08-12 8:00"  # go to sleep  until the specified date and time
sudo rtcwake --seconds 210  # go to sleep for 210 seconds (truncated to minute boundary)
sudo mrhat-rtcwake -t 1722903023 # go to sleep untile the specified time using seconds since epoch
```
All arguments are forwarded to the underlying `rtcwake` utility, for detailed time specification see [man entry for rtcwake](https://man7.org/linux/man-pages/man8/rtcwake.8.html)


## Operation

The EPSON RX8130CE RTC supports alarm interrupts on a minute granularity, and also the fact that shutdown and boot up are non-zero time operations we decided to use the RTC's periodic wakeup timer functionality so that in case a very near timepoint is specified, then there is zero chance missing the wakeup interrup.

If a valid time-point is specified, then the RTC alarm is armed the program uses the driver's ioctl API for setting the wakeup timer. Based on the mode specified the program then halts the system using the `sytemctl` utility on the normal Raspbian OS iamge. There's an extreme low power (XLP) PIC-18-Q20 family MCU onboard, that reacts to the RTC interrupt with our [default Firmware](https://github.com/EffectiveRange/fw-mrhat), and executes the wake-from-halt procedure - which is pulling the SCL line low - that in turn boots up the Raspberry Pi.

