const fs = require('fs')
const path = require('path')
const str = fs.readFileSync(path.join(__dirname, 'hidapi-8741697', 'hidapi', 'hidapi.h'), 'utf8')
const lines = str.split(/\r?\n/)

const outLines = lines.map((line, idx) => {
  if (line[0] == "#") {
    return false;
  }
  const prevLine = idx ? lines[idx-1] : "";


  if (
    !line.includes('HID_API_EXPORT') ||
    line.includes("#define")
  ) {
    return false
  }

  line = line.split('HID_API_EXPORT').pop();

  const matches = line.match(/.* ([\w]+)\W?\(/)
  if (!matches) {
    return false
  }

  const name = matches[1]
  return `rawkit_jit_add_export(jit, "${name}", ${name});`
}).filter(Boolean)

const outHeader = `
#pragma once
#include <rawkit/jit.h>
#include <hidapi/hidapi.h>

void host_init_hidapi(rawkit_jit_t *jit) {
  ${outLines.join('\n  ')}
}
`
fs.writeFileSync(
    path.join(__dirname, '..', '..', 'include', 'hot', 'host', 'hidapi.h'),
    outHeader
);
