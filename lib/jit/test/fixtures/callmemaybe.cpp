

extern "C" {
  void callmemaybe(unsigned int);
}

void setup() {
  callmemaybe(0xFEED);
}

void loop() {
  callmemaybe(0xF00D);
}