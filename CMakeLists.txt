cmake_minimum_required(VERSION 3.8)

project(libgpiod VERSION 1.3)

set(ABI_VERSION 3.0.1)
set(ABI_SOVERSION 3)
set(ABI_CXX_VERSION 1.0.0)
set(ABI_CXX_SOVERSION 1)

# put internal CMake-variable to the cache
option(BUILD_SHARED_LIBS "Build libmodbus as a shared library" OFF)

# is this still needed in 2019?
include(CheckIncludeFile)
check_include_file(arpa/inet.h HAVE_ARPA_INET_H)

include(CheckFunctionExists)
check_function_exists(accept4 HAVE_ACCEPT4)

check_function_exists(ioctl HAVE_IOCTL)
check_function_exists(asprintf HAVE_ASPRINTF)
check_function_exists(scandir HAVE_SCANDIR)
check_function_exists(alphasort HAVE_ALPHASORT)
check_function_exists(ppoll HAVE_PPOLL)

check_include_file(getopt.h        HAVE_GETOPT_H)
check_include_file(dirent.h,       HAVE_DIRENT_H)
check_include_file(sys/poll.h      HAVE_SYS_POLL_H)
check_include_file(sys/sysmacros.h HAVE_SYS_SYSMACROS_H)
check_include_file(linux/gpio.h    HAVE_LINUX_GPIO_H)

include(GNUInstallDirs)

add_subdirectory(src/lib)

option(ENABLE_TOOLS "enable libgpiod command-line tools" OFF)
if(ENABLE_TOOLS)
    add_subdirectory(src/tools)
endif()

option(ENABLE_BINDINGS_CXX "enable C++ bindings" OFF)
if(ENABLE_BINDINGS_CXX)
    add_subdirectory(bindings/cxx)
endif()

option(ENABLE_BINDINGS_PYTHON "enable python3 bindings" OFF)
if(ENABLE_BINDINGS_PYTHON)
    add_subdirectory(bindings/python)
endif()

option(BUILD_TESTS "enable libgpiod tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()


