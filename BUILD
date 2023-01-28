cc_library(
    name = "libmpce",
    srcs = [
        "cpu_state.cc",
        "interrupt.cc",
        "io_serial.cc",
        "memory.cc",
        "mmio.cc",
        "mmu.cc",
    ],
    hdrs = glob(["*.h"]),
    copts = ["--std=c++17"],
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "@com_github_google_glog//:glog",
    ],
)

cc_binary(
    name = "mpce",
    srcs = ["main.cc"],
    copts = ["--std=c++17"],
    deps = ["//:libmpce"],
)
