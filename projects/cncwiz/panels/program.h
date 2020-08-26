#pragma once

#include "../grbl/machine.h"
#include <rawkit/hot/state.h>

#include "../util/ls.h"
#include "../util/readfile.h"

#include <tinyfiledialogs.h>

#define RAWKIT_PROGRAM_STATE_OFFSET 0xFF100000

enum cncwiz_program_status {
  PROGRAM_STATE_NONE = 0,
  PROGRAM_STATE_BROWSING = 1<<1,
  PROGRAM_STATE_LOADING = 1<<2,
  PROGRAM_STATE_LOADED = 1<<3,
};

struct cncwiz_program_state {
  cncwiz_program_status status;
  String program_file;
  StringList program;
};



// const char *path = "E:\\cnc\\gcode\\";
#define CNCWIZ_PROGRAM_FILE_FILTER_COUNT 2
const char *cncwiz_program_file_filter[CNCWIZ_PROGRAM_FILE_FILTER_COUNT] = {
  "*.nc",
  "*.gcode"
};

void panel_program(GrblMachine *grbl) {
  cncwiz_program_state *state = (cncwiz_program_state *)hotState(
    RAWKIT_PROGRAM_STATE_OFFSET,
    sizeof(cncwiz_program_state),
    nullptr
  );

  if (state->status == PROGRAM_STATE_LOADING) {
    state->program.destroy();
    state->program = readfile_lines(state->program_file.handle);
    if (state->program.length()) {
      state->status = PROGRAM_STATE_NONE;
    }
  }

  igBegin("Program", nullptr, ImGuiWindowFlags_None);

  ImVec2 scrollingRegionSize = {0, -50};
  igBeginChildStr(
    "ScrollingRegion",
    scrollingRegionSize,
    false,
    ImGuiWindowFlags_HorizontalScrollbar
  );

  ImGuiListClipper clipper;
  ImGuiListClipper_Begin(&clipper, state->program.length(), 18.0);
  String *line = nullptr;
  while (ImGuiListClipper_Step(&clipper)) {
    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
      line = state->program.item(line_no);
      igTextUnformatted(line->c_str(), NULL);
    }
  }
  ImGuiListClipper_End(&clipper);

  igEndChild();
  ImVec2 buttonSize = {75, 20};
  ImVec2 spacerSize = {10, 0};
  igButton("run", buttonSize);
  igSameLine(0.0, 1.0);
  igDummy(spacerSize);
  igSameLine(0.0, 1.0);
  igButton("pause", buttonSize);
  igSameLine(0.0, 1.0);
  igDummy(spacerSize);
  igSameLine(0.0, 1.0);
  if (igButton("abort", buttonSize)) {
    tinyfd_beep();
  }
  igSameLine(0.0, 1.0);

  igSameLine(0.0, 100.0);
  if (igButton("load", buttonSize)) {
    // TODO: this is sort of pointless as tinyfd blocks the render loop.
    state->status = PROGRAM_STATE_LOADING;

    char *result = tinyfd_openFileDialog(
	    NULL,
      // TODO: pull this from config/settings
      "E:\\cnc\\gcode",
      CNCWIZ_PROGRAM_FILE_FILTER_COUNT,
	    cncwiz_program_file_filter,
	    "gcode files",
      0
    );

    if (result) {
      state->status = PROGRAM_STATE_LOADING;
      state->program.clear();
      state->program_file.set_c_str(result);

    }

    printf("LOADED: %s\n", result);
  }
  igEnd();
}