const fs = require('fs')
const path = require('path')
const str = fs.readFileSync(path.join(__dirname, '..', 'include', 'nanovg', 'nanovg.h'), 'utf8')
const lines = str.split(/\r?\n/)

const outLines = lines.map((line, i) => {
  if (line.trim().endsWith(",")) {
    line += " " + lines[i+1].trim();
  }

  line = line.replace("char*", "char *")
  const matches = line.match(/(\w+)\W+(\w+)\((.*)\);/);
  if (!matches) {
    return
  }

  if (line[0] === "#") {
    return
  }

  if (line.toLowerCase().includes("internal")) {
    return
  }

  if (matches[2].includes('nvgCreateImage')) {
    return
  }

  var name = matches[2].replace("nvg", "rawkit_vg_");

  const isUpper = (c) => c !== '_' && c === c.toUpperCase()
  const l = name.length
  var rawkitName = name[0]
  for (var i=1; i<l-1; i++) {
    var c = name[i];
    var p = name[i-1];
    var n = name[i+1];
    if (isUpper(c) && !isUpper(p) && !isUpper(n)) {
      if (!rawkitName.endsWith('_')) {
        rawkitName += "_"
       }
       rawkitName += c.toLowerCase()
    } else {
      rawkitName += c
    }
  }
  rawkitName += name[name.length-1]

  const args = matches[3].split(',').map(l => {
    l = l.trim()
    const parts = l.split(' ')
    var name = parts.pop();
    var type = parts.join(' ')
    if (type === 'NVGcontext*') {
      type = 'rawkit_vg_t *'
    }

    if (name === 'ctx') {
      name = 'vg'
    }
    return [type, name]
  })

  const returnType = matches[1]

  const signature = `${returnType} ${rawkitName}(${args.map(p => {
    if (p[0].endsWith('*')) {
      return p.join('');
    }
    return p.join(' ')
  }).join(', ')})`

  return {
    matches,
    returnType,
    originalName: matches[2],
    rawkitName,
    args,
    signature
  }
}).filter(Boolean)


fs.writeFileSync(path.join(__dirname, '..', '..', '..', 'include', 'hot', 'host', 'rawkit-vg.h'),
`#pragma once
#include <rawkit/jit.h>
#include <rawkit/vg.h>

void host_init_rawkit_vg(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "rawkit_vg", rawkit_vg);
  rawkit_jit_add_export(jit, "rawkit_vg_default", rawkit_vg_default);

  ${outLines.map(({rawkitName}) => {

    return `rawkit_jit_add_export(jit, "${rawkitName}", ${rawkitName});`
  }).join('\n  ')}
}
`
);

fs.writeFileSync(path.join(__dirname, '..', 'include', 'rawkit', 'vg.h'),
`#pragma once

#include <rawkit/core.h>
#include <rawkit/hot.h>
#include <nanovg/nanovg.h>
#include <rawkit/vulkan.h>

typedef struct rawkit_vg_t {

  // internal
  void *_state;
} rawkit_vg_t;

#ifdef __cplusplus
extern "C" {
#endif

  rawkit_vg_t *rawkit_vg(
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkRenderPass render_pass
  );

  rawkit_vg_t *rawkit_vg_default();

  ${outLines.map(({signature}) => {
  return signature + ';'
}).join('\n  ')}
#ifdef __cplusplus
}
#endif
`
);

fs.writeFileSync(path.join(__dirname, '..', 'src', 'nanovg-binding.cpp'),
`#include "nanovg-binding.h"

${outLines.map(({signature, returnType, originalName, args}) => {
  if (args[0][1] === 'vg') {

    return `
${signature} {
  VGState *state = (VGState *)vg->_state;
  NVGcontext *ctx = state->ctx;
  ${returnType !== 'void' ? 'return ' : ''}${originalName}(${args.map((a, i)=> !i ? 'ctx': a[1]).join(', ')});
}
    `
  }
  return `
${signature} {
  ${returnType !== 'void' ? 'return ' : ''}${originalName}(${args.map(a=>a[1]).join(', ')});
}
  `
}).join('')}
`
);
