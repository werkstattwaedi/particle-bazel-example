// Copyright 2024. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pw_digital_io/digital_io.h"
#include "pw_status/status.h"

namespace app {

// Platform-agnostic GPIO mirror that copies input state to output.
// Call Update() periodically to synchronize the output with the input.
class GpioMirror {
 public:
  GpioMirror(pw::digital_io::DigitalIn& input,
             pw::digital_io::DigitalOut& output)
      : input_(input), output_(output) {}

  // Reads input and sets output to match. Returns error if either fails.
  pw::Status Update();

 private:
  pw::digital_io::DigitalIn& input_;
  pw::digital_io::DigitalOut& output_;
};

}  // namespace app
