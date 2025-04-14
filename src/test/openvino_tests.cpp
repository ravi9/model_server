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
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <openvino/core/type/element_type.hpp>
#include <openvino/openvino.hpp>

#include "../ov_utils.hpp"
#include "../ovms.h"  // NOLINT
#include "c_api_test_utils.hpp"
#include "test_utils.hpp"

using namespace ov;

class OpenVINO : public ::testing::Test {
};

TEST_F(OpenVINO, TensorCopyDoesNotCopyUnderlyingData) {
    std::vector<float> data{1, 2, 3};
    ov::Shape shape{3};
    ov::Tensor t(ov::element::f32, shape, data.data());
    ov::Tensor t2(ov::element::f32, shape);
    EXPECT_NE(t2.data(), t.data());
    t2 = t;
    EXPECT_EQ(t2.data(), t.data());
}

TEST_F(OpenVINO, String) {
    std::vector<std::string> data{{"Intel"}, {"CCGafawfaw"}, {"aba"}};
    ov::Shape shape{3};
    ov::Tensor t(ov::element::string, shape, data.data());
    EXPECT_EQ(t.get_byte_size(), data.size() * sizeof(std::string));
}
TEST_F(OpenVINO, CallbacksTest) {
    Core core;
    auto model = core.read_model("/ovms/src/test/dummy/1/dummy.xml");
    const std::string inputName{"b"};
    auto input = model->get_parameters().at(0);
    ov::element::Type_t dtype = ov::element::Type_t::f32;
    ov::Shape ovShape;
    ovShape.emplace_back(1);
    ovShape.emplace_back(10000);
    std::map<std::string, ov::PartialShape> inputShapes;
    inputShapes[inputName] = ovShape;
    model->reshape(inputShapes);
    auto cpuCompiledModel = core.compile_model(model, "CPU");
    auto cpuInferRequest = cpuCompiledModel.create_infer_request();
    // prepare ov::Tensor data
    std::vector<ov::Tensor> inputOvTensors, outputOvTensors;
    inputOvTensors.emplace_back(dtype, ovShape);
    outputOvTensors.emplace_back(dtype, ovShape);
    cpuInferRequest.set_tensor(inputName, inputOvTensors[0]);

    uint32_t callbackUsed = 31;
    OVMS_InferenceResponse* response{nullptr};
    cpuInferRequest.set_callback([&cpuInferRequest, &response, &callbackUsed](std::exception_ptr exception) {
        if (exception) {
            try {
                std::rethrow_exception(exception);
            } catch (const std::exception& e) {
                std::cout << "Caught exception: '" << e.what() << "'\n";
            } catch (...) {
                return;
            }
        }
        SPDLOG_INFO("Using OV callback");
        // here will go OVMS C-API serialization code
        callbackMarkingItWasUsedWith42(response, 1, reinterpret_cast<void*>(&callbackUsed));
        cpuInferRequest.set_callback([](std::exception_ptr exception) {});
    });
    bool callbackCalled{false};
    cpuInferRequest.start_async();
    EXPECT_FALSE(callbackCalled);
    cpuInferRequest.wait();
    ov::Tensor outOvTensor = cpuInferRequest.get_tensor("a");
    auto outAutoTensor = cpuInferRequest.get_tensor("a");
    EXPECT_TRUE(outOvTensor.is<ov::Tensor>());
    EXPECT_TRUE(outAutoTensor.is<ov::Tensor>());
}
TEST_F(OpenVINO, ResetOutputTensors) {
    Core core;
    auto model = core.read_model("/ovms/src/test/dummy/1/dummy.xml");
    ov::AnyMap config = {ov::hint::performance_mode(ov::hint::PerformanceMode::THROUGHPUT),
        ov::auto_batch_timeout(0)};
    auto compiledModel = core.compile_model(model, "CPU", config);
    std::vector<float> in1(DUMMY_MODEL_INPUT_SIZE, 0.1);
    ov::Shape shape;
    shape.emplace_back(1);
    shape.emplace_back(DUMMY_MODEL_INPUT_SIZE);
    ov::element::Type_t dtype = ov::element::Type_t::f32;
    ov::Tensor input1(dtype, shape, in1.data());
    auto inferRequest = compiledModel.create_infer_request();
    inferRequest.set_tensor(DUMMY_MODEL_INPUT_NAME, input1);
    // keep original output tensor
    ov::Tensor originalOutput = inferRequest.get_tensor(DUMMY_MODEL_OUTPUT_NAME);
    // set output
    std::vector<float> out(DUMMY_MODEL_INPUT_SIZE, 15124.1);
    ov::Tensor output(dtype, shape, out.data());
    inferRequest.set_tensor(DUMMY_MODEL_OUTPUT_NAME, output);
    inferRequest.infer();
    for (size_t i = 0; i < DUMMY_MODEL_INPUT_SIZE; ++i) {
        EXPECT_NEAR(in1[i] + 1, out[i], 0.0004) << "i:" << i;
    }
    std::vector<float> in2(DUMMY_MODEL_INPUT_SIZE, 42);
    ov::Tensor input2(dtype, shape, in2.data());
    inferRequest.set_tensor(DUMMY_MODEL_INPUT_NAME, input2);
    inferRequest.set_tensor(DUMMY_MODEL_OUTPUT_NAME, originalOutput);
    inferRequest.infer();
    auto secondOutput = inferRequest.get_tensor(DUMMY_MODEL_OUTPUT_NAME);
    float* data2nd = reinterpret_cast<float*>(secondOutput.data());
    for (size_t i = 1; i < DUMMY_MODEL_INPUT_SIZE; ++i) {
        EXPECT_NEAR(in2[i] + 1, data2nd[i], 0.0004) << "i:" << i;
    }
    // now check if first output didn't change content
    for (size_t i = 0; i < DUMMY_MODEL_INPUT_SIZE; ++i) {
        EXPECT_NEAR(in1[i] + 1, out[i], 0.0004) << "i:" << i;
    }
}
TEST_F(OpenVINO, RerankModel) {
    Core core;
    auto model = core.read_model("/ovms/models/models/BAAI/bge-reranker-large/rerank/1/model.xml");
    ov::AnyMap config = {ov::hint::performance_mode(ov::hint::PerformanceMode::THROUGHPUT),
        ov::auto_batch_timeout(0)};
    auto compiledModel = core.compile_model(model, "CPU", config);
    /*
input_ids shape: 2x8,
0,33600,31,2,2,81907,2,1,,0,33600,31,2,2,4889,19256,2,,
attention_mask shape: 2x8,
1,1,1,1,1,1,1,0,,1,1,1,1,1,1,1,1
     */
    size_t batch_size = 2;
    size_t total_tokens_count_per_batch = 8;
std::vector<int64_t> inpIdsData{0,33600,31,2,2,81907,2,1/*,*/,0,33600,31,2,2,4889,19256,2};
std::vector<int64_t> atMaskData{1,1,1,1,1,1,1,0/*,*/,1,1,1,1,1,1,1,1};
    auto input_ids = ov::Tensor(ov::element::i64, ov::Shape{batch_size, total_tokens_count_per_batch});
    auto attention_mask = ov::Tensor(ov::element::i64, ov::Shape{batch_size, total_tokens_count_per_batch});
	std::memcpy(input_ids.data(), inpIdsData.data(), batch_size * total_tokens_count_per_batch * sizeof(int64_t));
	std::memcpy(attention_mask.data(), atMaskData.data(), batch_size * total_tokens_count_per_batch * sizeof(int64_t));
    auto inferRequest = compiledModel.create_infer_request();
    inferRequest.set_tensor("input_ids", input_ids);
    inferRequest.set_tensor("attention_mask", attention_mask);
    inferRequest.infer();
    ov::Tensor output = inferRequest.get_tensor("logits");
    // set output
		size_t i =0;
		int elemCount = output.get_size();
    for (const auto& shape : output.get_shape()) {
        SPDLOG_ERROR("Sh[{}]={}", i, shape);
++i;
    }
		for (int i = 0; i < elemCount; ++i) {
				float* val = reinterpret_cast<float*>(output.data()) + i;
         SPDLOG_ERROR("Sh[{}]={}", i, *val);
}
    inferRequest.infer();
    output = inferRequest.get_tensor("logits");
		i =0;
    for (const auto& shape : output.get_shape()) {
        SPDLOG_ERROR("Sh[{}]={}", i, shape);
++i;
    }
		for (int i = 0; i < elemCount; ++i) {
				float* val = reinterpret_cast<float*>(output.data()) + i;
         SPDLOG_ERROR("Sh[{}]={}", i, *val);
}
    
}
