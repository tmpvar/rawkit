#define CIMGUI_NO_EXPORT
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

#include <rawkit/rawkit.h>
#include <rawkit/hot/state.h>
#include <rawkit/string.h>

#include "grbl/machine.h"
#include "panels/probe.h"

void setup() {
  printf("loaded\n");
}

void panel_jog(GrblMachine *grbl) {
  igBegin(
    "Jog",
    nullptr,
    ImGuiWindowFlags_AlwaysAutoResize
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoResize
  );

  ImVec2 buttonSize = {32, 32};
  igDummy(buttonSize);
  igSameLine(0.0, 1.0);
  if (igArrowButtonEx("##Jog:Y+", ImGuiDir_Up, buttonSize,
                           ImGuiButtonFlags_None)) {
    grbl->write("$J=G91Y+10F10000\n");
  }

  igSameLine(0.0, 5.0);
  igDummy(buttonSize);
  igSameLine(0.0, 5.0);
  igDummy(buttonSize);
  igSameLine(0.0, 1.0);
  if (igArrowButtonEx("##Jog:Z+", ImGuiDir_Up, buttonSize,
                           ImGuiButtonFlags_None)) {
    grbl->write("$J=G91Z+10F10000\n");
  }

  if (igArrowButtonEx("##Jog:X-", ImGuiDir_Left, buttonSize,
                           ImGuiButtonFlags_None)) {
    grbl->write("$J=G91X-10F10000\n");
  }

  igSameLine(0.0, 2.0);
  igDummy(buttonSize);

  igSameLine(0.0, 0.0);

  if (igArrowButtonEx("##Jog:X+", ImGuiDir_Right, buttonSize,
                           ImGuiButtonFlags_None)) {
    grbl->write("$J=G91X+10F10000\n");
  }

  igDummy(buttonSize);
  igSameLine(0.0, 0.0);
  if (igArrowButtonEx("##Jog:Y-", ImGuiDir_Down, buttonSize,
                           ImGuiButtonFlags_None)) {
    grbl->write("$J=G91Y-10F10000\n");
  }

  igSameLine(0.0, 0.0);
  igDummy(buttonSize);
  igSameLine(0.0, 0.0);
  igDummy(buttonSize);
  igSameLine(0.0, 0.0);

  if (igArrowButtonEx("##Jog:Z-", ImGuiDir_Down, buttonSize,
                           ImGuiButtonFlags_None)) {
    grbl->write("$J=G91Z-10F100000\n");
  }

  igDummy(buttonSize);
  igPushStyleColorU32(ImGuiCol_Button, 0xFF0000FF);
  ImVec2 stopButtonSize = {0.0, 0.0};
  igGetContentRegionAvail(&stopButtonSize);
  stopButtonSize.y = 40.0;
  if (igButton("STOP", stopButtonSize)) {
    grbl->write("\x85");
  }
  igPopStyleColor(1);

  // handle escape
  if (igIsWindowFocused(0) && igIsKeyReleased(0x100)) {
    grbl->write("\x85");
  }

  igEnd();
}

void panel_terminal(GrblMachine *grbl) {
  char input[1024] = {0};
  igBegin("Grbl Terminal", 0, 0); 
  ImVec2 scrollingRegionSize = {0, -50};
  igBeginChildStr(
    "ScrollingRegion",
    scrollingRegionSize,
    false,
    ImGuiWindowFlags_HorizontalScrollbar
  );

  static int last_line_count = 0;
  ImGuiListClipper clipper;
  ImGuiListClipper_Begin(&clipper, grbl->log->length(), 18.0);
  String *line = nullptr;
  while (ImGuiListClipper_Step(&clipper)) {
    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
      line = grbl->log->item(line_no);
      igTextUnformatted(line->c_str(), NULL);
    }
  }
  ImGuiListClipper_End(&clipper);

  // TODO: this behavior can be improved by only auto-scrolling when
  //       the user hasn't scrolled away from the bottom
  if (last_line_count != grbl->log->length()) {
    igSetScrollHereY(1.0f);
    last_line_count = grbl->log->length();
  }

  igEndChild();
  igSeparator();
  bool textSubmitted = igInputText(
    "##SEND",
    input,
    1024,
    ImGuiInputTextFlags_EnterReturnsTrue,
    NULL,
    NULL
  );

  if (textSubmitted) {
    String tx;
    tx.set_c_str(input);
    tx.append_c_str("\n");

    grbl->write(tx.handle);
    input[0] = 0;

    // keep the input box focused
    if (igIsItemHovered(0) ||
      (igIsWindowFocused(0) && !igIsAnyItemActive() && !igIsMouseClicked(0, false))
    ) {
      igSetKeyboardFocusHere(-1); // Auto focus previous widget
    }
  }
  igEnd();
}

void panel_status(GrblMachine *grbl) {
  igBegin("Status", 0, 0);

  const GrblState *state = grbl->state;
  igText("STATE: %s", grbl_machine_state_str[state->state]);

  igText("");
  igText("MACHINE ABSOLUTE POSITION");
  igText("ABS (%f, %f, %f)",
    state->machine_position.x,
    state->machine_position.y,
    state->machine_position.z
  );

  igText("");
  igText("WORK COORDINATE SYSTEMS");

  igText("1 G54 (%f, %f, %f)", state->wcs_G54.x, state->wcs_G54.y, state->wcs_G54.z);
  igText("2 G55 (%f, %f, %f)", state->wcs_G55.x, state->wcs_G55.y, state->wcs_G55.z);
  igText("3 G56 (%f, %f, %f)", state->wcs_G56.x, state->wcs_G56.y, state->wcs_G56.z);
  igText("4 G57 (%f, %f, %f)", state->wcs_G57.x, state->wcs_G57.y, state->wcs_G57.z);
  igText("5 G58 (%f, %f, %f)", state->wcs_G58.x, state->wcs_G58.y, state->wcs_G58.z);
  igText("6 G59 (%f, %f, %f)", state->wcs_G59.x, state->wcs_G59.y, state->wcs_G59.z);

  igText("");
  igText("Predefined Positions");
  igText("G28 (%f, %f, %f)", state->pos_G28.x, state->pos_G28.y, state->pos_G28.z);
  igText("G30 (%f, %f, %f)", state->pos_G30.x, state->pos_G30.y, state->pos_G30.z);

  igText("");
  igText("TOOL OFFSET");
  igText("TLO (%f)", state->TLO);

  igText("");
  igText("LAST PROBE POSITION");
  igText("PRB (%f, %f, %f)", state->PRB.x, state->PRB.y, state->PRB.z);

  igEnd();
}

void panel_random(GrblMachine *grbl) {
  igBegin("random", 0, 0);
  ImVec2 buttonSize = {0.0, 0.0};
  igGetContentRegionAvail(&buttonSize);
  buttonSize.y = 40.0;
  if (igButton("home", buttonSize)) {
    grbl->home();
  }
  if (igButton("reset", buttonSize)) {
    grbl->soft_reset();
  }

  igText("");
  igText("machine actions (%zu/%zu)", grbl->state->action_complete, grbl->state->action_pending);

  igEnd();
}

void loop() {
  // this implicitly ticks the underlying serialport
  GrblMachine grbl;
  
  // grab bag of functionality until it all finds a home
  panel_random(&grbl);

  panel_jog(&grbl);
  panel_terminal(&grbl);
  panel_status(&grbl);
  panel_probe(&grbl);
}
