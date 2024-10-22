//*****************************************************************************
// Copyright 2024 Intel Corporation
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

#include <memory>
#include <string>
#include <thread>
#include <utility>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "logging.hpp"
#include "modelmanager.hpp"
#include "status.hpp"

#if (MEDIAPIPE_DISABLE == 0)
#include "http_payload.hpp"
#include "http_frontend/drogon_graph_executor_impl.hpp"
#include "http_frontend/http_client_connection.hpp"
#include "mediapipe_internal/mediapipegraphexecutor.hpp"
#endif

using rapidjson::Document;
using rapidjson::SizeType;
using rapidjson::Value;

namespace ovms {

Status processDrogonV3(ModelManager* mm, const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    SPDLOG_ERROR("Received request for server side event");

#if (MEDIAPIPE_DISABLE == 0)
    std::string request_body = std::string(req->getBody());
    std::string uri = req->getPath();
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
        // request.headers = request_components.headers;  // TODO: Not needed
        request.body = request_body;
        request.parsedJson = doc;
        request.uri = std::string(uri);
        request.client = std::make_shared<HttpClientConnection>();  // TODO: Not needed
    }
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
                std::thread([executor,
                                request,
                                stream = std::shared_ptr<drogon::ResponseStream>{std::move(stream)}]() mutable {
                    ExecutionContext executionContext{ExecutionContext::Interface::REST, ExecutionContext::Method::V3Stream};
                    auto status = executor->inferStream(request, stream, executionContext);
                    stream->close();
                })
                    .detach();
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

}  // namespace ovms
