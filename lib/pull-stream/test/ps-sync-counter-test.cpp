#include <doctest.h>

#include <pull/stream.h>

TEST_CASE("[pull/stream/redux] counter source stream") {
  // normal operation
  {
    ps_t *counter = create_counter();

    for (uint64_t i=0; i<10; i++) {
      ps_val_t *v = counter->fn(counter, PS_OK);
      REQUIRE(v != nullptr);
      CHECK(counter->status == PS_OK);
      uint64_t iv = *((uint64_t *)v->data);
      CHECK(iv == (i+1));
      ps_val_destroy(v);
    }

    ps_destroy(counter);
  }

  // infinite counter (abort via done)
  {
    ps_t *counter = create_counter();

    // check normal operation
    {
      for (uint64_t i=0; i<10; i++) {
        ps_val_t *v = counter->fn(counter, PS_OK);
        REQUIRE(v != nullptr);
        CHECK(counter->status == PS_OK);
        uint64_t iv = *((uint64_t *)v->data);
        CHECK(iv == (i+1));
        ps_val_destroy(v);
      }
    }

    // abort the stream via done
    {
      ps_val_t *v = counter->fn(counter, PS_DONE);
      CHECK(counter->status == PS_DONE);
      REQUIRE(v == nullptr);
    }

    // future calls resolve to done
    {
      for (uint64_t i=0; i<10; i++) {
        ps_val_t *v = counter->fn(counter, PS_OK);
        REQUIRE(v == nullptr);
      }
    }

    // changing the status to ERR via call does nothing
    {
      ps_val_t *v = counter->fn(counter, PS_ERR);
      CHECK(counter->status == PS_DONE);
      REQUIRE(v == nullptr);
    }

    ps_destroy(counter);
  }

  // infinite counter (abort via error)
  {
    ps_t *counter = create_counter();

    // check normal operation
    {
      for (uint64_t i=0; i<10; i++) {
        ps_val_t *v = counter->fn(counter, PS_OK);
        REQUIRE(v != nullptr);
        CHECK(counter->status == PS_OK);
        uint64_t iv = *((uint64_t *)v->data);
        CHECK(iv == (i+1));
        ps_val_destroy(v);
      }
    }

    // abort the stream via error
    {
      ps_val_t *v = counter->fn(counter, PS_ERR);
      REQUIRE(v == nullptr);
    }

    // future calls resolve to error
    {
      for (uint64_t i=0; i<10; i++) {
        ps_val_t *v = counter->fn(counter, PS_OK);
        REQUIRE(v == nullptr);
      }
    }

    // changing the status to DONE via call does nothing
    {
      ps_val_t *v = counter->fn(counter, PS_DONE);
      CHECK(counter->status == PS_ERR);
      REQUIRE(v == nullptr);
    }

    ps_destroy(counter);
  }
}
