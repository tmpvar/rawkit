#include <doctest.h>

#include <pull/stream.h>

TEST_CASE("[pull/stream/redux] counter") {
  // normal operation
  {
    ps_cb_t *counter = create_counter();

    for (uint64_t i=0; i<10; i++) {
      ps_value_t *v = counter->fn(counter, PS_OK);
      REQUIRE(v != nullptr);
      CHECK(counter->status == PS_OK);
      uint64_t iv = *((uint64_t *)v->data);
      CHECK(iv == (i+1));
      free(v);
    }

    free(counter);
  }

  // infinite counter (abort via done)
  {
    ps_cb_t *counter = create_counter();

    // check normal operation
    {
      for (uint64_t i=0; i<10; i++) {
        ps_value_t *v = counter->fn(counter, PS_OK);
        REQUIRE(v != nullptr);
        CHECK(counter->status == PS_OK);
        uint64_t iv = *((uint64_t *)v->data);
        CHECK(iv == (i+1));
        free(v);
      }
    }

    // abort the stream via done
    {
      ps_value_t *v = counter->fn(counter, PS_DONE);
      CHECK(counter->status == PS_DONE);
      REQUIRE(v == nullptr);
    }

    // future calls resolve to done
    {
      for (uint64_t i=0; i<10; i++) {
        ps_value_t *v = counter->fn(counter, PS_OK);
        REQUIRE(v == nullptr);
      }
    }

    // changing the status to ERR via call does nothing
    {
      ps_value_t *v = counter->fn(counter, PS_ERR);
      CHECK(counter->status == PS_DONE);
      REQUIRE(v == nullptr);
    }

    free(counter);
  }

  // infinite counter (abort via error)
  {
    ps_cb_t *counter = create_counter();

    // check normal operation
    {
      for (uint64_t i=0; i<10; i++) {
        ps_value_t *v = counter->fn(counter, PS_OK);
        REQUIRE(v != nullptr);
        CHECK(counter->status == PS_OK);
        uint64_t iv = *((uint64_t *)v->data);
        CHECK(iv == (i+1));
        free(v);
      }
    }

    // abort the stream via error
    {
      ps_value_t *v = counter->fn(counter, PS_ERR);
      REQUIRE(v == nullptr);
    }

    // future calls resolve to error
    {
      for (uint64_t i=0; i<10; i++) {
        ps_value_t *v = counter->fn(counter, PS_OK);
        REQUIRE(v == nullptr);
      }
    }

    // changing the status to DONE via call does nothing
    {
      ps_value_t *v = counter->fn(counter, PS_DONE);
      CHECK(counter->status == PS_ERR);
      REQUIRE(v == nullptr);
    }

    free(counter);
  }
}