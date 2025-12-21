// Copyright 2024. All rights reserved.
// SPDX-License-Identifier: Apache-2.0
//
// GPIO Mirror Firmware for Particle P2
// Demonstrates: GPIO, pw_chrono, pw_thread, pw_sync, pw_log

#include <chrono>

#include "lib/gpio_mirror/gpio_mirror.h"
#include "lib/particle_gpio/particle_digital_io.h"
#include "pinmap_hal.h"
#include "pw_chrono/system_clock.h"
#include "pw_log/log.h"
#include "pw_sync/mutex.h"
#include "pw_thread/sleep.h"

// Pin configuration (D0 and D1 are defined in pinmap_hal.h)
constexpr hal_pin_t kInputPin = D0;
constexpr hal_pin_t kOutputPin = D1;

// GPIO objects
particle::ParticleDigitalIn input(kInputPin,
                                  particle::ParticleDigitalIn::Mode::kInputPulldown);
particle::ParticleDigitalOut output(kOutputPin);

// Mirror logic
app::GpioMirror mirror(input, output);

// Shared state protected by mutex
pw::sync::Mutex g_state_mutex;
uint32_t g_loop_count = 0;
int64_t g_last_log_time_ms = 0;

// Particle user module entry points
extern "C" {

// Called before C++ constructors run
void module_user_init_hook() {
  // Nothing to do here
}

void setup() {
  PW_LOG_INFO("GPIO Mirror starting up");

  // Log the initial time
  auto now = pw::chrono::SystemClock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  PW_LOG_INFO("System clock at startup: %lld ms", static_cast<long long>(ms.count()));

  // Initialize GPIO
  if (input.Enable().ok() && output.Enable().ok()) {
    PW_LOG_INFO("GPIO initialized: input=D%d, output=D%d", kInputPin, kOutputPin);
  } else {
    PW_LOG_ERROR("GPIO initialization failed");
  }
}

void loop() {
  // Update GPIO mirror
  (void)mirror.Update();

  // Protect shared state with mutex
  g_state_mutex.lock();
  g_loop_count++;
  g_state_mutex.unlock();

  // Get current time
  auto now = pw::chrono::SystemClock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()).count();

  // Log every 5 seconds
  if (now_ms - g_last_log_time_ms >= 5000) {
    g_state_mutex.lock();
    uint32_t count = g_loop_count;
    g_state_mutex.unlock();

    PW_LOG_INFO("Status: time=%lld ms, loops=%lu",
                static_cast<long long>(now_ms),
                static_cast<unsigned long>(count));

    g_last_log_time_ms = now_ms;
  }

  // Sleep for 10ms to avoid busy-looping
  pw::this_thread::sleep_for(std::chrono::milliseconds(10));
}

// Called after each loop iteration
void _post_loop() {
  // Nothing to do here
}

}  // extern "C"
