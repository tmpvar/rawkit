const fs = require('fs')
const path = require('path')
const str = fs.readFileSync(path.join(__dirname, 'lz4-1.9.3', 'lib', 'lz4.h'), 'utf8')
const lines = str.split(/\r?\n/)

const outLines = lines.map((line, idx) => {
  if (line[0] == "#") {
    return false;
  }
  const prevLine = idx ? lines[idx-1] : "";


  if (
    !line.includes('LZ4LIB_API') ||
    line.includes('LZ4_DEPRECATED') ||
    prevLine.includes('LZ4_DEPRECATED')
  ) {
    return false
  }

  line = line.split('LZ4LIB_API').pop();

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
#include <lz4.h>

void host_init_lz4(rawkit_jit_t *jit) {
  ${outLines.join('\n  ')}
}

`
fs.writeFileSync(
    path.join(__dirname, '..', '..', 'include', 'hot', 'host', 'lz4.h'),
    outHeader
);
