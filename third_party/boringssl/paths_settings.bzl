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
###############################
# bazel paths settings
###############################

def _boringssl_repository_impl(repository_ctx):
    if repository_ctx.os.name == "linux":
        BORINGSSSL_PATH = "/usr/"
        BORINGSSSL_BUILD = "@//third_party/boringssl:BUILD"
    else: # for windows
        BORINGSSSL_PATH = "C:\\opt\\boringssl"
        BORINGSSSL_BUILD = "@//third_party/boringssl:boringssl_windows.BUILD"

    name = "boringssl"
    path = BORINGSSSL_PATH
    build_file = BORINGSSSL_BUILD
    repository_ctx.file("BUILD", build_file.format(BORINGSSSL_PATH=BORINGSSSL_PATH, BORINGSSSL_BUILD=BORINGSSSL_BUILD))

boringssl_repository = repository_rule(
    implementation = _boringssl_repository_impl,
    environ = ["BASE_IMAGE"],
    local=True,
)

SOURCES = "C:\\opt\\boringssl\\lib\\libssl.lib"
