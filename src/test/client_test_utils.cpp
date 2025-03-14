#include "client_test_utils.hpp"

#include "../logging.hpp"

void requestServerAlive(const char* grpcPort, grpc::StatusCode status, bool expectedStatus) {
    grpc::ChannelArguments args;
    std::string address = std::string("localhost") + ":" + grpcPort;
    SPDLOG_INFO("Verifying if server is live on address: {}", address);
    ServingClient client(grpc::CreateCustomChannel(address, grpc::InsecureChannelCredentials(), args));
    client.verifyLive(status, expectedStatus);
}

void requestServerReady(const char* grpcPort, grpc::StatusCode status, bool expectedStatus) {
    grpc::ChannelArguments args;
    std::string address = std::string("localhost") + ":" + grpcPort;
    SPDLOG_INFO("Verifying if server is ready on address: {}", address);
    ServingClient client(grpc::CreateCustomChannel(address, grpc::InsecureChannelCredentials(), args));
    client.verifyReady(status, expectedStatus);
}

void requestModelReady(const char* grpcPort, const std::string& modelName, grpc::StatusCode status, bool expectedStatus) {
    grpc::ChannelArguments args;
    std::string address = std::string("localhost") + ":" + grpcPort;
    SPDLOG_INFO("Verifying if model is ready on address: {}", address);
    ServingClient client(grpc::CreateCustomChannel(address, grpc::InsecureChannelCredentials(), args));
    client.verifyModelReady(modelName, status, expectedStatus);
}

void checkServerMetadata(const char* grpcPort, grpc::StatusCode status) {
    grpc::ChannelArguments args;
    std::string address = std::string("localhost") + ":" + grpcPort;
    SPDLOG_INFO("Verifying if server responds with correct metadata on address: {}", address);
    ServingClient client(grpc::CreateCustomChannel(address, grpc::InsecureChannelCredentials(), args));
    client.verifyServerMetadata(status);
}

void requestRestServerAlive(const char* httpPort, httplib::StatusCode status, bool expectedStatus) {
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
