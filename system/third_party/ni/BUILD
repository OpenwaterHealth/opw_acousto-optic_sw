package(default_visibility = ["//visibility:public"])

config_setting(name = "win", constraint_values = ["@platforms//os:windows"])
config_setting(name = "mac", constraint_values = ["@platforms//os:osx"])

cc_library(
  name = "nidaqmx",
  hdrs = [ "inc/NIDAQmx.h", "inc/nisyscfg.h", "inc/nisyscfg_errors.h", "inc/nisyscfg_wide.h" ],
  srcs = select({
    ":win": [ "lib/NIDAQmx.lib", "lib/nisyscfg.lib" ],
    "//conditions:default": []
  }),
)
