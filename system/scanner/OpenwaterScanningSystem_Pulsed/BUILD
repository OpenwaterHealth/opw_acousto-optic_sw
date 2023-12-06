package(default_visibility = ["//visibility:public"])

config_setting(name = "win", constraint_values = ["@platforms//os:windows"])
config_setting(name = "mac", constraint_values = ["@platforms//os:osx"])
config_setting(name = "linux", constraint_values = ["@platforms//os:linux"])

# This is the main scanner app. Succeeding targets are in alphabetical order.
cc_binary(
  name = "scan",
  srcs = [ 'OpenwaterScanningSystem_Pulsed.cpp' ],
  linkopts = select({
    ":win": [
      "advapi32.lib",
      "user32.lib",
      "ws2_32.lib",
      "/NODEFAULTLIB:libcmt.lib",
    ],
    "//conditions:default": []
  }),
  deps = [
    ":async_scanner",
    ":camera_manager_fx3",
    ":delays",
    ":octopus_manager",
    ":quantum_composers",
    ":robot",
    ":robot_scanner",
    ":rotisserie_scanner",
    ":verdi",
    "//system/third_party/glog:glog",
    "//system/third_party/json-develop:json_develop",
  ]
)

cc_library(
  name = "AmpStage",
  hdrs = ["AmpStage.h"],
  srcs = ["AmpStage.cpp"],
  deps = [
    ":stages",
    "//system/component:time",
  ],
)

cc_library(
  name = "async_scanner",
  hdrs = ["AsyncScanner.h"],
  srcs = ["AsyncScanner.cpp"],
  deps = [
    ":ConexStage",
    ":scanner",
    "//system/third_party/glog:glog",
  ],
)

cc_library(
  name = "camera_manager",
  hdrs = [ "CameraManager.h" ],
  deps = [
    ":trigger",
  ],
)

cc_library(
  name = "camera_manager_fx3",
  srcs = ["CameraManager.cpp"],
  deps = [
    ":camera_manager",
    ":FrameSave",
    ":trigger",
    ":voxel_data",
    ":VoxelSave",
    "//system/component:fx3",
    "//system/component:rcam",
    "//system/component:roi",
    "//system/component:stddev",
    "//system/component:time",
    "//system/third_party/json-develop:json_develop",
    "//system/third_party/SFML-2.5.1:SFML",
  ],
)

cc_library(
  name = "ConexStage",
  hdrs = ["ConexStage.h"],
  srcs = ["ConexStage.cpp"],
  deps = [
    ":stages",
  ],
)

cc_library(
  name = "delays",
  hdrs = ["delays.h"],
  srcs = ["delays.cpp"],
  copts = select({
    ":linux": [ "-fprofile-instr-generate -fcoverage-mapping" ],
    "//conditions:default": [],
  }),
  linkopts = select({
    ":linux": [ "-fprofile-instr-generate -fcoverage-mapping" ],
    "//conditions:default": [],
  }),
  deps = [
    ":rs232_wrapper",
    "//system/component:time",
    "//system/third_party/glog:glog",
    "//system/third_party/rs232",
  ],
)

cc_test(
  name = "delays_test",
  srcs = ["test/delays_test.cpp"],
  copts = select({
    ":linux": [ "-fprofile-instr-generate -fcoverage-mapping" ],
    "//conditions:default": [],
  }),
  linkopts = select({
    ":linux": [ "-fprofile-instr-generate -fcoverage-mapping" ],
    "//conditions:default": [],
  }),
  deps = [
    ":delays",
    "//googletest:gtest",
    "//googletest:gtest_main",
  ],
)

cc_library(
  name = "FrameSave",
  hdrs = ["FrameSave.h"],
  srcs = ["FrameSave.cpp"],
  deps = [
    "//system/component:execnode",
    "//system/component:frame",
  ],
)

cc_library(
  name = "laser",
  hdrs = ["laser.h"],
)

cc_test(
  name = "laser_test",
  srcs = ["test/laser_test.cpp"],
  copts = select({
    ":linux": [ "-fprofile-instr-generate -fcoverage-mapping" ],
    "//conditions:default": [],
  }),
  linkopts = select({
    ":linux": [ "-fprofile-instr-generate -fcoverage-mapping" ],
    "//conditions:default": [],
  }),
  deps = [
    ":quantum_composers",
    ":verdi",
    "//googletest:gtest",
    "//googletest:gtest_main",
  ],
)

cc_library(
  name = "octopus_manager",
  hdrs = ["OctopusManager.h"],
  srcs = ["OctopusManager.cpp"],
  deps = [
    ":trigger",
    "//system/component:octopus",
    "//system/third_party/json-develop:json_develop",
  ],
)

