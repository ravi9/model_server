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
#include <string>

#pragma warning(push)
#pragma warning(disable : 4005 4309 6001 6385 6386 6326 6011 6246 4456 6246)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/canonical_errors.h"
#pragma GCC diagnostic pop
#pragma warning(pop)

#include "../http_payload.hpp"
#include "../profiler.hpp"
#include "../logging.hpp"

using namespace ovms;

namespace mediapipe {

class CreateImageVariationCalculator : public CalculatorBase {
    static const std::string INPUT_TAG_NAME;
    static const std::string OUTPUT_TAG_NAME;
    //static const std::string LOOPBACK_TAG_NAME;

public:
    static absl::Status GetContract(CalculatorContract* cc) {
        RET_CHECK(!cc->Inputs().GetTags().empty());
        RET_CHECK(!cc->Outputs().GetTags().empty());
        cc->Inputs().Tag(INPUT_TAG_NAME).Set<ovms::HttpPayload>();
        //cc->Inputs().Tag(LOOPBACK_TAG_NAME).Set<bool>();
        cc->Outputs().Tag(OUTPUT_TAG_NAME).Set<std::string>();
        //cc->Outputs().Tag(LOOPBACK_TAG_NAME).Set<bool>();
        return absl::OkStatus();
    }

    absl::Status Close(CalculatorContext* cc) final {
        OVMS_PROFILE_FUNCTION();
        SPDLOG_LOGGER_DEBUG(llm_calculator_logger, "LLMCalculator [Node: {} ] Close", cc->NodeName());
        return absl::OkStatus();
    }

    absl::Status Open(CalculatorContext* cc) final {
        OVMS_PROFILE_FUNCTION();
        SPDLOG_LOGGER_DEBUG(llm_calculator_logger, "LLMCalculator  [Node: {}] Open start", cc->NodeName());
        SPDLOG_LOGGER_DEBUG(llm_calculator_logger, "LLMCalculator [Node: {}] Open end", cc->NodeName());
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext* cc) final {
        SPDLOG_LOGGER_DEBUG(llm_calculator_logger, "LLMCalculator  [Node: {}] Process start", cc->NodeName());
        OVMS_PROFILE_FUNCTION();

        if (!cc->Inputs().Tag(INPUT_TAG_NAME).IsEmpty()) {
            // get payload
            const auto& payload = cc->Inputs().Tag(INPUT_TAG_NAME).Get<ovms::HttpPayload>();
            SPDLOG_LOGGER_DEBUG(llm_calculator_logger, "LLMCalculator  [Node: {}] Process payload: {}", cc->NodeName(), payload.body);

            // read all files
            SPDLOG_LOGGER_DEBUG(llm_calculator_logger, "Number of files: {}", payload.client->getNumberOfFiles());
            SPDLOG_LOGGER_DEBUG(llm_calculator_logger, "File content: [{}]", payload.client->getFileContent(0));
            SPDLOG_LOGGER_DEBUG(llm_calculator_logger, "Param size: [{}]", payload.client->getMultiPartField("size"));

            cc->Outputs().Tag(OUTPUT_TAG_NAME).Add(new std::string{R"(
                {
                    "status": "ok",
                    "message": "Image variation created"
                }
                
                )"}, cc->InputTimestamp());
        }

        SPDLOG_LOGGER_DEBUG(llm_calculator_logger, "LLMCalculator  [Node: {}] Process end", cc->NodeName());
        return absl::OkStatus();
    }
};

const std::string CreateImageVariationCalculator::INPUT_TAG_NAME{"HTTP_REQUEST_PAYLOAD"};
const std::string CreateImageVariationCalculator::OUTPUT_TAG_NAME{"HTTP_RESPONSE_PAYLOAD"};
//const std::string CreateImageVariationCalculator::LOOPBACK_TAG_NAME{"LOOPBACK"};

REGISTER_CALCULATOR(CreateImageVariationCalculator);

}  // namespace mediapipe
