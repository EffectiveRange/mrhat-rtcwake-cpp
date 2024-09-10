cmake_minimum_required(VERSION 3.22)
# Pull in definitions from
# Effective Range CMake module like ER_DEPS, ER_PACK, etc.
include(ERBuild NO_POLICY_SCOPE)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



set(mrhat-rtcwake-ver 0.3.0)

project(mrhat-rtcwake VERSION ${mrhat-rtcwake-ver} DESCRIPTION "rtcwake compatible uitlity to use the RTC wakeup functionality on the MrHAT RPI extension hat")



# GSS_DEPS() should be called once per top level project, this
# will install the package dependencies listed in the deps.json file 
ER_DEPS()

find_package(fmt REQUIRED)

find_package(Catch2  REQUIRED)

# fetch latest argparse
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

set(BUILD_TZ_LIB ON)
set(USE_SYSTEM_TZ_DB ON)
FetchContent_Declare(
    date
    GIT_REPOSITORY https://github.com/HowardHinnant/date.git
)
FetchContent_MakeAvailable(date)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
)

FetchContent_MakeAvailable(httplib)


find_path(RTC_RX8130_INCLUDE_DIR rtc-rx8130.h HINTS /usr/include)

if(NOT RTC_RX8130_INCLUDE_DIR)
message(INFO ":rtc-rx8130.h not found, using mock implementation")
SET(RTC_SOURCE rtc_mock.cpp)
else()
message(INFO ":rtc-rx8130.h  found, using real device")
SET(RTC_SOURCE rtc_dev.cpp)
endif()


add_library(mrhat-rtcwake-lib STATIC ${RTC_SOURCE} rtc_tools.cpp mrhat_integration.cpp)
target_link_libraries(mrhat-rtcwake-lib PUBLIC date::date date::date-tz fmt::fmt httplib::httplib)
target_include_directories(mrhat-rtcwake-lib PUBLIC .)
target_compile_definitions(mrhat-rtcwake-lib PUBLIC -DMRHATRTCWAKE_VER="${mrhat-rtcwake-ver}" FMT_HEADER_ONLY)

ER_ADD_EXECUTABLE(mrhat-rtcwake SOURCES main.cpp )
target_link_libraries(mrhat-rtcwake argparse mrhat-rtcwake-lib )


add_executable(mrhat-rtcwake-test test/test_rtc.cpp test/test_utils.cpp test/test_mrhat_integration.cpp rtc_mock.cpp)

target_link_libraries(mrhat-rtcwake-test PRIVATE  mrhat-rtcwake-lib  Catch2::Catch2WithMain )

ER_ENABLE_TEST()

add_test(test-mrhat-rtcwake mrhat-rtcwake-test)


set(CPACK_DEBIAN_PACKAGE_REPLACES "python3-mrhat-rtcwake")
ER_PACK()
