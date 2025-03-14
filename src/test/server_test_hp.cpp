//*****************************************************************************
// Copyright 2025 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*****************************************************************************

#include <gmock/gmock.h>
#include <grpcpp/create_channel.h>
#include <gtest/gtest.h>

#include <string>
#include <chrono>

#include "../cleaner_utils.hpp"
#include "../dags/node_library.hpp"
#include "../kfs_frontend/kfs_grpc_inference_service.hpp"
#include "../localfilesystem.hpp"
#include "../logging.hpp"
#include "../model.hpp"
#include "../modelinstanceunloadguard.hpp"
#include "../modelmanager.hpp"
#include "../module_names.hpp"
#include "../prediction_service_utils.hpp"
#include "../servablemanagermodule.hpp"
#include "../server.hpp"
#include "../version.hpp"
#include "c_api_test_utils.hpp"
#include "client_test_utils.hpp"

using grpc::Channel;
using grpc::ClientContext;

const std::string portOldDefault{"9178"};
const std::string typicalRestDefault{"9179"};

TEST(Server, CAPIAliveGrpcNotHttpYes) {
    std::string port = "9000";
    randomizePort(port);
    char* argv[] = {
        (char*)"OpenVINO Model Server",
        (char*)"--model_name",
        (char*)"dummy",
        (char*)"--rest_port",
        (char*)port.c_str(),
        (char*)"--model_path",
        (char*)getGenericFullPathForSrcTest("/ovms/src/test/dummy").c_str(),
        nullptr};

    ovms::Server& server = ovms::Server::instance();
    bool isLive = true;
    auto* cserver = reinterpret_cast<OVMS_Server*>(&server);
    OVMS_ServerLive(cserver, &isLive);
    ASSERT_TRUE(!isLive);
    std::thread t([&argv, &server]() {
        ASSERT_EQ(EXIT_SUCCESS, server.start(7, argv));
    });
    auto start = std::chrono::high_resolution_clock::now();
    while ((ovms::Server::instance().getModuleState(ovms::SERVABLE_MANAGER_MODULE_NAME) != ovms::ModuleState::INITIALIZED) &&
           (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count() < 5)) {
    }
    isLive = false;
    OVMS_ServerLive(cserver, &isLive);
    ASSERT_TRUE(isLive);
    // GrPC is initialized before Servable ManagerModule
    requestServerAlive(portOldDefault.c_str(), grpc::StatusCode::UNAVAILABLE, false);
    requestRestServerAlive(port.c_str());
    server.setShutdownRequest(1);
    t.join();
    server.setShutdownRequest(0);
}