cc_library(
  name = "quantum_composers",
  hdrs = ["QuantumComposers.h"],
  srcs = ["QuantumComposers.cpp"],
  copts = select({
    ":linux": [ "-fprofile-instr-generate -fcoverage-mapping" ],
    "//conditions:default": [],
  }),
  linkopts = select({
    ":linux": [ "-fprofile-instr-generate -fcoverage-mapping" ],
    "//conditions:default": [],
  }),
  deps = [
    ":laser",
    ":rs232_wrapper",
    "//system/third_party/rs232",
    "//system/third_party/json-develop:json_develop",
  ],
)

cc_library(
  name = "rigol",
  hdrs = ["rigol.h"],
  srcs = ["rigol.cpp"],
  deps = [
    "//system/third_party/Win64:visa",
  ] + select({
    ":win": [
      "//system/third_party:visa32DLL",
      "//system/third_party:visa64DLL",
      "//system/third_party:visaConfMgrDLL",
      "//system/third_party:visaUtilitiesDLL",
    ],
    "//conditions:default": []
  }),
)

cc_library(
  name = "robot_header",
  hdrs = ["robot.h"],
)

cc_library(
  name = "robot",
  hdrs = ["robot.h"],
  srcs = ["robot.cpp"],
  deps = [
    "//system/component:time",
    "//system/third_party/glog:glog",
    "//system/third_party/practical-socket",
  ],
)

cc_library(
  name = "robot_scanner",
  hdrs = ["RobotScanner.h"],
  srcs = ["RobotScanner.cpp"],
  deps = [
    ":robot_header",
    ":scanner",
  ],
)

cc_library(
  name = "rotisserie_scanner",
  hdrs = ["RotisserieScanner.h"],
  srcs = ["RotisserieScanner.cpp"],
  deps = [
    ":AmpStage",
    ":ConexStage",
    ":scanner",
  ],
)

cc_library(
  name = "rs232_wrapper",
  hdrs = ["rs232_wrapper.h"],
)

cc_library(
  name = "scanner",
  hdrs = ["scanner.h"],
  srcs = ["scanner.cpp"],
  deps = [
    ":camera_manager",
    ":delays",
    ":laser",
    ":octopus_manager",
    ":quantum_composers",
    ":trigger",
    ":voxel_data",
    "//system/component:time",
    "//system/component:ustx",
    "//system/third_party/glog:glog",
    "//system/third_party/json-develop:json_develop",
    "//system/third_party/pthread:pthread",
  ],
)

cc_test(
  name = "scanner_test",
  srcs = ["test/scanner_test.cpp"],
  linkopts = select({
    ":win": [ "advapi32.lib", "user32.lib", "ws2_32.lib", "/NODEFAULTLIB:libcmt.lib" ],
    ":linux": [ "-lstdc++fs" ],
    "//conditions:default": [],
  }),
  deps = [
    ":async_scanner",
    ":camera_manager",
    ":ConexStage",
    ":delays",
    ":laser",
    ":octopus_manager",
    ":robot_scanner",
    "//googletest:gtest",
    "//googletest:gtest_main",
    "//system/component:ustx",
    "//system/third_party/glog:glog",
    "//system/third_party/json-develop:json_develop",
  ] + select({
    ":win": [
      "//system/component:fftt",
      "//system/third_party/fftw:fftw_dll",
      "//system/third_party:tiff_dll",
    ],
    "//conditions:default": []
  }),
)

cc_library(
  name = "stages",
  hdrs = ["stages.h"],
  deps = [
    ":rs232_wrapper",
    "//system/third_party/rs232",
  ],
)

cc_test(
  name = "stages_test",
  srcs = ["test/stages_test.cpp"],
  deps = [
    ":ConexStage",
    "//googletest:gtest",
    "//googletest:gtest_main",
  ],
)

cc_library(
  name = "trigger",
  hdrs = ["trigger.h"],
)

cc_library(
  name = "verdi",
  hdrs = ["Verdi.h"],
  srcs = ["Verdi.cpp"],
  deps = [
    ":laser",
    ":rs232_wrapper",
    "//system/component:time",
    "//system/third_party/glog:glog",
    "//system/third_party/json-develop:json_develop",
    "//system/third_party/rs232",
  ],
)

cc_library(
  name = "voxel_data",
  hdrs = ["VoxelData.h"],
)

cc_library(
  name = "VoxelSave",
  hdrs = ["VoxelSave.h"],
  srcs = ["VoxelSave.cpp"],
  deps = [
    ":camera_manager",
    ":voxel_data",
    "//system/component:execnode",
    "//system/component:fftt",
    "//system/component:frame",
    "//system/component:roi",
    "//system/component:stddev",
    "//system/third_party/json-develop:json_develop",
  ],
)
