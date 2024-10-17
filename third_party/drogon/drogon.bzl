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

load("@bazel_tools//tools/build_defs/repo:git.bzl", "new_git_repository")

def drogon_cpp():
    drogon_cpp_repository(name="_drogon_cpp")
    new_git_repository(
        name = "drogon",
        remote = "https://github.com/drogonframework/drogon",
        tag = "v1.9.7",  # Sep 10 2024
        build_file = "@_drogon_cpp//:BUILD",
        init_submodules = True,
        recursive_init_submodules = True,
        patch_cmds = ["find . -name '中文.txt' -delete"],
    )

def _impl(repository_ctx):
    http_proxy = repository_ctx.os.environ.get("http_proxy", "")
    https_proxy = repository_ctx.os.environ.get("https_proxy", "")

    result = repository_ctx.execute(["cat","/etc/os-release"],quiet=False)
    ubuntu20_count = result.stdout.count("PRETTY_NAME=\"Ubuntu 20")
    ubuntu22_count = result.stdout.count("PRETTY_NAME=\"Ubuntu 22") 

    if ubuntu20_count == 1 or ubuntu22_count == 1:
        lib_path = "lib"  # TODO: Check install path for ubuntu/redhat (lib/lib64)
    else: # for redhat
        lib_path = "lib64"

    # Note we need to escape '{/}' by doubling them due to call to format
    build_file_content = """
load("@rules_foreign_cc//foreign_cc:cmake.bzl", "cmake")
load("@bazel_skylib//rules:common_settings.bzl", "string_flag")

visibility = ["//visibility:public"]

config_setting(
    name = "dbg",
    values = {{"compilation_mode": "dbg"}},
)

config_setting(
    name = "opt",
    values = {{"compilation_mode": "opt"}},
)

filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"],
)

build_release = {{"CMAKE_BUILD_TYPE": "Release"}}
build_debug = {{"CMAKE_BUILD_TYPE": "Debug"}}

cmake(
    name = "drogon_cmake",
    build_args = [
        "--verbose",
        "--",  # <- Pass remaining options to the native tool.
        # https://github.com/bazelbuild/rules_foreign_cc/issues/329
        # there is no elegant parallel compilation support
        "VERBOSE=1",
        "-j 6",
    ],
    cache_entries = {{
        "BUILD_TESTING": "OFF",
        "BUILD_CTL": "OFF",
        "BUILD_EXAMPLES": "OFF",
        "BUILD_ORM": "OFF",
        "BUILD_BROTLI": "OFF",
        "BUILD_YAML_CONFIG": "OFF",
        "CMAKE_POSITION_INDEPENDENT_CODE": "ON",
        "CMAKE_CXX_FLAGS": " -s -D_GLIBCXX_USE_CXX11_ABI=1 -Wno-error=deprecated-declarations -Wuninitialized\"
    }} | select({{
           "//conditions:default": dict(
               build_release
            ),
            ":dbg":  dict(
               build_debug
            ),
        }}),
    env = {{
        "HTTP_PROXY": "{http_proxy}",
        "HTTPS_PROXY": "{https_proxy}",
    }},
    lib_source = ":all_srcs",
    out_lib_dir = "{lib_path}",
    # linking order
    out_static_libs = [
            "libdrogon.a",
            "libtrantor.a",
        ],
    tags = ["requires-network"],
    alwayslink = False,
    visibility = ["//visibility:public"],
)

cc_library(
    name = "drogon",
    deps = [
        ":drogon_cmake",
    ],
    visibility = ["//visibility:public"],
    alwayslink = False,
)
"""
    repository_ctx.file("BUILD", build_file_content.format(http_proxy=http_proxy, https_proxy=https_proxy, lib_path=lib_path))

drogon_cpp_repository = repository_rule(
    implementation = _impl,
    local=False,
)
