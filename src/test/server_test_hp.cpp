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
#include <httplib.h>

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

using grpc::Channel;
using grpc::ClientContext;

const std::string portOldDefault{"9178"};
const std::string typicalRestDefault{"9179"};

// TODO: Move to utils?
class ServingClient {
    std::unique_ptr<inference::GRPCInferenceService::Stub> stub_;

public:
    ServingClient(std::shared_ptr<Channel> channel) :
        stub_(inference::GRPCInferenceService::NewStub(channel)) {
    }

    // Pre-processing function for synthetic data.
    // gRPC request proto is generated with synthetic data with shape/precision matching endpoint metadata.
    void verifyLive(grpc::StatusCode expectedStatus = grpc::StatusCode::OK, bool alive = true) {
        ClientContext context;
        ::inference::ServerLiveRequest request;
        ::inference::ServerLiveResponse response;

        ASSERT_NE(nullptr, stub_);
        auto status = stub_->ServerLive(&context, request, &response);
        // if we failed to connect it is ok return here
        ASSERT_EQ(status.error_code(), expectedStatus);
        EXPECT_EQ(response.live(), alive);
    }
};

// TODO: Move to utils?
static void requestServerAlive(const char* grpcPort, grpc::StatusCode status = grpc::StatusCode::OK, bool expectedStatus = true) {
    grpc::ChannelArguments args;
    std::string address = std::string("localhost") + ":" + grpcPort;
    SPDLOG_INFO("Verifying if server is live on address: {}", address);
    ServingClient client(grpc::CreateCustomChannel(address, grpc::InsecureChannelCredentials(), args));
    client.verifyLive(status, expectedStatus);
}

static void requestRestServerAlive(const char* httpPort, httplib::StatusCode status = httplib::StatusCode::OK_200, bool expectedStatus = true) {
    std::unique_ptr<httplib::Client> cli{nullptr};
    try {
        cli = std::make_unique<httplib::Client>(std::string("http://localhost:") + httpPort);
    } catch (std::exception& e) {
        SPDLOG_ERROR("Exception caught during rest request:{}", e.what());
        EXPECT_TRUE(false);
        return;
    } catch (...) {
        SPDLOG_ERROR("Exception caught during rest request");
        EXPECT_TRUE(false);
        return;
    }
    try {
        auto res = cli->Get("/v2/health/live");
        if (!res) {
            SPDLOG_ERROR("Got error:{}", httplib::to_string(res.error()));
            EXPECT_TRUE(!expectedStatus);
            return;
        } else {
            EXPECT_TRUE(expectedStatus);
        }
        if (res->status != httplib::StatusCode::OK_200) {
            SPDLOG_ERROR("Failed to get liveness status code: {}, status: {}", res->status, httplib::status_message(res->status));
            EXPECT_TRUE(false);
            return;
        }
    } catch (std::exception& e) {
        SPDLOG_ERROR("Exception caught during rest request:{}", e.what());
        EXPECT_TRUE(false);
        return;
    } catch (...) {
        SPDLOG_ERROR("Exception caught during rest request");
        EXPECT_TRUE(false);
        return;
    }
    return;
}

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
