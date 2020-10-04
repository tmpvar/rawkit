#include <doctest.h>

#include <pull/stream.h>

TEST_CASE("[pull/stream] collector through stream") {
  // error if not connected
  {
    ps_t *collector = create_collector();
    REQUIRE(collector != nullptr);
    CHECK(collector->status == PS_OK);
    ps_val_t *val = collector->fn(collector, PS_OK);
    REQUIRE(val == nullptr);
    CHECK(collector->status == PS_ERR);
    ps_val_destroy(val);
    ps_destroy(collector);
  }

  // collect 5 numbers and output them in one packet
  {

    ps_t *counter = create_counter();
    ps_t *taker = create_taker(5, PS_DONE);
    ps_t *collector = create_collector();

    taker->source = counter;
    collector->source = taker;

    // drain the queue
    ps_val_t *val = NULL;
    for (unsigned int i=1; i<=5; i++) {
      val = collector->fn(collector, PS_OK);
      REQUIRE(val == nullptr);
      CHECK(collector->status == PS_OK);
    }

    // pull the full packet
    val = collector->fn(collector, PS_OK);
    REQUIRE(val != nullptr);
    CHECK(val->len == (sizeof(uint64_t) * 5));
    uint64_t *values = (uint64_t *)val->data;
    CHECK(values[0] == 1);
    CHECK(values[1] == 2);
    CHECK(values[2] == 3);
    CHECK(values[3] == 4);
    CHECK(values[4] == 5);
    CHECK(collector->status == PS_OK);

    ps_val_destroy(val);

    // pull after done..
    val = collector->fn(collector, PS_OK);
    REQUIRE(val == nullptr);
    CHECK(collector->status == PS_DONE);


    ps_destroy(counter);
    ps_destroy(taker);
    ps_destroy(collector);
  }
}