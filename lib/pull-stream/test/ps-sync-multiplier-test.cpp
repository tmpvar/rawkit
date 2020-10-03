#include <doctest.h>

#include <pull/stream.h>

TEST_CASE("[pull/stream/redux] multiplier through stream") {
  // expect error when not hooked up
  {
    ps_cb_t *mult = create_multiplier(10);
    // call fn without setting up the source
    ps_value_t *v = mult->fn(mult, PS_OK);
    REQUIRE(v == nullptr);
    free(mult);
  }

  // wire to counter source
  {
    ps_cb_t *counter = create_counter();
    ps_cb_t *mult = create_multiplier(10);

    mult->source = counter;

    for (uint64_t i=1; i<10; i++) {
      ps_value_t *v = mult->fn(mult, PS_OK);
      REQUIRE(v != nullptr);
      REQUIRE(v->data != nullptr);

      uint64_t iv = *((uint64_t *)v->data);
      CHECK(iv == (i * 10));
      free(v);
    }

    free(counter);
    free(mult);
  }

  // error propagates
  {
    ps_cb_t *counter = create_counter();
    ps_cb_t *mult = create_multiplier(10);

    mult->source = counter;

    handle_status(mult, PS_ERR);

    CHECK(counter->status == PS_ERR);
    CHECK(mult->status == PS_ERR);

    free(counter);
    free(mult);
  }
}
