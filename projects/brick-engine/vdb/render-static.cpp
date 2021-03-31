#define CPU_HOST

#include <rawkit/rawkit.h>
#include "nanovdb/util/Primitives.h"
#include "nanovdb/util/IO.h"

#include "prof.h"

struct Tree {
  rawkit_ssbo_t *tiles;
};

struct State {
  Tree tree;
};


void setup() {
  State *state = rawkit_hot_state("state", state);

  try {
    Prof build("build");
    auto handle = nanovdb::createPointSphere<float>(1, 100.0f);
    auto* grid = handle.grid<float>(); // get a (raw) pointer to a NanoVDB grid of value type float

    build.finish();
    for (int i = -103; i < -96; ++i) {
        printf("(%3i,0,0) NanoVDB cpu: % -4.2f\n", i, acc.getValue(nanovdb::Coord(i, 0, 0)));
    }
  }
  catch (const std::exception& e) {
      std::cerr << "An exception occurred: \"" << e.what() << "\"" << std::endl;
  }

}

void loop() {}