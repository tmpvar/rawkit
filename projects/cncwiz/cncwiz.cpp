#define CIMGUI_NO_EXPORT
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <hot/state.h>
#include <rawkit/serial.h>

#include <rawkit/string.h>

#include <stb_ds.h>

#include <vector>

struct Lines {
  gbString *handle = nullptr;
  size_t capacity = 1;
  size_t index = 0;

  Lines() {}

  size_t length() {
    return index;
  }

  // void push_copy(gbString *src) {
  //   if (index + 1 < capacity) {
  //     printf("resize lines from %zu to %zu\n", capacity, capacity * 2);
  //     this->capacity *= 2;
  //     this->handle = (gbString *)realloc(
  //       this->handle, 
  //       this->capacity * sizeof(gbString)
  //     );
  //   }

  //   this->handle[this->index++] = gb_duplicate_string(src);
  //   // this->handle = (String *)stbds_arrput((String *)this->handle, line);
  // }
};

void setup() {}

void panel_jog(SerialPort *sp) {
  igBegin(
    "Jog",
    nullptr,
    ImGuiWindowFlags_AlwaysAutoResize
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoResize
  );

  ImVec2 buttonSize = {32, 32};
  igDummy(buttonSize);
  igSameLine(0.0, 0.0);
  if (igArrowButtonEx("##Jog:Y+", ImGuiDir_Up, buttonSize,
                           ImGuiButtonFlags_None)) {
    sp->write("$J=G91Y10F1000\n");
  }

  igSameLine(0.0, 0.0);
  igDummy(buttonSize);
  igSameLine(0.0, 0.0);
  igDummy(buttonSize);
  igSameLine(0.0, 0.0);
  if (igArrowButtonEx("##Jog:Z+", ImGuiDir_Up, buttonSize,
                           ImGuiButtonFlags_None)) {
    sp->write("$J=G91Z10F1000\n");
  }

  if (igArrowButtonEx("##Jog:X-", ImGuiDir_Left, buttonSize,
                           ImGuiButtonFlags_None)) {
    sp->write("$J=G91X-10F1000\n");
  }

  igSameLine(0.0, 0.0);
  igDummy(buttonSize);

  igSameLine(0.0, 0.0);

  if (igArrowButtonEx("##Jog:X+", ImGuiDir_Right, buttonSize,
                           ImGuiButtonFlags_None)) {
    sp->write("$J=G91X+10F1000\n");
  }

  igDummy(buttonSize);
  igSameLine(0.0, 0.0);
  if (igArrowButtonEx("##Jog:Y-", ImGuiDir_Down, buttonSize,
                           ImGuiButtonFlags_None)) {
    sp->write("$J=G91Y-10F1000\n");
  }

  igSameLine(0.0, 0.0);
  igDummy(buttonSize);
  igSameLine(0.0, 0.0);
  igDummy(buttonSize);
  igSameLine(0.0, 0.0);

  if (igArrowButtonEx("##Jog:Z-", ImGuiDir_Down, buttonSize,
                           ImGuiButtonFlags_None)) {
    sp->write("$J=G91Z-10F10000\n");
  }

  igDummy(buttonSize);
  igPushStyleColorU32(ImGuiCol_Button, 0xFF0000FF);
  ImVec2 stopButtonSize = {0.0, 0.0};
  igGetContentRegionAvail(&stopButtonSize);
  stopButtonSize.y = 40.0;
  if (igButton("STOP", stopButtonSize)) {
    sp->write("\x85");
  }
  igPopStyleColor(1);

  // handle escape
  if (igIsWindowFocused(0) && igIsKeyReleased(0x100)) {
    sp->write("\x85");
  }
  igEnd();
}

void panel_terminal(SerialPort *sp, StringList *lines) {
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

  ImGuiListClipper clipper;
  ImGuiListClipper_Begin(&clipper, lines->length(), 20.0);
  String *line = nullptr;
  while (ImGuiListClipper_Step(&clipper)) {
    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
      line = lines->item(line_no);
      igTextUnformatted(line->c_str(), NULL);
    }
  }
  ImGuiListClipper_End(&clipper);

  igSetScrollHereY(1.0f);
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
    printf(">> %s\n", input);
    // sp_rx_lines.push_back(">> " + string(input));
    sp->write(input);
    sp->write("\n");
    input[0] = 0;

    // Demonstrate keeping auto focus on the input box
    if (
      igIsItemHovered(0) ||
      (igIsWindowFocused(0) && !igIsAnyItemActive() && !igIsMouseClicked(0, false))
    ) {
      igSetKeyboardFocusHere(-1); // Auto focus previous widget
    }
  }
  igEnd();
}


struct Line {
  gbString handle = NULL;
};

const Line defaultLine = {0};

void loop() {
  SerialPort sp("COM3");
  igBegin("hot", 0, 0);

  igText("sp: %u, %u", sp.id, sp.available());
  String *rx = (String *)hotState(
    /* id            */ 1,
    /* size          */ sizeof(String),
    /* initial value */ nullptr
  );

  Line *line = (Line *)hotState(
      /* id            */ 2,
      /* size          */ sizeof(Line),
      /* initial value */ (void *)&defaultLine);

  // Lines *lines = (Lines *)hotState(
  //     /* id            */ 3,
  //     /* size          */ sizeof(Lines),
  //     /* initial value */ nullptr);

  StringList *lines = (StringList *)hotState(
      /* id            */ 3,
      /* size          */ sizeof(Lines),
      /* initial value */ nullptr);

  if (sp.available()) {
    printf("recv: ");
    while (sp.available()) {
      char c = sp.read();
      if (c == '\r') {
        continue;
      }

      if (c == '\n') {
        if (rx->length() == 0) {
          continue;
        }

        lines->push(rx);
        printf("LINE: %s\n", rx->handle);
        rx->clear();
        continue;
      }

      // const char add[2] = {c, 0};
      //line->handle = gb_append_cstring(line->handle, add);
      rx->append_char(c);
      // 
      // if (c == '\n') {

      //   const char str[2] = { c, 0 };
      //   printf("line: %p, line->handle: %p\n", line, line->handle);
      //   if (line->handle == NULL) {
      //     printf("make handle a string\n");
      //     line->handle = gb_make_string(str);
      //   } else {
      //     printf("append value to string\n");
      //     line->handle = gb_append_cstring(line->handle, str);
      //   }
      //   printf("here\n");
      //   // String s(rx);
      //   // printf("after\n");
        
      //   //lines->push_copy(rx->handle);//stbds_arrpush(lines, &s);
      //   // rx->clear();
      // }
      // printf("%c", c);
      // rx->append(c);
    }
    printf("\n");
  }


  uint32_t *ticker = (uint32_t *) hotState(
  /* id            */  2,
  /* size          */  sizeof(uint32_t),
  /* initial value */  0
  );
  
  igText("it's hot in here: %i sp: %u", (*ticker)++);
  igText("recv buffer\n%s", rx->handle);

  panel_jog(&sp);
  panel_terminal(&sp, lines);
}
