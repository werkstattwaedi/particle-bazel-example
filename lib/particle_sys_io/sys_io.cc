// Copyright 2024 Werkstatt Waedi
// SPDX-License-Identifier: Apache-2.0
//
// pw_sys_io backend for Particle Device OS using Serial1 (UART)

#include "pw_sys_io/sys_io.h"

#include "usart_hal.h"

namespace {

constexpr hal_usart_interface_t kSerial = HAL_USART_SERIAL1;
constexpr uint32_t kBaudRate = 115200;

bool g_initialized = false;

void EnsureInitialized() {
  if (!g_initialized) {
    hal_usart_init(kSerial, nullptr, nullptr);
    hal_usart_begin(kSerial, kBaudRate);
    g_initialized = true;
  }
}

}  // namespace

namespace pw::sys_io {

Status ReadByte(std::byte* dest) {
  EnsureInitialized();

  // Block until data available
  while (hal_usart_available(kSerial) <= 0) {
    // Busy wait
  }

  int32_t data = hal_usart_read(kSerial);
  if (data < 0) {
    return Status::ResourceExhausted();
  }

  *dest = static_cast<std::byte>(data);
  return OkStatus();
}

Status TryReadByte(std::byte* dest) {
  EnsureInitialized();

  if (hal_usart_available(kSerial) <= 0) {
    return Status::Unavailable();
  }

  int32_t data = hal_usart_read(kSerial);
  if (data < 0) {
    return Status::ResourceExhausted();
  }

  *dest = static_cast<std::byte>(data);
  return OkStatus();
}

Status WriteByte(std::byte b) {
  EnsureInitialized();

  // Block until space available
  while (hal_usart_available_data_for_write(kSerial) <= 0) {
    // Busy wait
  }

  hal_usart_write(kSerial, static_cast<uint8_t>(b));
  return OkStatus();
}

StatusWithSize WriteLine(std::string_view s) {
  size_t bytes_written = 0;

  for (char c : s) {
    if (Status status = WriteByte(static_cast<std::byte>(c)); !status.ok()) {
      return StatusWithSize(status, bytes_written);
    }
    bytes_written++;
  }

  // Write newline
  if (Status status = WriteByte(static_cast<std::byte>('\r')); !status.ok()) {
    return StatusWithSize(status, bytes_written);
  }
  bytes_written++;

  if (Status status = WriteByte(static_cast<std::byte>('\n')); !status.ok()) {
    return StatusWithSize(status, bytes_written);
  }
  bytes_written++;

  return StatusWithSize(bytes_written);
}

}  // namespace pw::sys_io
