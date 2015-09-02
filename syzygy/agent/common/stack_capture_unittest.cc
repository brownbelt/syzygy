// Copyright 2013 Google Inc. All Rights Reserved.
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

#include "syzygy/agent/common/stack_capture.h"

#include "base/memory/scoped_ptr.h"
#include "gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace agent {
namespace common {

namespace {

using ::testing::_;

class TestStackCapture : public StackCapture {
 public:
  TestStackCapture() {}

  void set_relative_stack_id(StackId relative_stack_id) {
    relative_stack_id_ = relative_stack_id;
  }

  MOCK_CONST_METHOD0(ComputeRelativeStackId, StackId());
};

class StackCaptureTest : public testing::Test {
 public:
  void SetUp() override {
    // Setup the "global" state.
    StackCapture::Init();
  }
};

}  // namespace

TEST_F(StackCaptureTest, InitFromBuffer) {
  StackCapture capture;

  // Validate the capture's initial state.
  EXPECT_FALSE(capture.IsValid());
  EXPECT_EQ(0u, capture.absolute_stack_id());
  EXPECT_EQ(0, capture.num_frames());
  EXPECT_EQ(StackCapture::kMaxNumFrames, capture.max_num_frames());
  EXPECT_TRUE(capture.frames() != NULL);

  // Create some fake stack trace data.
  ULONG absolute_stack_id = 10;
  void* frames[StackCapture::kMaxNumFrames + 1] = { 0 };
  for (size_t i = 0; i < arraysize(frames); ++i) {
    frames[i] = reinterpret_cast<void*>(i);
  }

  // Initialize the stack capture without using all of the frames.
  capture.InitFromBuffer(absolute_stack_id, frames, 7);
  EXPECT_TRUE(capture.IsValid());
  EXPECT_EQ(10u, capture.absolute_stack_id());
  EXPECT_EQ(7, capture.num_frames());
  EXPECT_EQ(StackCapture::kMaxNumFrames, capture.max_num_frames());
  EXPECT_TRUE(capture.frames() != NULL);

  // Attempt to initialize the stack capture using too many frames; the
  // resulting capture should truncate to kMaxNumFrames.
  capture.InitFromBuffer(absolute_stack_id, frames, arraysize(frames));
  EXPECT_TRUE(capture.IsValid());
  EXPECT_EQ(StackCapture::kMaxNumFrames, capture.num_frames());
  EXPECT_EQ(StackCapture::kMaxNumFrames, capture.max_num_frames());
  EXPECT_TRUE(capture.frames() != NULL);
}

TEST_F(StackCaptureTest, InitFromStack) {
  StackCapture capture;

  EXPECT_FALSE(capture.IsValid());
  EXPECT_EQ(0u, capture.absolute_stack_id());
  EXPECT_EQ(0, capture.num_frames());
  EXPECT_EQ(StackCapture::kMaxNumFrames, capture.max_num_frames());

  capture.InitFromStack();
  EXPECT_TRUE(capture.IsValid());
  EXPECT_LT(0u, capture.num_frames());
  EXPECT_EQ(StackCapture::kMaxNumFrames, capture.max_num_frames());
}

TEST_F(StackCaptureTest, RestrictedFrameCount) {
  StackCapture::set_bottom_frames_to_skip(0);
  // Restrict this to a stack depth that is smaller than the stack depth of
  // this test.
  StackCapture capture(5);
  EXPECT_FALSE(capture.IsValid());
  EXPECT_EQ(0u, capture.absolute_stack_id());
  EXPECT_EQ(0, capture.num_frames());
  EXPECT_EQ(5u, capture.max_num_frames());

  capture.InitFromStack();
  EXPECT_TRUE(capture.IsValid());
  EXPECT_EQ(5u, capture.num_frames());
  EXPECT_EQ(5u, capture.max_num_frames());
}

TEST_F(StackCaptureTest, RelativeStackId) {
  TestStackCapture test_stack_capture;

  // Expect one callback when calling relative_stack_id the first time.
  EXPECT_CALL(test_stack_capture, ComputeRelativeStackId());
  EXPECT_EQ(0U, test_stack_capture.relative_stack_id());

  // Needed since ComputeRelativeStackId is mocked and makes sures that we don't
  // end up with a valid id that is 0 (which would trigger flakiness).
  test_stack_capture.set_relative_stack_id(123456U);

  // Should not trigger call to ComputeRelativeStackId.
  EXPECT_EQ(123456U, test_stack_capture.relative_stack_id());
}

}  // namespace common
}  // namespace agent
