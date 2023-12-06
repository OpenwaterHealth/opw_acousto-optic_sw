package(default_visibility = ["//visibility:public"])

config_setting(name = "win", constraint_values = ["@platforms//os:windows"])
config_setting(name = "mac", constraint_values = ["@platforms//os:osx"])

cc_library(
  name = "fftw_win",
  hdrs = [ "fftw-3.3.5-dll64/fftw3.h" ],
  strip_include_prefix = "fftw-3.3.5-dll64",
  srcs = [ "fftw-3.3.5-dll64/libfftw3-3.lib" ],
)

cc_library(
  name = "fftw_mac",
  hdrs = [ "fftw3_3.3.8_mac/include/fftw3.h" ],
  strip_include_prefix = "fftw3_3.3.8_mac/include",
  srcs = [
    "fftw3_3.3.8_mac/lib/libfftw3.a",
    "fftw3_3.3.8_mac/lib/libfftw3_threads.a",
  ],
)

cc_library(
  name = "fftw",
  deps = select({
    ":win": [ "fftw_win" ],
    ":mac": [ "fftw_mac" ],
    "//conditions:default": [ "fftw_win" ]  # for linux compile only, no libs
  }),
)

cc_import(
  name = "fftw_dll",
  shared_library = "fftw-3.3.5-dll64/libfftw3-3.dll",
)

cc_import(
  name = "fftw_mac_lib",
  shared_library = "fftw3_3.3.8_mac/lib/libfftw3.3.dylib",
)
