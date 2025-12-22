// Copyright 2024. All rights reserved.
// SPDX-License-Identifier: Apache-2.0
//
// GPIO Mirror Firmware for Particle P2
// Demonstrates: GPIO, pw_chrono, pw_thread, pw_sync, pw_log
//
// Features:
// - Mirrors D0 input to D1 output
// - Blinks LED on D7 every 500ms
// - Logs status every 5 seconds via USB serial

#include <chrono>

#include "lib/gpio_mirror/gpio_mirror.h"
#include "particle_digital_io.h"
#include "log_bridge.h"
#include "pinmap_hal.h"
#include "pw_chrono/system_clock.h"
#include "pw_log/log.h"
#include "pw_sync/mutex.h"
#include "pw_thread/sleep.h"

// Pin configuration
constexpr hal_pin_t kLedPin = D7;
constexpr hal_pin_t kMirrorInputPin = D0;
constexpr hal_pin_t kMirrorOutputPin = D1;

// GPIO objects using pw_digital_io abstraction
particle::ParticleDigitalOut led(kLedPin);
particle::ParticleDigitalIn mirror_input(kMirrorInputPin,
    particle::ParticleDigitalIn::Mode::kInputPulldown);
particle::ParticleDigitalOut mirror_output(kMirrorOutputPin);

// GPIO mirror logic
app::GpioMirror mirror(mirror_input, mirror_output);

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
  // Initialize log bridge to capture Device OS system logs
  particle_log::InitLogBridge();
}

void setup() {
  PW_LOG_INFO("GPIO Mirror starting up");

  // Initialize LED
  if (led.Enable().ok()) {
    PW_LOG_INFO("LED initialized on D7");
  } else {
    PW_LOG_ERROR("LED initialization failed");
  }

  // Initialize mirror GPIO pins
  if (mirror_input.Enable().ok() && mirror_output.Enable().ok()) {
    PW_LOG_INFO("Mirror GPIO initialized: D%d -> D%d", kMirrorInputPin, kMirrorOutputPin);
  } else {
    PW_LOG_ERROR("Mirror GPIO initialization failed");
  }

  auto now = pw::chrono::SystemClock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  PW_LOG_INFO("System clock at startup: %lld ms", static_cast<long long>(ms.count()));
}

void loop() {
  // Update GPIO mirror (D0 -> D1)
  // (void)mirror.Update();

  // Update loop count with mutex protection
  // g_state_mutex.lock();
  g_loop_count++;
  // g_state_mutex.unlock();

  // Get current time using pw::chrono
  auto now = pw::chrono::SystemClock::now();
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()).count();

  // Toggle LED based on time (every 500ms)
  if (now_ms - g_last_blink_time_ms >= 500) {
    g_led_state = !g_led_state;
    (void)led.SetState(g_led_state ? pw::digital_io::State::kActive
                                   : pw::digital_io::State::kInactive);
    g_last_blink_time_ms = now_ms;
  }

  // Log every 5 seconds
  if (now_ms - g_last_log_time_ms >= 5000) {
    uint32_t count = g_loop_count;

    PW_LOG_INFO("Statusx: time=%lld ms, loops=%lu (%lu/sec), led=%s",
                static_cast<long long>(now_ms),
                static_cast<unsigned long>(count),
                static_cast<unsigned long>(count / (now_ms / 1000)),
                g_led_state ? "ON" : "OFF");

    g_last_log_time_ms = now_ms;
  }

  // No sleep - run as fast as possible
}

// Called after each loop iteration
void _post_loop() {
  // Nothing to do here
}

}  // extern "C"
