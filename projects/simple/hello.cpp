#include <stdio.h>

#include <cimgui.h>

#include <vulkan/vulkan.h>

void setup() {}
void loop() {
  bool show_demo_window = true;
  // igShowDemoWindow(NULL);
  igBegin("simple window", 0, 0);
    igTextUnformatted("hello!", NULL);
  igEnd();
}
