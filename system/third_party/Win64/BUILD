package(default_visibility = ["//visibility:public"])

config_setting(name = "win", constraint_values = ["@platforms//os:windows"])
config_setting(name = "mac", constraint_values = ["@platforms//os:osx"])

cc_library(
  name = "visa",
  hdrs = [
    "Include/visa.h",
    "Include/visatype.h",
  ],
  srcs = select({
    ":win": [ "Lib_x64/msc/visa32.lib" ],
    "//conditions:default": []
  }),
)
