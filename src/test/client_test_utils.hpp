#include <gmock/gmock.h>
#include <grpcpp/server.h>
#include <grpcpp/create_channel.h>
#include <gtest/gtest.h>

#include <memory>

#include "../kfs_frontend/kfs_grpc_inference_service.hpp"
#include "../version.hpp"

class ServingClient {
    std::unique_ptr<inference::GRPCInferenceService::Stub> stub_;

public:
    ServingClient(std::shared_ptr<grpc::Channel> channel) :
        stub_(inference::GRPCInferenceService::NewStub(channel)) {
    }

    // Pre-processing function for synthetic data.
    // gRPC request proto is generated with synthetic data with shape/precision matching endpoint metadata.
    void verifyLive(grpc::StatusCode expectedStatus = grpc::StatusCode::OK, bool alive = true) {
        grpc::ClientContext context;
        ::inference::ServerLiveRequest request;
        ::inference::ServerLiveResponse response;

        ASSERT_NE(nullptr, stub_);
        auto status = stub_->ServerLive(&context, request, &response);
        // if we failed to connect it is ok return here
        ASSERT_EQ(status.error_code(), expectedStatus);
        EXPECT_EQ(response.live(), alive);
    }

    void verifyReady(grpc::StatusCode expectedStatus = grpc::StatusCode::OK, bool ready = true) {
        grpc::ClientContext context;
        ::inference::ServerReadyRequest request;
        ::inference::ServerReadyResponse response;

        ASSERT_NE(nullptr, stub_);
        auto status = stub_->ServerReady(&context, request, &response);
        ASSERT_EQ(status.error_code(), expectedStatus);
        EXPECT_EQ(response.ready(), ready);
    }
    void verifyModelReady(const std::string& modelName, grpc::StatusCode expectedStatus = grpc::StatusCode::OK, bool ready = true) {
        grpc::ClientContext context;
        ::KFSGetModelStatusRequest request;
        ::KFSGetModelStatusResponse response;
        request.set_name(modelName);

        ASSERT_NE(nullptr, stub_);
        auto status = stub_->ModelReady(&context, request, &response);
        ASSERT_EQ(status.error_code(), expectedStatus);
        EXPECT_EQ(response.ready(), ready);
    }

    void verifyServerMetadata(grpc::StatusCode expectedStatus = grpc::StatusCode::OK) {
        grpc::ClientContext context;
        ::KFSServerMetadataRequest request;
        ::KFSServerMetadataResponse response;
        ASSERT_NE(nullptr, stub_);
        auto status = stub_->ServerMetadata(&context, request, &response);
        ASSERT_EQ(status.error_code(), expectedStatus);
        EXPECT_EQ(response.name(), PROJECT_NAME);
        EXPECT_EQ(response.version(), PROJECT_VERSION);
        EXPECT_EQ(response.extensions().size(), 0);
    }
};
