package(default_visibility = ["//visibility:public"])

config_setting(name = "win", constraint_values = ["@platforms//os:windows"])
config_setting(name = "mac", constraint_values = ["@platforms//os:osx"])
config_setting(name = "linux", constraint_values = ["@platforms//os:linux"])

cc_library(
  name = "circular_buffer",
  hdrs = [ "inc/circular_buffer.h" ],
)

cc_library(
  name = "cli",
  hdrs = [ "inc/cli.h" ],
  srcs = [ "src/cli.cpp" ],
  deps = [
    "//system/third_party/SFML-2.5.1:SFML",
  ],
)

cc_library(
  name = "execnode",
  hdrs = [ "inc/execnode.h" ],
  srcs = [ "src/execnode.cpp" ],
  deps = [
    ":circular_buffer",
  ],
)

cc_library(
  name = "fftt",
  hdrs = [ "inc/fftt.h" ],
  srcs = [ "src/fftt.cpp" ],
  deps = [
    ":fftwutil",
    ":pool",
    ":rcam",
    ":time",
    "//system/third_party/fftw:fftw",
    "//system/third_party/SFML-2.5.1:SFML",
  ],
)

cc_library(
  name = "fftwutil",
  hdrs = [ "inc/fftwutil.h" ],
  srcs = [ "src/fftwutil.cpp" ],
  deps = [
    ":rcam",
    ":zoomable",
    "//system/third_party/SFML-2.5.1:SFML",
  ],
)

cc_library(
  name = "filterdev",
  hdrs = [ "inc/filterdev.h" ],
  srcs = [ "src/filterdev.cpp" ],
  deps = [
    ":execnode",
    ":frame",
    ":pool",
    "//system/third_party/json-develop:json_develop",
  ],
)

cc_library(
  name = "frame",
  hdrs = [ "inc/frame.h" ],
  srcs = [ "src/frame.cpp" ],
  deps = [
    ":tiff_interface",
    ":time",
    "//system/third_party:tiff",
  ],
)

cc_library(
  name = "colormap",
  hdrs = [ "inc/colormap.h" ],
  srcs = [ "src/colormap.cpp" ]
)

cc_library(
  name = "frame_draw",
  hdrs = [ "inc/frame_draw.h" ],
  srcs = [ "src/frame_draw.cpp" ],
  deps = [
    ":colormap",
    ":execnode",
    ":frame",
    ":zoomable",
    "//system/third_party/SFML-2.5.1:SFML",
  ],
)

cc_test(
  name = "frame_test",
  srcs = [ "test/frame_test.cpp" ],
  linkopts = select({
    ":win": [ "advapi32.lib", "user32.lib" ],
    "//conditions:default": [],
  }),
  deps = [
    ":frame",
    ":tiff_interface",
    "//googletest:gtest",
    "//googletest:gtest_main",
  ] + select({
    ":win": [ "//system/third_party:tiff_dll" ],
    "//conditions:default": [],
  }),
)

cc_library(
  name = "fx3",
  hdrs = [ "inc/fx3.h" ],
  srcs = [ "src/fx3.cpp" ],
  deps = [
  ] + select({
    ":win": [ "//system/third_party/cypress-fx3:CyAPI" ],
    ":mac": [ "//system/third_party/cypress-fx3/cyusb_mac_1.0:cyusb_mac" ],
    ":linux": [ "//system/third_party/cypress-fx3/cyusb_linux_1.0.5:cyusb_linux" ],
    "//conditions:default": [],
  })
)

cc_library(
  name = "intelhex",
  hdrs = [ "inc/intelhex.h" ],
  srcs = [ "src/intelhex.cpp" ],
)

cc_library(
  name = "invertroi",
  hdrs = [ "inc/invertroi.h" ],
  srcs = [ "src/invertroi.cpp" ],
  deps = [
    ":execnode",
    ":fftt",
    ":frame",
    ":fftwutil", ]
)

cc_library(
  name = "octopus",
  hdrs = [ "inc/octopus.h",
           "inc/octo_fw.h",
           "inc/prettyPrintOctopusRegisters.h", ],
  srcs = [ "src/octopus.cpp" ,
           "src/prettyPrintOctopusRegisters.c", ],
  deps = [ ":fx3" ],
)

cc_library(
  name = "pool",
  srcs = ["inc/pool.h"],
)

cc_library(
  name = "rcam",
  hdrs = [
    "inc/rcam.h",
    "inc/rcam_param.h",
    "inc/rcam_fw.h",
  ],
  srcs = [
    "src/rcam.cpp",
  ],
  deps = [
    ":circular_buffer",
    ":execnode",
    ":frame",
    ":fx3",
    ":time",
  ] + select({
    ":win": [ "//system/third_party/cypress-fx3:CyAPI" ],
    ":mac": [ "//system/third_party/cypress-fx3/cyusb_mac_1.0:cyusb_mac" ],
    ":linux": [ "//system/third_party/cypress-fx3/cyusb_linux_1.0.5:cyusb_linux" ],
    "//conditions:default": [],
  })
)

cc_test(
  name = "rcam_test",
  srcs = [ "test/rcam_test.cpp" ],
  linkopts = select({
    ":win": [ "advapi32.lib", "user32.lib", "/NODEFAULTLIB:libcmt.lib" ],
    "//conditions:default": [],
  }),
  deps = [
    ":rcam",
    "//googletest:gtest",
    "//googletest:gtest_main",
  ] + select({
    ":win": [],
    "//conditions:default": [],
  }),
)

cc_library(
  name = "roi",
  hdrs = [ "inc/roi.h" ],
  srcs = [ "src/roi.cpp" ],
  deps = [
    ":fftt",
    ":fftwutil",
    ":rcam",
    "//system/third_party/fftw:fftw",
    "//system/third_party/SFML-2.5.1:SFML",
  ],
)

cc_library(
  name = "serial",
  hdrs = [ "inc/serial.h" ],
  srcs = [ "src/serial.cpp" ],
)

cc_library(
  name = "stddev",
  hdrs = [ "inc/stddev.h" ],
  srcs = [ "src/stddev.cpp"],
  deps = [
    ":execnode",
    ":frame",
    ":pool",
  ],
)

cc_library(
  name = "tiff_interface",
  hdrs = [ "inc/TiffInterface.h" ],
)

cc_library(
  name = "time",
  hdrs = [ "inc/time.h" ],
)

cc_library(
  name = "ustx",
  hdrs = [ "inc/ustx.h" ],
  srcs = [ "src/ustx.cpp" ],
  deps = [
    ":intelhex",
    ":serial",
    "//system/third_party/json-develop:json_develop",
  ]
)

cc_library(
  name = "zoomable",
  hdrs = [ "inc/zoomable.h" ],
  srcs = [ "src/zoomable.cpp" ],
  includes = [ "system/third_party/SFML-2.5.1/include" ],  # N.B.: Propagates up!
  deps = [
    "//system/third_party/SFML-2.5.1:SFML",
  ],
)
