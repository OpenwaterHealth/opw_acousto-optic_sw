package(default_visibility = ["//visibility:public"])

config_setting(name = "win", constraint_values = ["@platforms//os:windows"])
config_setting(name = "mac", constraint_values = ["@platforms//os:osx"])
config_setting(name = "linux", constraint_values = ["@platforms//os:linux"])

cc_import(
  name = "visa32DLL",
  shared_library = "dll/visa32.dll",
)

cc_import(
  name = "visa64DLL",
  shared_library = "dll/visa64.dll",
)

cc_import(
  name = "visaConfMgrDLL",
  shared_library = "dll/visaConfMgr.dll",
)

cc_import(
  name = "visaUtilitiesDLL",
  shared_library = "dll/visaUtilities.dll",
)

cc_library(
  name = "tiff",
  hdrs = [
    "inc/tiff.h",
    "inc/tiffconf.h",
    "inc/tiffio.h",
    "inc/tiffvers.h",
  ],
  srcs = select({
    ":win": [ "libx64/tiff.lib" ],
    ":linux": [ "tiff/linux/lib/libtiff.so.5", "tiff/linux/lib/libjbig.so.0" ],
    "//conditions:default": []
  }),
)

cc_import(
  name = "tiff_dll",
  shared_library = "dll/tiff.dll",
)
