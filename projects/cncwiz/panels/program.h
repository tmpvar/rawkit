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
  PROGRAM_STATE_RUNNING = 1<<4,
  PROGRAM_STATE_PAUSED = 1<<5,
  PROGRAM_STATE_ABORTED = 1<<6,
};

struct cncwiz_program_state {
  cncwiz_program_status status;
  String program_file;
  StringList program;
  uint64_t current_line;
  grbl_action_id pending_action;
};



// const char *path = "E:\\cnc\\gcode\\";
#define CNCWIZ_PROGRAM_FILE_FILTER_COUNT 2
const char *cncwiz_program_file_filter[CNCWIZ_PROGRAM_FILE_FILTER_COUNT] = {
  "*.nc",
  "*.gcode"
};

void panel_program_next_line(GrblMachine *grbl, cncwiz_program_state *state) {
  String *line = state->program.item(state->current_line++);
  if (line == NULL) {
    state->status = PROGRAM_STATE_LOADED;
    state->current_line = 0;
  } else {
    grbl->write(line->handle);
    state->pending_action = grbl->end_action();
  }
}

void panel_program(GrblMachine *grbl) {
  cncwiz_program_state *state = (cncwiz_program_state *)hotState(
    RAWKIT_PROGRAM_STATE_OFFSET,
    sizeof(cncwiz_program_state),
    nullptr
  );

  if (state->status & PROGRAM_STATE_LOADING) {
    state->program.destroy();
    state->program = readfile_lines(state->program_file.handle);
    if (state->program.length()) {
      state->status = PROGRAM_STATE_LOADED;
    }
  } else if (state->status & PROGRAM_STATE_RUNNING) {
    if (grbl->is_action_complete(state->pending_action)) {
      panel_program_next_line(grbl, state);
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

  const float line_height = 18.0;

  ImGuiListClipper clipper;
  ImGuiListClipper_Begin(&clipper, state->program.length(), line_height);
  String *line = nullptr;
  while (ImGuiListClipper_Step(&clipper)) {
    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
      line = state->program.item(line_no);
      if (state->current_line == line_no) {
        igTextColored({0.0f, 1.0f, 0.0f, 1.0f}, line->c_str());
      } else {
        igTextUnformatted(line->c_str(), NULL);
      }
    }
  }
  ImGuiListClipper_End(&clipper);

  igSetScrollYFloat(state->current_line * line_height - line_height * 20.0f);

  igEndChild();
  ImVec2 buttonSize = {75, 20};
  ImVec2 spacerSize = {10, 0};

  if (!(state->status & PROGRAM_STATE_RUNNING)) {
    if (igButton("run", buttonSize)) {
      if (state->status & PROGRAM_STATE_LOADED) {
        grbl->cycle_start();
        if (!(state->status & PROGRAM_STATE_RUNNING)) {
          if (grbl->is_action_complete(state->pending_action)) {
            panel_program_next_line(grbl, state);
          }
        }

        state->status = (cncwiz_program_status)(PROGRAM_STATE_LOADED | PROGRAM_STATE_RUNNING);
      }
    }
  } else {
    if (igButton("pause", buttonSize)) {
      if (state->status & PROGRAM_STATE_RUNNING) {
        grbl->feed_hold();
        state->status = (cncwiz_program_status)(PROGRAM_STATE_LOADED | PROGRAM_STATE_PAUSED);
      }
    }
  }

  igSameLine(0.0, 1.0);
  igDummy(spacerSize);
  igSameLine(0.0, 1.0);
  if (igButton("abort", buttonSize)) {
    state->current_line = 0;
    state->pending_action = 0;
    if (state->status & PROGRAM_STATE_LOADED) {
      state->status = PROGRAM_STATE_LOADED;
    }
  }
  igSameLine(0.0, 1.0);

  igSameLine(0.0, 100.0);
  if (igButton("load", buttonSize)) {
    // TODO: this is sort of pointless as tinyfd blocks the render loop.
    state->status = PROGRAM_STATE_BROWSING;

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
      state->pending_action = -1;
      state->current_line = 0;
    }

    printf("LOADED: %s\n", result);
  }

  {
    String status("");

    if (state->status & PROGRAM_STATE_ABORTED) {
      status.append_c_str("aborted ");
    }

    if (state->status & PROGRAM_STATE_BROWSING) {
      status.append_c_str("browsing ");
    }

    if (state->status & PROGRAM_STATE_LOADED) {
      status.append_c_str("loaded ");
    }

    if (state->status & PROGRAM_STATE_LOADING) {
      status.append_c_str("loading ");
    }

    if (state->status == PROGRAM_STATE_NONE) {
      status.append_c_str("none ");
    }

    if (state->status & PROGRAM_STATE_PAUSED) {
      status.append_c_str("paused ");
    }

    if (state->status & PROGRAM_STATE_RUNNING) {
      status.append_c_str("running ");
    }


    igText(
      "status: ( %s) line: %zu action: %zu / %zu",
      status.handle,
      state->current_line,
      state->pending_action,
      grbl->state->action_pending
    );
    status.destroy();
  }

  igEnd();
}