const fs = require('fs')
const path = require('path')
const str = fs.readFileSync(
  path.join(__dirname, '..', 'lib', 'pull-stream', 'deps', "libuv", "libuv-1.39.0", "include", "uv.h"),
  'utf8'
)
const lines = str.split(/\r?\n/)
const outLines = lines.map(line => {
  if (!line.includes('UV_EXTERN')) {
    return false
  }
  // UV_EXTERN int uv_read_start(
  const matches = line.match(/UV_EXTERN.*(uv_[\w_]+)\(/)
  if (!matches) {
    return false
  }

  const name = matches[1]
  let address = name;
  if (name == "uv_tty_set_mode") {
    address = "static_cast<int (*)(uv_tty_t*, uv_tty_mode_t mode)>(uv_tty_set_mode)"
  }
  return `rawkit_jit_add_export(jit, "${name}", ${address});`
}).filter(Boolean)


const outHeader = `
#pragma once
#include <rawkit/jit.h>
#include <uv.h>

void host_init_uv(rawkit_jit_t *jit) {
  ${outLines.join('\n  ')}
}

`

fs.writeFileSync(
    path.join(__dirname, '..', 'include', 'hot', 'host', 'uv.h'),
    outHeader
);
