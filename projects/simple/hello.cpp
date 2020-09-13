#include <stdio.h>

#include <cimgui.h>

void setup() {}
void loop() {
  bool show_demo_window = true;
  igShowDemoWindow(&show_demo_window);
  igBegin("simple window", 0, 0);
    igTextUnformatted("hello!", NULL);
  igEnd();
}
