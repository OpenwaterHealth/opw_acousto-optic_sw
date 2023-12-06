package(default_visibility = ["//visibility:public"])

config_setting(name = "win", constraint_values = ["@platforms//os:windows"])
config_setting(name = "mac", constraint_values = ["@platforms//os:osx"])
config_setting(name = "linux", constraint_values = ["@platforms//os:linux"])

cc_library(
  name = "glog_win",
  hdrs = [
    "win/include/glog/logging.h",
    "win/include/glog/log_severity.h",
    "win/include/glog/vlog_is_on.h",
  ],
  srcs = [
    "win/src/base/commandlineflags.h",
    "win/src/base/googleinit.h",
    "win/src/base/mutex.h",
    "win/src/config.h",
    "win/src/demangle.cc",
    "win/src/demangle.h",
    "win/src/flagsv.cpp",
    "win/src/glog/raw_logging.h",
    "win/src/logging.cc",
    "win/src/port.cc",
    "win/src/port.h",
    "win/src/raw_logging.cc",
    "win/src/signalhandler.cc",
    "win/src/stacktrace.h",
    "win/src/stacktrace_windows-inl.h",
    "win/src/symbolize.cc",
    "win/src/symbolize.h",
    "win/src/utilities.cc",
    "win/src/utilities.h",
    "win/src/windows/dirent.h",
  ],
  defines = [
    "GOOGLE_GLOG_DLL_DECL=",  # empty = not building as a DLL
    "HAVE_SNPRINTF",
  ],
  strip_include_prefix = "win/include",
)

cc_library(
  name = "glog_mac",
  hdrs = [
    "mac/include/glog/logging.h",
    "mac/include/glog/log_severity.h",
    "mac/include/glog/vlog_is_on.h",
  ],
  srcs = [
    "mac/src/stubs.cc",
    "mac/lib/libglog.a",
  ],
  strip_include_prefix = "mac/include",
)

# for linux compile only, no libs
cc_library(
  name = "glog_linux",
  hdrs = [
    "mac/include/glog/logging.h",
    "mac/include/glog/log_severity.h",
    "mac/include/glog/vlog_is_on.h",
  ],
  srcs = [
    "mac/src/stubs.cc",
    "linux/src/stubs.cc",
  ],
  strip_include_prefix = "mac/include",
)

cc_library(
  name = "glog",
  deps = select({
    ":win": [ "glog_win" ],
    ":mac": [ "glog_mac" ],
    ":linux": [ "glog_linux" ],
    "//conditions:default": []
  }),
)
