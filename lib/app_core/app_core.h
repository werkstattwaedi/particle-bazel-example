#pragma once

#include "pw_result/result.h"
#include "pw_status/status.h"

namespace app {

// Simple example function to verify Pigweed integration
pw::Result<int> Add(int a, int b);

pw::Status Initialize();

}  // namespace app
