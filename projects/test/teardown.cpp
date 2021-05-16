#include <rawkit/rawkit.h>

void setup() {
  rawkit_teardown_fn(nullptr, [](void *){
    printf("teardown!\n");
  });
}

void loop() {

}