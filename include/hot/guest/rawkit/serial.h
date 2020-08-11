#pragma once

#include <stdint.h>
#include <string.h>
typedef int32_t SerialID;

extern "C" {
  SerialID Serial_Open(const char *port);
  size_t Serial_Available(SerialID id);
  int16_t Serial_Read(SerialID id);
  void Serial_Write(SerialID id, const uint8_t *buf, size_t len);
}

struct SerialPort {
  SerialID id;
  SerialPort(const char *port);
  SerialPort() {}
  void open(const char *port) {
    this->id = Serial_Open(port);
  }
  size_t available();
  // we use int16 here because we want the full lower 8 bits
  // but also want to be able to signal an error using -1
  int16_t read();
  // size_t write(uint8_t byte);
  void write(const char *str) {
    Serial_Write(this->id, (const uint8_t *)str, strlen(str));
  }

  void write(const uint8_t *buf, size_t len) {
    Serial_Write(this->id, buf, len);
  }
};

SerialPort::SerialPort(const char *port) {
  this->id = Serial_Open(port);
}

size_t SerialPort::available() {
  return Serial_Available(this->id);
}

int16_t SerialPort::read() {
  return Serial_Read(this->id);
}

