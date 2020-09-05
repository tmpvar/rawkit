#define CIMGUI_NO_EXPORT
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

#include <rawkit/rawkit.h>
#include <rawkit/hot/state.h>
#include <rawkit/string.h>

#include "grbl/machine.h"
#include "panels/probe.h"
#include "panels/program.h"


void setup() {}

void panel_jog(GrblMachine *grbl) {
  igBegin(
    "Jog",
    nullptr,
    ImGuiWindowFlags_AlwaysAutoResize
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoResize
  );

  float jogSpeed = 10000.0f;
  float jogDistance = 10.0f;

  ImVec2 buttonSize = {32, 32};
  igDummy(buttonSize);
  igSameLine(0.0, 1.0);
  if (igArrowButtonEx("##Jog:Y+", ImGuiDir_Up, buttonSize, ImGuiButtonFlags_None)) {
    grbl->jog({0.0f, jogDistance, 0.0}, jogSpeed);
  }

  igSameLine(0.0, 5.0);
  igDummy(buttonSize);
  igSameLine(0.0, 5.0);
  igDummy(buttonSize);
  igSameLine(0.0, 1.0);
  if (igArrowButtonEx("##Jog:Z+", ImGuiDir_Up, buttonSize, ImGuiButtonFlags_None)) {
    grbl->jog({0.0f, 0.0f, jogDistance}, jogSpeed);
  }

  if (igArrowButtonEx("##Jog:X-", ImGuiDir_Left, buttonSize, ImGuiButtonFlags_None)) {
    grbl->jog({-jogDistance, 0.0f, 0.0f}, jogSpeed);
  }

  igSameLine(0.0, 2.0);
  igDummy(buttonSize);

  igSameLine(0.0, 0.0);

  if (igArrowButtonEx("##Jog:X+", ImGuiDir_Right, buttonSize, ImGuiButtonFlags_None)) {
    grbl->jog({jogDistance, 0.0f, 0.0f}, jogSpeed);
  }

  igDummy(buttonSize);
  igSameLine(0.0, 0.0);
  if (igArrowButtonEx("##Jog:Y-", ImGuiDir_Down, buttonSize, ImGuiButtonFlags_None)) {
    grbl->jog({0.0f, -jogDistance, 0.0f}, jogSpeed);
  }

  igSameLine(0.0, 0.0);
  igDummy(buttonSize);
  igSameLine(0.0, 0.0);
  igDummy(buttonSize);
  igSameLine(0.0, 0.0);

  if (igArrowButtonEx("##Jog:Z-", ImGuiDir_Down, buttonSize, ImGuiButtonFlags_None)) {
    grbl->jog({0.0f, 0.0f, -jogDistance}, jogSpeed);
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

    grbl->user_input(input);
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
  if (igButton("unlock", buttonSize)) {
    grbl->unlock();
  }
  if (igButton("cycle start", buttonSize)) {
    grbl->cycle_start();
  }
  if (igButton("feed hold", buttonSize)) {
    grbl->feed_hold();
  }

  igText("");
  igText("machine actions (%zu/%zu)", grbl->state->action_complete, grbl->state->action_pending);

  igEnd();
}


bool overlay_disconnected(GrblMachine *grbl) {
  if (!grbl->sp.valid()) {
    igOpenPopup("overlay::disconnected", 0);

    igBeginPopupModal(
      "overlay::disconnected",
      0,
      ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoMove
    );

    igPushTextWrapPos(igGetFontSize() * 30.0f);
      igTextUnformatted(
        "grbl is not connected...\n\n"
      , NULL);
    igPopTextWrapPos();
    igEndPopup();
    return true;
  }
  return false;
}

bool overlay_initializing(GrblMachine *grbl) {
  if (!grbl->state->initialized) {
    igOpenPopup("overlay::initializing", 0);

    igBeginPopupModal(
      "overlay::initializing",
      0,
      ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoMove
    );

    igPushTextWrapPos(igGetFontSize() * 30.0f);
      igTextUnformatted(
        "grbl is initializing...\n\n"
      , NULL);
    igPopTextWrapPos();
    igEndPopup();
    return true;
  }
  return false;
}

bool overlay_alarm(GrblMachine *grbl) {
  if (grbl->state->state == GRBL_MACHINE_STATE_ALARM) {
    igOpenPopup("overlay::alarm", 0);

    igBeginPopupModal(
      "overlay::alarm",
      0,
      ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoMove
    );

    igPushTextWrapPos(igGetFontSize() * 25.0f);
      igTextUnformatted(
        "grbl is in an alarm state!\n\n"
        "You should probably click the Home button below.\n\n"
      , NULL);
    igPopTextWrapPos();

    ImVec2 buttonSize = {75, 20};
    ImVec2 spacerSize = {10, 0};

    // Home button
    {
      igPushStyleColorVec4(ImGuiCol_Button, ImVec4{0.0f, 0.7f, 0.0f, 1.0f});
      ImVec2 homeButtonSize = {0.0, 0.0};
      igGetContentRegionAvail(&homeButtonSize);
      homeButtonSize.y = 40.0;
      if (igButton("Home", homeButtonSize)) {
        grbl->home();
      }
      igPopStyleColor(1);
    }

    igPushTextWrapPos(igGetFontSize() * 25.0f);
    igTextUnformatted(
      "\n"
      "The Unlock button is here in case you need to unlock and "
      "jog the machine before it can be safely homed.\n\n",
      NULL
    );
    igPopTextWrapPos();

    // Home button
    {
      igPushStyleColorVec4(ImGuiCol_Button, ImVec4{1.0f, 0.3f, 0.0f, 1.0f});
      ImVec2 unlockButtonSize = {0.0, 0.0};
      igGetContentRegionAvail(&unlockButtonSize);
      unlockButtonSize.y = 40.0;
      if (igButton("Unlock", unlockButtonSize)) {
        grbl->unlock();
      }
      igPopStyleColor(1);
    }

    igDummy({0.0f, 0.0f});
    igEndPopup();
    return true;
  }
  return false;
}

bool overlay_homing(GrblMachine *grbl) {
  if (grbl->state->state == GRBL_MACHINE_STATE_HOMING) {
    igOpenPopup("overlay::homing", 0);

    igBeginPopupModal(
      "overlay::homing",
      0,
      ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoMove
    );

    igPushTextWrapPos(igGetFontSize() * 25.0f);
      igTextUnformatted(
        "grbl is homing...\n\n"
      , NULL);
    igPopTextWrapPos();

    // Cancel homing
    {
      igPushStyleColorVec4(ImGuiCol_Button, ImVec4{1.0f, 0.0f, 0.0f, 1.0f});
      ImVec2 cancelButtonSize = {0.0, 0.0};
      igGetContentRegionAvail(&cancelButtonSize);
      cancelButtonSize.y = 40.0;
      if (igButton("Cancel", cancelButtonSize) || igIsKeyReleased(0x100)) {
        grbl->soft_reset();
      }

      igPopStyleColor(1);
    }

    igEndPopup();
    return true;
  }
  return false;
}


void loop() {
  // this implicitly ticks the underlying serialport
  GrblMachine grbl;


  bool booting = (
    overlay_disconnected(&grbl) ||
    overlay_initializing(&grbl)
  );

  if (booting) {
    return;
  }

  panel_status(&grbl);
  panel_terminal(&grbl);


  bool unlocked = (
    !overlay_alarm(&grbl) &&
    !overlay_homing(&grbl)
  );


  if (unlocked) {
    // grab bag of functionality until it all finds a home
    panel_random(&grbl);


    panel_jog(&grbl);

    panel_probe(&grbl);
    panel_program(&grbl);
  }
}
