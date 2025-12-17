// Simple test to verify Particle SDK headers compile

#include "hal_platform.h"

// Just verify basic platform defines are set
static_assert(PLATFORM_ID == 32, "Expected P2 platform");
