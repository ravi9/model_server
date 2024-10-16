//*****************************************************************************
// Copyright 2020 Intel Corporation
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
#include "drogon_endpoints.hpp"

#include <thread>
#include <string>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "logging.hpp"
#include "status.hpp"
#include "http_payload.hpp"

#include "modelmanager.hpp"

#if (MEDIAPIPE_DISABLE == 0)
#include "http_frontend/http_client_connection.hpp"
#include "http_frontend/drogon_graph_executor_impl.hpp"
#include "mediapipe_internal/mediapipegraphexecutor.hpp"
#endif

using rapidjson::Document;
using rapidjson::SizeType;
using rapidjson::Value;

namespace ovms {

Status processDrogonV3(ModelManager* mm, const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    SPDLOG_ERROR("Received request for server side event");

#if (MEDIAPIPE_DISABLE == 0)
    std::string request_body = std::string(req->getBody());
    std::string uri = req->getPath();
    SPDLOG_ERROR("AAAAAAAAAAAA: {}\nBBBBBBBBB: {}", request_body, uri);
    HttpPayload request;
    std::shared_ptr<Document> doc = std::make_shared<Document>();
    std::shared_ptr<MediapipeGraphExecutor> executor;
    bool streamFieldVal = false;
    {
        doc->Parse(request_body.c_str());
    }
    {
        if (doc->HasParseError()) {
            return Status(StatusCode::JSON_INVALID, "Cannot parse JSON body");
        }

        if (!doc->IsObject()) {
            return Status(StatusCode::JSON_INVALID, "JSON body must be an object");
        }

        auto modelNameIt = doc->FindMember("model");
        if (modelNameIt == doc->MemberEnd()) {
            return Status(StatusCode::JSON_INVALID, "model field is missing in JSON body");
        }

        if (!modelNameIt->value.IsString()) {
            return Status(StatusCode::JSON_INVALID, "model field is not a string");
        }

        const std::string model_name = modelNameIt->value.GetString();

        bool isTextGenerationEndpoint = uri.find("completions") != std::string_view::npos;
        if (isTextGenerationEndpoint) {
            auto streamIt = doc->FindMember("stream");
            if (streamIt != doc->MemberEnd()) {
                if (!streamIt->value.IsBool()) {
                    return Status(StatusCode::JSON_INVALID, "stream field is not a boolean");
                }
                streamFieldVal = streamIt->value.GetBool();
            }
        }

        auto status = mm->createPipeline(executor, model_name);
        if (!status.ok()) {
            return status;
        }
        // TODO: Possibly avoid making copy
        //request.headers = request_components.headers;  TODO
        request.body = request_body;
        request.parsedJson = doc;
        request.uri = std::string(uri);
        (void)streamFieldVal;
        request.client = std::make_shared<HttpClientConnection>();  // TODO: ?
    }
    // if (streamFieldVal == false) {
    //     ExecutionContext executionContext{ExecutionContext::Interface::REST, ExecutionContext::Method::V3Unary};
    //     return executor->infer(&request, &response, executionContext);
    // } else {
    //     serverReaderWriter->OverwriteResponseHeader("Content-Type", "text/event-stream");
    //     serverReaderWriter->OverwriteResponseHeader("Cache-Control", "no-cache");
    //     serverReaderWriter->OverwriteResponseHeader("Connection", "keep-alive");
    //     ExecutionContext executionContext{ExecutionContext::Interface::REST, ExecutionContext::Method::V3Stream};
    //     auto status = executor->inferStream(request, *serverReaderWriter, executionContext);
    //     if (!status.ok()) {
    //         serverReaderWriter->PartialReplyWithStatus("{\"error\": \"" + status.string() + "\"}", tensorflow::serving::net_http::HTTPStatusCode::BAD_REQUEST);
    //     }
    //     serverReaderWriter->PartialReplyEnd();
    //     return StatusCode::PARTIAL_END;
    // }

    if (streamFieldVal == false) {
        std::string response;
        auto resp = drogon::HttpResponse::newHttpResponse();
        ExecutionContext executionContext{ExecutionContext::Interface::REST, ExecutionContext::Method::V3Unary};
        auto status = executor->infer(&request, &response, executionContext);
        resp->setBody(response);
        callback(resp);
        return status;
    } else {
        auto resp = drogon::HttpResponse::newAsyncStreamResponse(
            [executor, request](drogon::ResponseStreamPtr stream) {
                std::thread([
                    executor,
                    request,
                    stream = std::shared_ptr<drogon::ResponseStream>{std::move(stream)}]() mutable {
                    SPDLOG_ERROR("Hi! xxxxxxxxxxxxx");
                    // Perform your stream writing here...
                    ExecutionContext executionContext{ExecutionContext::Interface::REST, ExecutionContext::Method::V3Stream};
                    auto status = executor->inferStream(request, stream, executionContext);
                    (void)status;// TODO
                    // Close the stream after writing is done
                    stream->close();
                }).detach();
            });

        resp->addHeader("Content-Type", "text/event-stream");
        resp->addHeader("Cache-Control", "no-cache");
        resp->addHeader("Connection", "keep-alive");
        callback(resp);
        return StatusCode::OK;
    }

#else
    SPDLOG_DEBUG("Mediapipe support was disabled during build process...");
    return StatusCode::NOT_IMPLEMENTED;
#endif
    return StatusCode::OK;
}

}

