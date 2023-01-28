cc_library(
    name = "libmpce",
    hdrs = glob(["*.h"]),
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
