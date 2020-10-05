#include <doctest.h>

#include <pull/stream.h>


TEST_CASE("[pull/stream] taker through stream") {

  // error when not hooked up
  {
    ps_t *taker = create_taker(5, PS_DONE);
    ps_val_t *val = taker->fn(taker, PS_OK);
    REQUIRE(val == nullptr);
    CHECK(taker->status == PS_ERR);
    ps_destroy(taker);
  }

  // take n entries and then DONE
  {
    ps_t *counter = create_counter();
    ps_t *taker = create_taker(1, PS_DONE);

    taker->source = counter;

    // first read (good)
    {
      ps_val_t *val = taker->fn(taker, PS_OK);
      REQUIRE(val != nullptr);
      CHECK(val->len == sizeof(uint64_t));
      CHECK((*(uint64_t *)val->data) == 0);
      ps_destroy(val);
    }

    // second read (done)
    {
      ps_val_t *val = taker->fn(taker, PS_OK);
      REQUIRE(val == nullptr);
      CHECK(counter->status == PS_DONE);
      CHECK(taker->status == PS_DONE);
    }

    ps_destroy(counter);
    ps_destroy(taker);
  }

  // take n entries and then ERR
  {
    ps_t *counter = create_counter();
    ps_t *taker = create_taker(1, PS_ERR);

    taker->source = counter;

    // first read (good)
    {
      ps_val_t *val = taker->fn(taker, PS_OK);
      REQUIRE(val != nullptr);
      CHECK(val->len == sizeof(uint64_t));
      CHECK((*(uint64_t *)val->data) == 0);
      ps_destroy(val);
    }

    // second read (error)
    {
      ps_val_t *val = taker->fn(taker, PS_OK);
      REQUIRE(val == nullptr);
      CHECK(counter->status == PS_ERR);
      CHECK(taker->status == PS_ERR);
    }

    ps_destroy(counter);
    ps_destroy(taker);
  }

}
