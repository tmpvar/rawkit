const fs = require('fs')
const path = require('path')
const str = fs.readFileSync(path.join(__dirname, '..', 'deps', 'croaring', 'roaring/roaring.h'), 'utf8')
const lines = str.split(/\r?\n/)

const outLines = lines.map(line => {
  if (
    line.includes(' * ') ||
    line.includes('/*') ||
    line.includes('static ') ||
    line.includes('//')
  ) {
    return false
  }

  const matches = line.match(/^.* \*?(roaring_[\w]+)\(/)
  if (!matches) {
    return false
  }

  const name = matches[1]
  return `rawkit_jit_add_export(jit, "${name}", ${name});`
}).filter(Boolean)


const outHeader = `
#pragma once
#include <rawkit/jit.h>
#include <roaring/roaring.h>

void host_croaring_init(rawkit_jit_t  *jit) {
  ${outLines.join('\n  ')}
}
`

fs.writeFileSync(
    path.join(__dirname, '..', 'include', 'hot', 'host', 'croaring.h'),
    outHeader
);
