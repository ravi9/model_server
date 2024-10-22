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
#include "drogon_graph_executor_impl.hpp"

#include <exception>
#include <string>
#include <utility>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "tensorflow_serving/util/net_http/server/public/server_request_interface.h"
#pragma GCC diagnostic pop
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "mediapipe/framework/calculator_graph.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#pragma GCC diagnostic pop
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#include "mediapipe/framework/formats/tensor.h"
#include "mediapipe/framework/port/status.h"
#pragma GCC diagnostic pop

#include "../mediapipe_internal/mediapipe_utils.hpp"

namespace ovms {

Status onPacketReadySerializeAndSendImpl(
    const std::string& requestId,
    const std::string& endpointName,
    const std::string& endpointVersion,
    const std::string& packetName,
    const mediapipe_packet_type_enum packetType,
    const ::mediapipe::Packet& packet,
    HttpReaderWriter& serverReaderWriter) {
    std::string out;
    OVMS_RETURN_ON_FAIL(
        onPacketReadySerializeImpl(
            requestId,
            endpointName,
            endpointVersion,
            packetName,
            packetType,
            packet,
            out));
    if (serverReaderWriter->send(out)) {
        return StatusCode::OK;
    } else {
        throw std::logic_error("disconnected");
        return StatusCode::INTERNAL_ERROR;
    }
}

Status sendErrorImpl(
    const std::string& message,
    HttpReaderWriter& serverReaderWriter) {
    serverReaderWriter->send("{\"error\": \"" + message + "\"}");
    return StatusCode::OK;
}

bool waitForNewRequest(
    HttpReaderWriter& serverReaderWriter,
    HttpPayload& newRequest) {
    return false;
}

}  // namespace ovms
