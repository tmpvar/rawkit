# pull-stream

A C99 / >= C++10 interpretation of the JS [pull-stream](https://pull-stream.github.io/)
ecosystem. This is meant to be used primarily with [rawkit](https://github.com/tmpvar/rawkit),
but maybe it has utility elsewhere.

## rules

There are a few rules that pull streams should follow:

1. Data only flows when you pull.
2. Values returned by a pull are `ps_destroy`able
3. Marking the status of a stream to `PS_ERR` should release and pending data and abort.
4. Marking the status of a stream to `PS_DONE` may continue dequeuing pending data.
5. `NULL` returned by a pull means nothing more than the source stream did not have data to return.

## usage

```c
#include <pull/stream.h>
#include <stdio.h>

int main() {
  ps_t *s = ps_pipeline(
    // emit a single value "hello"
    create_single_value((void *)"hello", 5),

    // bytewise reverse (at the packet level)
    create_reverser(),

    // take one packet and then mark the pipeline as PS_DONE
    create_taker(1, PS_DONE)
  );

  // pull from s, which is actually the sink defined w/ `create_taker`
  ps_val_t *val = ps_pull(s, PS_OK);

  printf("RESULT: %s\n", (char *)val->data);
}
```