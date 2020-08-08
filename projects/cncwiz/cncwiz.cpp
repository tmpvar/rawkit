#include <cimgui.h>

#include <hot/state.h>

#include <rawkit/serial.h>

#include <rawkit/string.h>

void setup() {}

void loop() {
  SerialPort sp("COM3");
  igBegin("hot", 0, 0);

  igText("sp: %u, %u", sp.id, sp.available());
  String *rx = (String *)hotState(
    /* id            */ 1,
    /* size          */ sizeof(String),
    /* initial value */ nullptr
  );

  while (sp.available()) {
    char c = sp.read();
    printf("recv: %c\n", c);
    rx->append(c);
  }
  uint32_t *ticker = (uint32_t *) hotState(
  /* id            */  2,
  /* size          */  sizeof(uint32_t),
  /* initial value */  0
  );
  
  igText("it's hot in here: %i sp: %u", (*ticker)++);
  igText("recv buffer\n%s", rx->handle);
}
