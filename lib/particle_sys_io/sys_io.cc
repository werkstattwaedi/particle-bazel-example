// Copyright 2024 Werkstatt Waedi
// SPDX-License-Identifier: Apache-2.0
//
// pw_sys_io backend for Particle Device OS using USB CDC Serial
// This enables logging via `particle serial monitor`

#include "pw_sys_io/sys_io.h"

#include "usb_hal.h"

namespace {

constexpr HAL_USB_USART_Serial kSerial = HAL_USB_USART_SERIAL;
constexpr uint32_t kBaudRate = 115200;

bool g_initialized = false;

void EnsureInitialized() {
  if (!g_initialized) {
    HAL_USB_USART_Init(kSerial, nullptr);
    HAL_USB_USART_Begin(kSerial, kBaudRate, nullptr);
    g_initialized = true;
  }
}

}  // namespace

namespace pw::sys_io {

Status ReadByte(std::byte* dest) {
  EnsureInitialized();

  // Block until data available
  while (HAL_USB_USART_Available_Data(kSerial) <= 0) {
    // Busy wait
  }

  int32_t data = HAL_USB_USART_Receive_Data(kSerial, 0);
  if (data < 0) {
    return Status::ResourceExhausted();
  }

  *dest = static_cast<std::byte>(data);
  return OkStatus();
}

Status TryReadByte(std::byte* dest) {
  EnsureInitialized();

  if (HAL_USB_USART_Available_Data(kSerial) <= 0) {
    return Status::Unavailable();
  }

  int32_t data = HAL_USB_USART_Receive_Data(kSerial, 0);
  if (data < 0) {
    return Status::ResourceExhausted();
  }

  *dest = static_cast<std::byte>(data);
  return OkStatus();
}

Status WriteByte(std::byte b) {
  EnsureInitialized();

  // Send via USB CDC - HAL handles buffering
  HAL_USB_USART_Send_Data(kSerial, static_cast<uint8_t>(b));
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
