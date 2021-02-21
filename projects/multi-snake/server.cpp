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
  State *state = rawkit_hot_state("state", State);
  TCPServer *server = state->server;

  u32 c = sb_count(server->clients);
  igText("clients: %u", c);

  sockaddr_storage addr = {};
  int namelen = sizeof(sockaddr_storage);
  for (u32 i=0; i<c; i++) {
    if (server->clients[i] == nullptr) {
      igText("  (empty)");
    }
    uv_tcp_getpeername(server->clients[i], (sockaddr *)&addr, &namelen);
    struct sockaddr_in *sin = (struct sockaddr_in *)&addr;
    u8 *ip = (u8 *)&sin->sin_addr.s_addr;
    igText("  %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  }

  igText("sent: %llu", server->bytes_sent);
  igText("recv: %llu", server->bytes_recv);


}