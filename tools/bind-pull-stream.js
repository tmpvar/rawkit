const fs = require('fs')
const path = require('path')

const base = path.join(__dirname, '..', 'lib', 'pull-stream', 'include', 'pull');
const str = fs.readdirSync(base)
  .filter((a) => a.includes(".h"))
  .map((a) => fs.readFileSync(path.join(base, a), 'utf8'))
  .join('\n');

const lines = str.split(/\r?\n/).filter(Boolean)
const outLines = lines.map(line => {
  if (!line.includes('PS_EXPORT')) {
    return false
  }

  const matches = line.match(/PS_EXPORT.*\W\*?(\w+)\(/)

  if (!matches) {
    return false
  }

  const name = matches[1]
  let address = name;
  return `rawkit_jit_add_export(jit, "${name}", ${address});`
}).filter(Boolean)


const outHeader = `
#pragma once
#include <rawkit/jit.h>
#include <pull/stream.h>

void host_init_pull_stream(rawkit_jit_t *jit) {
  ${outLines.join('\n  ')}
}

`

fs.writeFileSync(
    path.join(__dirname, '..', 'include', 'hot', 'host', 'pull-stream.h'),
    outHeader
);
