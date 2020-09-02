#pragma once

#include "../grbl/machine.h"
#include <rawkit/hot/state.h>

#include "../util/ls.h"
#include "../util/readfile.h"

#include <tinyfiledialogs.h>

#include <roaring/roaring.h>


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
  GCODEParser parser;
  int64_t current_line;
  grbl_action_id pending_action;
  roaring_bitmap_t *keyframes;
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
    const size_t len = state->program.length();
    if (len) {
      state->status = PROGRAM_STATE_LOADED;

      for (size_t i = 0; i<len; i++) {
        const String *line = state->program.item(i);
        if (line == NULL) {
          continue;
        }

        state->parser.push(line->handle);
        state->parser.push('\n');

        // locate keyframes (aka lines that are G1 Z-<f>)
        {
          const gcode_line_t *last_line = state->parser.line(-1);
          if (last_line == NULL) {
            continue;
          }

          bool z_only = (
            last_line->words & (1<<GCODE_AXIS_WORD_Z) &&
            !(last_line->words & (1<<GCODE_AXIS_WORD_Y)) &&
            !(last_line->words & (1<<GCODE_AXIS_WORD_X))
          );

          if (z_only) {
            gcode_word_pair_t* pairs = last_line->pairs;
            const size_t pair_count = stb_sb_count(pairs);
            for (size_t pair_idx = 0; pair_idx < pair_count; pair_idx++) {
              gcode_word_pair_t *pair = &pairs[pair_idx];
              if (pair->letter == 'Z') {//} && pair->value  0.0f) {
                roaring_bitmap_add(
                  state->keyframes,
                  i
                );
                break;
              }
            }
          }
        }
      }
    }
  } else if (state->status & PROGRAM_STATE_RUNNING) {
    if (grbl->is_action_complete(state->pending_action)) {
      panel_program_next_line(grbl, state);
    }
  }

  ImVec2 buttonSize = {75, 20};
  ImVec2 spacerSize = {10, 0};

  bool is_unconditionally_paused = false;
  if (state->current_line - 1 > 0) {
    gcode_line_t *l = state->parser.line(state->current_line-1);
    if (l) {
      if (
        l->type == GCODE_LINE_TYPE_M &&
        (l->code == 0.0f || l->code == 0.0f) &&
        grbl->state->state == GRBL_MACHINE_STATE_HOLD
      ) {
        is_unconditionally_paused = true;
      }
    }
  }

  if (is_unconditionally_paused) {
    igOpenPopup("Unconditionally Paused", 0);
  }

  if (igBeginPopupModal("Unconditionally Paused", &is_unconditionally_paused, ImGuiWindowFlags_AlwaysAutoResize)) {
    igPushTextWrapPos(igGetFontSize() * 30.0f);
      igTextUnformatted(
        "The gcode program you are running has paused.\n\n"
        "This typically occurs around tool changes and such.\n\n"
        "Please inspect the program and press resume when you are ready to continue"
      , NULL);
    igPopTextWrapPos();
    if (igButton("Resume", buttonSize)) {
      grbl->cycle_start();
    }
    igEndPopup();
  }

  igBegin("Program", nullptr, ImGuiWindowFlags_None);

  ImVec2 scrollingRegionSize = {0, -90};
  igBeginChildStr(
    "ScrollingRegion",
    scrollingRegionSize,
    false,
    ImGuiWindowFlags_HorizontalScrollbar
  );

  const float line_height = 18.0;
  static int hovered_line = -1;
  ImGuiListClipper clipper;
  ImGuiListClipper_Begin(&clipper, state->program.length(), line_height);
  String *line = nullptr;
  int last_hovered_line = hovered_line;
  hovered_line = -1;
  while (ImGuiListClipper_Step(&clipper)) {
    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
      line = state->program.item(line_no);

      igText("%i", line_no); igSameLine(0, 20.0f);

      if (state->keyframes && roaring_bitmap_contains(state->keyframes, line_no)) {
        igTextColored({1.0f, 1.0f, 0.0f, 1.0f}, line->c_str());
      } else if (line_no == last_hovered_line) {
        igTextColored({1.0f, 0.0f, 1.0f, 1.0f}, line->c_str());
      } else if (state->current_line - 1 == line_no) {
        igTextColored({0.5f, 0.5f, 1.0f, 1.0f}, line->c_str());
      } else {
        igTextUnformatted(line->c_str(), NULL);
      }
      if (igIsItemHovered(0)) {
        hovered_line = line_no;
        const char *tooltip_text = state->parser.line_state_debug(line_no);
        if (tooltip_text != NULL) {
          igBeginTooltip();
          igPushTextWrapPos(igGetFontSize() * 35.0f);
          igText("line: %i", line_no);
          igTextUnformatted(tooltip_text, NULL);
          igPopTextWrapPos();
          igEndTooltip();
        }
      }
    }
  }
  ImGuiListClipper_End(&clipper);

  igSetScrollYFloat(state->current_line * line_height - line_height * 10.0f);

  igEndChild();


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
    // TODO: can we use state->parser instead of having two parsers?
    grbl->tx_parser->reset();
    grbl->soft_reset();
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
      state->parser.reset();

      if (state->keyframes != NULL) {
        roaring_bitmap_clear(state->keyframes);
      } else {
        state->keyframes = roaring_bitmap_create();
      }
    }

    printf("LOADED: %s\n", result);
  }

  // display current status
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
      "status: ( %s) line: %zu action: %zu vs grbl (%zu / %zu)",
      status.handle,
      state->current_line,
      state->pending_action,
      grbl->state->action_complete,
      grbl->state->action_pending
    );
    status.destroy();
  }

  // print out the parser state
  {
    igText("%s", grbl->tx_parser_state_debug());
  }

  igEnd();
}