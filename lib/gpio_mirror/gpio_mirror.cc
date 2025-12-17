// Copyright 2024. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include "lib/gpio_mirror/gpio_mirror.h"

#include "pw_status/try.h"

namespace app {

pw::Status GpioMirror::Update() {
  PW_TRY_ASSIGN(const auto state, input_.GetState());
  return output_.SetState(state);
}

}  // namespace app
