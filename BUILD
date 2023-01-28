cc_library(
    name = "libmpce",
    hdrs = glob(["*.h"]),
)

cc_binary(
    name = "mpce",
    srcs = ["main.cc"],
    copts = ["--std=c++17"],
    deps = ["//:libmpce"],
)
