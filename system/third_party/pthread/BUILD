package(default_visibility = ["//visibility:public"])

config_setting(name = "win", constraint_values = ["@platforms//os:windows"])
config_setting(name = "mac", constraint_values = ["@platforms//os:osx"])

cc_library(
  name = "pthread",
  srcs = select({
    ":win": [ "lib/x64/pthreadVC2.lib" ],
    "//conditions:default": []
  }),
)
