#
# Copyright (c) 2024 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

package(
    default_visibility = ["//visibility:public"],
)

load("@//third_party/boringssl:paths_settings.bzl", "SOURCES")

cc_library(
    name = "ssl",
    hdrs = select({
        "//conditions:default": glob(["include/openssl/*"]),
        "//src:windows" :  glob(["windows_boringssl\\include\\openssl\\*"]),
    }),
    srcs = select({
                "//conditions:default": glob(["lib/x86_64-linux-gnu/libssl.so"]),
                "//src:windows" : glob(["windows_boringssl\\lib\\libssl.lib"]),
    }),
    deps = select({
        "//conditions:default": [],
        "//src:windows" :  ["@windows_boringssl//:ssl"],
        }),
    copts = ["-lcrypto", "-lssl"],
    visibility = ["//visibility:public"],
    strip_include_prefix = "windows_boringssl",
)

cc_library(
    name = "crypto",
    hdrs = glob(["include/openssl/*"]),
    srcs = glob(["lib/x86_64-linux-gnu/libssl.so"]),
    copts = ["-lcrypto", "-lssl"],
    visibility = ["//visibility:public"],
)
