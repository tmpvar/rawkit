#include <rawkit/rawkit.h>

#include <stdio.h>

class Hello {
  public:
    Hello() {
      printf("hello ctor\n");
    }

    ~Hello() {
      printf("hello dtor\n");
    }

};

Hello *h = nullptr;

void setup() {

  h = new Hello();

  delete h;
}
void loop() {}