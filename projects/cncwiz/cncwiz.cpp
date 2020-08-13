#define CIMGUI_NO_EXPORT
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

#include <rawkit/hot/state.h>
#include <rawkit/serial.h>
#include <rawkit/string.h>

struct GrblMachine {
  SerialPort sp;
  StringList *log;
  String *line;

  GrblMachine() {
    this->sp.open("COM3");

    this->line = (String *)hotState( 1, sizeof(String), nullptr);
    this->log = (StringList *)hotState(3, sizeof(StringList), nullptr);

    while (this->sp.available()) {
      char c = this->sp.read();

      if (c == '\r') {
        continue;
      }

      if (c == '\n') {
        if (this->line->length() == 0) {
          continue;
        }

        if (this->line->length() == 1 && this->line->c_str()[0] == '\n') {
          continue;
        }
        
        this->log->push(String("<< ").append_string(this->line));
        this->line->clear();
        continue;
      }

      this->line->append_char(c);
    }
  }

  void write(const char *str) {
    if (strlen(str) == 1 && str[0] == '\n') {
      return;
    }

    String tx;
    tx.append_c_str(">> ");
    tx.append_c_str(str);

    this->log->push(&tx);
    this->sp.write(str);
  }

};

void setup() {}

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
  igText("output from grbl:");
 
  ImVec2 scrollingRegionSize = {0, -50};
  igBeginChildStr(
    "ScrollingRegion",
    scrollingRegionSize,
    false,
    ImGuiWindowFlags_HorizontalScrollbar
  );

  static int last_line_count = 0;
  ImGuiListClipper clipper;
  ImGuiListClipper_Begin(&clipper, grbl->log->length(), 16.0);
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
    "SEND",
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

void loop() {
  // this implicitly ticks the underlying serialport
  GrblMachine grbl;

  panel_jog(&grbl);
  panel_terminal(&grbl);
}
