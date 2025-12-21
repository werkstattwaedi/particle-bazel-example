// Copyright 2024. All rights reserved.
// SPDX-License-Identifier: Apache-2.0
//
// LED Blink Firmware for Particle P2
// Demonstrates: GPIO, pw_chrono, pw_thread, pw_sync, pw_log

#include <chrono>

#include "lib/particle_gpio/particle_digital_io.h"
#include "pinmap_hal.h"
#include "pw_chrono/system_clock.h"
#include "pw_log/log.h"
#include "pw_sync/mutex.h"
#include "pw_thread/sleep.h"

// D7 is the onboard user LED on Particle devices
constexpr hal_pin_t kLedPin = D7;

// LED GPIO object using pw_digital_io abstraction
particle::ParticleDigitalOut led(kLedPin);

// Shared state protected by mutex
pw::sync::Mutex g_state_mutex;
uint32_t g_loop_count = 0;
int64_t g_last_log_time_ms = 0;
int64_t g_last_blink_time_ms = 0;
bool g_led_state = false;

// Particle user module entry points
extern "C" {

// Called before C++ constructors run
void module_user_init_hook() {
  // Nothing to do here
}

void setup() {
  PW_LOG_INFO("LED Blink starting up");

  // Initialize LED using GPIO abstraction
  if (led.Enable().ok()) {
    PW_LOG_INFO("LED initialized on D7");
  } else {
    PW_LOG_ERROR("LED initialization failed");
  }

  auto now = pw::chrono::SystemClock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  PW_LOG_INFO("System clock at startup: %lld ms", static_cast<long long>(ms.count()));
}

void loop() {
  // Toggle LED using GPIO abstraction
  g_led_state = !g_led_state;
  (void)led.SetState(g_led_state ? pw::digital_io::State::kActive
                                 : pw::digital_io::State::kInactive);

  // Update loop count with mutex protection
  g_state_mutex.lock();
  g_loop_count++;
  g_state_mutex.unlock();

  // Get current time using pw::chrono
  auto now = pw::chrono::SystemClock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()).count();

  // Log every 5 seconds
  if (now_ms - g_last_log_time_ms >= 5000) {
    g_state_mutex.lock();
    uint32_t count = g_loop_count;
    g_state_mutex.unlock();

    PW_LOG_INFO("Status: time=%lld ms, loops=%lu, led=%s",
                static_cast<long long>(now_ms),
                static_cast<unsigned long>(count),
                g_led_state ? "ON" : "OFF");

    g_last_log_time_ms = now_ms;
  }

  // Use pw::this_thread::sleep_for
  pw::this_thread::sleep_for(std::chrono::milliseconds(500));
}

// Called after each loop iteration
void _post_loop() {
  // Nothing to do here
}

}  // extern "C"
