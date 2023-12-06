package(default_visibility = ["//visibility:public"])

cc_binary(
  name = "scan.so",
  linkshared = True,
  deps = [
    ":scan",
  ],
)

cc_library(
  name = "scan",
  hdrs = [ "scan.h" ],
  srcs = [ "scan.c" ],
  deps = [
    "//system/third_party/python/python-3.7.3:python",
  ],
)
