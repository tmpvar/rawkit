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

  vec3 last_machine_position;
  uint8_t aborting;
  bool autoscroll;
  bool show_resume_modal;
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

String overlay_program_resume(GrblMachine *grbl, cncwiz_program_state *state, gcode_line_t *parsed_line) {
  if (parsed_line == NULL) {
    return String("");
  }

  bool result = false;
  igOpenPopup("overlay::program::resume", 0);

  igBeginPopupModal(
    "overlay::program::resume",
    0,
    ImGuiWindowFlags_AlwaysAutoResize |
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoMove
  );

  // compute the gcode that we'll use to put grbl in the right
  // state.
  String acc("");
  {
    gcode_parser_state_strings_t strings;
    if (!state->parser.line_state_strings(state->current_line, &strings)) {
      printf("ERROR: unable to retrieve the line state strings for line %lli\n", state->current_line);
      exit(1);
    }

    acc.append("%s %s %s %s %s %s\n",
      strings.wcs_select,
      strings.plane_select,
      strings.units_mode,
      "G90",
      strings.coolant_state,
      strings.feed_rate_mode
    );

    // restore spindle state
    if (
      parsed_line->parser_state.spindle_state &&
      parsed_line->parser_state.spindle_speed > 0
    ) {
      acc.append("%s S%i\n",
        strings.spindle_state,
        parsed_line->parser_state.spindle_speed
      );
    }

    // move to a safe z
    acc.append_c_str("G53 G0 Z0\n");

    // dwell while we let the spindle spool for N seconds
    // TODO: allow spindle spool time to be configured
    int spindle_spool_seconds = 1;
    acc.append("G4 P%i\n", spindle_spool_seconds);

    float feed_rate = parsed_line->parser_state.feed_rate;

    // move to the starting x/y
    acc.append("G0 X%0.3f Y%0.3f\n",
      parsed_line->parser_state.start_x,
      parsed_line->parser_state.start_y
    );

    // move to the starting z
    acc.append("Z%0.3f\n", parsed_line->parser_state.start_z);

    if (parsed_line->parser_state.distance_mode != GCODE_DISTANCE_MODE_G90) {
      acc.append("%s\n", strings.distance_mode);
    }

    //only write the motion mode if it is not specified on the line
    if (parsed_line->parser_state.motion_mode != GCODE_MOTION_MODE_G0) {
      if (
        // TODO: this can easily bug if a different G command is specified (G90 for instance)
        //       I think to do this properly we have to scan the pairs
        !(parsed_line->words & (1 << GCODE_WORD_G)) &&
        !(parsed_line->words & (1 << GCODE_WORD_M))
      ) {
        acc.append("%s ", strings.motion_mode);
      }
    } else {
      acc.append("%s\n", strings.motion_mode);
    }

    if (
      feed_rate > 0.0f &&
      !(parsed_line->words & (1 << GCODE_WORD_F))
    ) {
      acc.append("F%0.3f ", feed_rate);
    }
  }


  igPushTextWrapPos(igGetFontSize() * 25.0f);
    igTextUnformatted(
      "Are you sure you want to resume?\n"
      "The following code will be run to put the machine in the correct state:\n\n"
    , NULL);
  igPopTextWrapPos();

  igTextUnformatted(
    acc.handle,
    NULL
  );
  igDummy({0.0, 10.0f});

  {
    ImVec2 buttonSize = {70.0, 30.0};
    igDummy({25.0f, 0.0f});
    igSameLine(0.0, 0.0f);


    if (igButton("cancel", buttonSize) || igIsKeyReleased(0x100)) {
      result = false;
      state->show_resume_modal = false;
    }

    igSameLine(0.0, 100.0f);
    if (igButton("continue", buttonSize)) {
      result = true;
      state->show_resume_modal = false;
    }
  }

  igEndPopup();

  if (result) {
    return acc;
  } else {
    acc.destroy();
    return String("");
  }
}


