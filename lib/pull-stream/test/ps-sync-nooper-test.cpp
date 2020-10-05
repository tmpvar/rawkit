#include <doctest.h>

#include <pull/stream.h>

TEST_CASE("[pull/stream] noop through stream") {
  // expect error to propagate down when not hooked up
  {
    ps_t *nop1 = create_nooper();
    ps_t *nop2 = create_nooper();
    ps_t *nop3 = create_nooper();

    nop2->source = nop1;
    nop3->source = nop2;

    // call fn without setting up the source
    ps_val_t *v = nop3->fn(nop3, PS_OK);
    REQUIRE(v == nullptr);

    // ensure the error propagates down
    CHECK(nop3->status == PS_ERR);
    CHECK(nop2->status == PS_ERR);
    CHECK(nop1->status == PS_ERR);

    ps_destroy(nop1);
    ps_destroy(nop2);
    ps_destroy(nop3);
  }

  // expect error to propagate up
  {
    ps_t *counter = create_counter();
    ps_t *nop1 = create_nooper();
    ps_t *nop2 = create_nooper();
    ps_t *nop3 = create_nooper();

    nop1->source = counter;
    nop2->source = nop1;
    nop3->source = nop2;

    // pull with OK
    {
      ps_val_t *v = nop3->fn(nop3, PS_OK);
      REQUIRE(v != nullptr);
      ps_destroy(v);
      CHECK(counter->status == PS_OK);
      CHECK(nop3->status == PS_OK);
      CHECK(nop2->status == PS_OK);
      CHECK(nop1->status == PS_OK);
    }

    // pull with ERR
    {
      ps_val_t *v = nop3->fn(nop3, PS_ERR);
      REQUIRE(v == nullptr);
      // ensure the error propagates up
      CHECK(counter->status == PS_ERR);
      CHECK(nop3->status == PS_ERR);
      CHECK(nop2->status == PS_ERR);
      CHECK(nop1->status == PS_ERR);
    }

    ps_destroy(counter);
    ps_destroy(nop1);
    ps_destroy(nop2);
    ps_destroy(nop3);
  }
}
