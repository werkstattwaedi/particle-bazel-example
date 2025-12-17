// Copyright 2024. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include "lib/gpio_mirror/gpio_mirror.h"

#include "pw_digital_io/digital_io_mock.h"
#include "pw_unit_test/framework.h"

namespace app {
namespace {

using ::pw::digital_io::DigitalInOutMock;
using ::pw::digital_io::State;

TEST(GpioMirror, OutputFollowsInputWhenActive) {
  DigitalInOutMock<4> input;
  DigitalInOutMock<4> output;

  ASSERT_EQ(input.Enable(), pw::OkStatus());
  ASSERT_EQ(output.Enable(), pw::OkStatus());

  GpioMirror mirror(input, output);

  // Set input active
  ASSERT_EQ(input.SetStateActive(), pw::OkStatus());

  // Update mirror
  EXPECT_EQ(mirror.Update(), pw::OkStatus());

  // Verify output is active
  auto result = output.IsStateActive();
  ASSERT_TRUE(result.ok());
  EXPECT_TRUE(result.value());
}

TEST(GpioMirror, OutputFollowsInputWhenInactive) {
  DigitalInOutMock<4> input;
  DigitalInOutMock<4> output;

  ASSERT_EQ(input.Enable(), pw::OkStatus());
  ASSERT_EQ(output.Enable(), pw::OkStatus());

  GpioMirror mirror(input, output);

  // Set input inactive (this is the default, but be explicit)
  ASSERT_EQ(input.SetStateInactive(), pw::OkStatus());

  // Update mirror
  EXPECT_EQ(mirror.Update(), pw::OkStatus());

  // Verify output is inactive
  auto result = output.IsStateActive();
  ASSERT_TRUE(result.ok());
  EXPECT_FALSE(result.value());
}

TEST(GpioMirror, OutputTogglesWithInput) {
  DigitalInOutMock<8> input;
  DigitalInOutMock<8> output;

  ASSERT_EQ(input.Enable(), pw::OkStatus());
  ASSERT_EQ(output.Enable(), pw::OkStatus());

  GpioMirror mirror(input, output);

  // Toggle several times
  for (int i = 0; i < 3; ++i) {
    // Active
    ASSERT_EQ(input.SetStateActive(), pw::OkStatus());
    EXPECT_EQ(mirror.Update(), pw::OkStatus());
    EXPECT_TRUE(output.IsStateActive().value());

    // Inactive
    ASSERT_EQ(input.SetStateInactive(), pw::OkStatus());
    EXPECT_EQ(mirror.Update(), pw::OkStatus());
    EXPECT_FALSE(output.IsStateActive().value());
  }
}

}  // namespace
}  // namespace app
