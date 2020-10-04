#include <pull/stream.h>

#include <stdint.h>

typedef struct hex_printer_t {
  PS_CB_FIELDS

  FILE *output;
} hex_printer_t;

static void dump_hex(FILE *file, void *data, uint64_t size) {
  // lifted from https://gist.github.com/ccbrown/9722406 WTFPL license
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		fprintf(file, "%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			fprintf(file, " ");
			if ((i+1) % 16 == 0) {
				fprintf(file, "|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					fprintf(file, " ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					fprintf(file, "   ");
				}
				fprintf(file, "|  %s \n", ascii);
			}
		}
	}
}

ps_val_t *hex_printer_fn(ps_t *base, ps_status status) {

  ps_val_t *val = pull_through(base, status);
  if (!val) {
    return NULL;
  }
  hex_printer_t *printer = (hex_printer_t *)base;
  FILE *file = printer->output;
  dump_hex(file, val->data, val->len);

  return val;
}

ps_t *create_hex_printer(FILE *output) {
  hex_printer_t *printer = (hex_printer_t *)calloc(sizeof(hex_printer_t), 1);
  printer->fn = hex_printer_fn;

  if (!output) {
    handle_status((ps_t *)printer, PS_ERR);
  } else {
	printer->output = output;
  }

  return (ps_t *)printer;
}