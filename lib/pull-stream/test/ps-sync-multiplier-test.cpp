#include <doctest.h>

#include <pull/stream.h>

TEST_CASE("[pull/stream/redux] multiplier through stream") {
  // expect error when not hooked up
  {
    ps_t *mult = create_multiplier(10);
    // call fn without setting up the source
    ps_val_t *v = ps_pull(mult, PS_OK);
    REQUIRE(v == nullptr);
    ps_destroy(mult);
  }

  // wire to counter source
  {
    ps_t *counter = create_counter();
    ps_t *mult = create_multiplier(10);

    mult->source = counter;

    for (uint64_t i=0; i<10; i++) {
      ps_val_t *v = ps_pull(mult, PS_OK);
      REQUIRE(v != nullptr);
      REQUIRE(v->data != nullptr);

      uint64_t iv = *((uint64_t *)v->data);
      CHECK(iv == (i * 10));
      ps_destroy(v);
    }

    ps_destroy(counter);
    ps_destroy(mult);
  }

  // error propagates
  {
    ps_t *counter = create_counter();
    ps_t *mult = create_multiplier(10);

    mult->source = counter;

    ps_status(mult, PS_ERR);

    CHECK(counter->status == PS_ERR);
    CHECK(mult->status == PS_ERR);

    ps_destroy(counter);
    ps_destroy(mult);
  }
}
