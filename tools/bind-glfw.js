const fs = require('fs')
const path = require('path')

const str = fs.readFileSync(
  path.join(__dirname, '..', 'lib', 'rawkit-window', 'deps', 'glfw', 'glfw-3.3-stable', 'include', 'GLFW', 'glfw3.h')
  ,
  'utf8'
)
const lines = str.split(/\r?\n/)

const outLines = lines.map(line => {
  if (!line.includes('GLFWAPI') || line.includes('#define')) {
    return false
  }

  const matches = line.match(/GLFWAPI.* ([\w]+)\(/)
  if (!matches) {
    return false
  }

  const name = matches[1]
  return `rawkit_jit_add_export(jit, "${name}", ${name});`
}).filter(Boolean)


const outHeader = `
#pragma once
#include <rawkit/jit.h>
#include <GLFW/glfw3.h>
void host_glfw_init(rawkit_jit_t *jit) {
  ${outLines.join('\n  ')}
}
`

fs.writeFileSync(
  path.join(__dirname, '..', 'include', 'hot', 'host', 'glfw.h'),
  outHeader
);
