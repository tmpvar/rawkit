const fs = require('fs')
const path = require('path')
const str = fs.readFileSync(path.join(__dirname, '..', 'deps', 'cimgui', 'cimgui.h'), 'utf8')
const lines = str.split(/\r?\n/)

const outLines = lines.map(line => {
  if (!line.includes('CIMGUI_API')) {
    return false
  }

  const matches = line.match(/CIMGUI_API.* ([\w]+)\(/)
  if (!matches) {
    return false
  }

  const name = matches[1]
  return `job->addExport("${name}", ${name});`  
}).filter(Boolean)


const outHeader = `
#pragma once
#include <hot/jitjob.h>

void host_cimgui_init(JitJob *job);

`

const outSource = `
#include <hot/host/cimgui.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

void host_cimgui_init(JitJob *job) {
  ${outLines.join('\n  ')}
}

`

fs.writeFileSync(
    path.join(__dirname, '..', 'include', 'hot', 'host', 'cimgui.h'),
    outHeader
);

fs.writeFileSync(
    path.join(__dirname, '..', 'src', 'hot', 'cimgui.cpp'),
    outSource
);