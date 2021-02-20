#include <rawkit/rawkit.h>
#include <stb_sb.h>
#include <uv.h>

#include "shared.h"

#include "tcp.h"

struct State {
  TCPServer *server;
};


void setup() {
  State *state = rawkit_hot_state("state", State);
  if (!state->server) {
    state->server = new TCPServer(3030);
  }
}

void loop() {

}