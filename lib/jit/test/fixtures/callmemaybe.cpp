#include <stdint.h>

extern "C" {
  void callmemaybe(uint32_t);
}

void setup() {
  callmemaybe(0xFEED);
}

void loop() {
  callmemaybe(0xF00D);
}