#include <doctest.h>

#include <pull/stream.h>

#include <string.h>
#include <stdlib.h>

TEST_CASE("[pull/stream] hex printer test") {
  // error if not connected to a source
  {
    ps_t *printer = create_hex_printer(NULL);
    REQUIRE(printer != nullptr);
    CHECK(printer->status == PS_ERR);
    ps_destroy(printer);
  }

  // error if not connected to a source
  {
    ps_t *printer = create_hex_printer(stdout);
    REQUIRE(printer != nullptr);
    CHECK(printer->status == PS_OK);
    ps_val_t *val =printer->fn(printer, PS_OK);
    REQUIRE(val == nullptr);
    CHECK(printer->status == PS_ERR);
    ps_destroy(printer);
  }

  {
    FILE *tmp = tmpfile();
    const char *str = "abc123";
    const uint64_t len = strlen(str);

    ps_t *source = create_single_value((void *)str, len);
    ps_t *printer = create_hex_printer(tmp);

    printer->source = source;

    // first pull (OK)
    {
      ps_val_t *val = printer->fn(printer, PS_OK);
      REQUIRE(val != nullptr);
      CHECK(printer->status == PS_OK);
    }

    // second pull (DONE)
    {
      ps_val_t *val = printer->fn(printer, PS_OK);
      REQUIRE(val == nullptr);
      CHECK(printer->status == PS_DONE);

      ps_val_destroy(val);

      uint64_t buffer_len = ftell(tmp);

      char *buffer = (char *)calloc(buffer_len + 1, 1);
      REQUIRE(buffer != nullptr);

      fseek(tmp, 0L, SEEK_SET);
      fread(buffer, sizeof(char), buffer_len, tmp);
      fclose(tmp);

      CHECK(strcmp(
        buffer,
        "61 62 63 31 32 33                                 |  abc123 \n"
      ) == 0);
    }

    ps_destroy(printer);
  }
}