void panel_program(GrblMachine *grbl) {
  cncwiz_program_state *state = (cncwiz_program_state *)hotState(
    RAWKIT_PROGRAM_STATE_OFFSET,
    sizeof(cncwiz_program_state),
    nullptr
  );

  // Ensure grbl's idle state reflects the current program run state
  {
    if (
      (state->status & PROGRAM_STATE_PAUSED) ||
      (state->status & PROGRAM_STATE_RUNNING)
    ) {
      grbl->state->force_not_idle = true;
    } else {
      grbl->state->force_not_idle = false;
    }
  }

  // Handle grbl disconnections while the job is running
  {
    bool grbl_able_to_run = (
      grbl->state->state == GRBL_MACHINE_STATE_IDLE ||
      grbl->state->state == GRBL_MACHINE_STATE_RUN ||
      grbl->state->state == GRBL_MACHINE_STATE_HOLD
    );

    if (!grbl_able_to_run && state->status > PROGRAM_STATE_LOADED) {
      state->status = PROGRAM_STATE_LOADED;
      state->pending_action = -1;
      grbl->state->action_complete = 0;
      grbl->state->action_pending = 0;
      // TODO: can we use state->parser instead of having two parsers?
      grbl->tx_parser->reset();
      state->aborting = 0;
    }
  }

  if (state->status & PROGRAM_STATE_LOADING) {
    state->program.destroy();
    state->program = readfile_lines(state->program_file.handle);
    const size_t len = state->program.length();
    if (len) {
      state->status = PROGRAM_STATE_LOADED;
      state->autoscroll = 1;
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

          if (z_only && last_line->parser_state.motion_mode == GCODE_MOTION_MODE_G0) {
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
        (l->code == 0.0f || l->code == 1.0f) &&
        grbl->state->state == GRBL_MACHINE_STATE_HOLD
      ) {
        is_unconditionally_paused = true;
      }
    }
  }

  if (!state->aborting && is_unconditionally_paused) {
    igOpenPopup("Unconditionally Paused", 0);
  }

  if (igBeginPopupModal("Unconditionally Paused", &is_unconditionally_paused, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
    igPushTextWrapPos(igGetFontSize() * 30.0f);
      igTextUnformatted(
        "The gcode program you are running has paused.\n\n"
        "This typically occurs around tool changes and such.\n\n"
        "Please inspect the program and press resume when you are ready to continue\n\n"
      , NULL);
    igPopTextWrapPos();
    igDummy({ 0.0f, 0.0f });
    igSameLine(0.0f, 50.0f);
    if (igButton("Abort", buttonSize)) {
      state->aborting = 1;
      grbl->feed_hold();
    }
    igSameLine(0.0f, 100.0f);
    if (igButton("Resume", buttonSize)) {
      grbl->cycle_start();
    }

    igEndPopup();
  }

  igBegin("Program", nullptr, ImGuiWindowFlags_None);

  ImVec2 scrollingRegionSize = {0, -100};
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
      gcode_line_t *parsed_line = state->parser.line(line_no);

      igText("%i", line_no); igSameLine(0, 20.0f);

      if (state->keyframes && roaring_bitmap_contains(state->keyframes, line_no)) {
        igTextColored({1.0f, 1.0f, 0.0f, 1.0f}, line->c_str());
      } else if (line_no == last_hovered_line) {
        igTextColored({1.0f, 0.0f, 1.0f, 1.0f}, line->c_str());
      // } else if (state->current_line - 1 == line_no) {
      //   igTextColored({0.5f, 0.5f, 1.0f, 1.0f}, line->c_str());
      } else if (state->current_line == line_no) {
        igTextColored({0.5f, 0.5f, 1.0f, 1.0f}, line->c_str());
      } else {
        igTextUnformatted(line->c_str(), NULL);
      }
      if (igIsItemHovered(0)) {
        // only allow double click on a line when a program is loaded and idle.
        if (igIsMouseDoubleClicked(0) && state->status == PROGRAM_STATE_LOADED) {
          state->current_line = line_no;
          printf("hrm: %s\n", state->parser.line_state_debug(state->current_line));
        }

        hovered_line = line_no;
        const char *tooltip_text = state->parser.line_state_debug(line_no);
        if (tooltip_text != NULL) {
          igBeginTooltip();
          igPushTextWrapPos(igGetFontSize() * 35.0f);
          igText("line: %i", line_no);
          igTextUnformatted(tooltip_text, NULL);
          igText(
            "(%0.3f, %0.3f, %0.3f) -> (%0.3f, %0.3f, %0.3f)",
            parsed_line->parser_state.start_x,
            parsed_line->parser_state.start_y,
            parsed_line->parser_state.start_z,
            parsed_line->parser_state.end_x,
            parsed_line->parser_state.end_y,
            parsed_line->parser_state.end_z
          );

          igPopTextWrapPos();
          igEndTooltip();
        }
      }
    }
  }
  ImGuiListClipper_End(&clipper);
  if (state->autoscroll) {
    igSetScrollYFloat(state->current_line * line_height - line_height * 10.0f);
  }

  igEndChild();

  if (state->autoscroll && igIsItemHovered(0) && igGetIO()->MouseWheel != 0) {
    state->autoscroll = 0;
  }

  igCheckbox("autoscroll", &state->autoscroll);
  igDummy({ 0.0f, 10.0f });
  igDummy({ 0.0f, 0.0f });

  if (!(state->status & PROGRAM_STATE_RUNNING)) {

    bool pressed = false;
    bool resuming = false;
    if (state->status & PROGRAM_STATE_PAUSED) {
      pressed = igButton("continue", buttonSize);
    } else if (state->current_line > 0) {
      pressed = igButton("resume", buttonSize);
      if (igIsItemHovered(0)){
        igBeginTooltip();
        igPushTextWrapPos(igGetFontSize() * 35.0f);
        igText(
          "Resume this program from the selected line (%i)\n",
          state->current_line
        );
        igPopTextWrapPos();
        igEndTooltip();
      }

      // TODO: message the user saying that they should double check the gcode
      //       that we are generating below.
      // this should move into GrblMachine as it will probably involve a bunch more work
      // than just setting the line_state as seen below.
      if (pressed) {
        state->show_resume_modal = true;
        pressed = false;
      }

      gcode_line_t *parsed_line = state->parser.line(state->current_line);

      if (state->show_resume_modal) {
        pressed = false;
        String resume_code = overlay_program_resume(grbl, state, parsed_line);
        if (resume_code.length()) {
          resuming = true;
          pressed = true;
          grbl->write(resume_code.handle);
          resume_code.destroy();
        }
      }

    } else if (state->status == PROGRAM_STATE_LOADED) {
      pressed = igButton("run", buttonSize);
    }

    if (pressed) {
      if (state->status & PROGRAM_STATE_LOADED) {
        if (!resuming) {
          grbl->cycle_start();
        }
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

  if (state->aborting) {
    const vec3 current = grbl->state->machine_position;

    if (grbl->state->state == GRBL_MACHINE_STATE_HOLD) {
      if (vec3_eq(&state->last_machine_position, &current)) {
        // ensure the coords are stable for 5 frames
        if ((state->aborting++) > 5) {
          state->pending_action = -1;
          // // TODO: can we use state->parser instead of having two parsers?
          grbl->tx_parser->reset();
          state->aborting = 0;
          grbl->soft_reset();

          if (state->status & PROGRAM_STATE_LOADED) {
            state->status = PROGRAM_STATE_LOADED;
          }
        }
      } else {
        state->aborting = 1;
        vec3_copy(&state->last_machine_position, &current);
      }
    }
  }

  if (!grbl->is_idle()) {
    igSameLine(0.0, 10.0);
    if (igButton("abort", buttonSize)) {
      // abort procedure
      // - feed hold
      // - wait for two ? queries to return the same MPos
      // - soft reset
      grbl->feed_hold();
      state->aborting = 1;
      vec3_copy(
        &state->last_machine_position,
        &grbl->state->machine_position
      );
    }
  }

  if (grbl->is_idle()) {
    igSameLine(0.0, 10.0);
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
  }

  // button for resetting the current line to 0
  {
    bool can_reset_current_line = (
      state->current_line > 0 &&
      grbl->is_idle() &&
      state->status == PROGRAM_STATE_LOADED
    );

    if (can_reset_current_line) {
      igSameLine(0.0, 10.0);
      if (igButton("reset", buttonSize)) {
        state->current_line = 0;
        state->autoscroll = true;
      }

      if (igIsItemHovered(0)){
          igBeginTooltip();
          igPushTextWrapPos(igGetFontSize() * 35.0f);
          igTextUnformatted(
            "Reset the starting point of this program\n"
            "back to the first line.\n",
            NULL
          );
          igPopTextWrapPos();
          igEndTooltip();
      }
    }
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
      "status: ( %s) line: %i action: %i vs grbl (%zu / %zu)",
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