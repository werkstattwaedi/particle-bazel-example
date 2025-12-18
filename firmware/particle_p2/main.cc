// Copyright 2024. All rights reserved.
// SPDX-License-Identifier: Apache-2.0
//
// GPIO Mirror Firmware for Particle P2
// Mirrors the state of input pin D0 to output pin D1.

#include "lib/gpio_mirror/gpio_mirror.h"
#include "lib/particle_gpio/particle_digital_io.h"
#include "pinmap_hal.h"

// Pin configuration (D0 and D1 are defined in pinmap_hal.h)
constexpr hal_pin_t kInputPin = D0;
constexpr hal_pin_t kOutputPin = D1;

// GPIO objects
particle::ParticleDigitalIn input(kInputPin,
                                  particle::ParticleDigitalIn::Mode::kInputPulldown);
particle::ParticleDigitalOut output(kOutputPin);

// Mirror logic
app::GpioMirror mirror(input, output);

// Particle user module entry points
extern "C" {

// Called before C++ constructors run
void module_user_init_hook() {
  // Nothing to do here
}

void setup() {
  // Ignore errors for simplicity - in production you'd want proper error handling
  (void)input.Enable();
  (void)output.Enable();
}

void loop() {
  (void)mirror.Update();
}

// Called after each loop iteration
void _post_loop() {
  // Nothing to do here
}

}  // extern "C"
