#define CIMGUI_NO_EXPORT
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

#include <rawkit/rawkit.h>
#include <rawkit/hot/state.h>
#include <rawkit/serial.h>
#include <rawkit/string.h>
#include <rawkit/grbl/parser.h>
#include <rawkit/gcode/parser.h>

#define GRBL_MACHINE_HOT_STATE_OFFSET 0xFF000000

typedef grbl_vec3_t vec3;

void vec3_copy(vec3 *dst, const vec3 *src) {
  dst->x = src->x;
  dst->y = src->y;
  dst->z = src->z;
}

struct GrblCommandPair {
  gcode_line_t input;
  uint64_t token_start;
  uint64_t token_end;
};

struct GrblMachine;

typedef void (*AsyncProcessFn)(GrblMachine *grbl);

typedef struct AsyncProcess_t {
  uint32_t current_sequence_index;
  AsyncProcessFn *sequence;
} AsyncProcess;

struct GrblState {
  uint64_t token_index;
  double last_fetch;
  bool initialized;


  uint64_t _index;

  // track pairs of requests and responses in a queue. This can be used
  // for blocking operations like $h, probing, etc.. and can also inform
  // when we need to send $# to update WCS and similar (e.g., after a user updates
  // the working coordinates via G10)

  grbl_machine_state last_state;
  grbl_machine_state state;
  
  vec3 machine_position;
  vec3 wcs_G54;
  vec3 wcs_G55;
  vec3 wcs_G56;
  vec3 wcs_G57;
  vec3 wcs_G58;
  vec3 wcs_G59;
  vec3 pos_G28;
  vec3 pos_G30;
  float TLO;
  vec3 PRB;
  vec3 cso_G92;

  AsyncProcess *process_queue;
};

struct GrblMachine {
  SerialPort sp;
  StringList *log;
  String *line;
  GrblParser *rx_parser;
  GCODEParser *tx_parser;
  GrblState *state;

  GrblMachine() {
    this->sp.open("COM3");

    this->line = (String *)hotState(
      GRBL_MACHINE_HOT_STATE_OFFSET,
      sizeof(String),
      nullptr
    );

    this->log = (StringList *)hotState(
      GRBL_MACHINE_HOT_STATE_OFFSET + 1,
      sizeof(StringList),
      nullptr
    );

    this->rx_parser = (GrblParser *)hotState(
      GRBL_MACHINE_HOT_STATE_OFFSET + 2,
      sizeof(GrblParser),
      nullptr
    );

    this->tx_parser = (GCODEParser *)hotState(
      GRBL_MACHINE_HOT_STATE_OFFSET + 3,
      sizeof(GCODEParser),
      nullptr
    );

    this->state = (GrblState *)hotState(
      GRBL_MACHINE_HOT_STATE_OFFSET + 4,
      sizeof(GrblState),
      nullptr
    );

    double now = rawkit_now();
    double last_fetch = this->state->last_fetch;
    double pollingRate = .2;

    if (this->state->initialized) {
      if (last_fetch == 0.0) {
        this->write("$#\n");
      }

      if (last_fetch == 0.0 || now - last_fetch > pollingRate) {
        this->write("?\n");
        this->state->last_fetch = now + 10.0f;
      }

      if (this->state->state != GRBL_MACHINE_STATE_ALARM) {
        if (this->state->last_state != this->state->state) {
          this->write("$#\n");
          this->state->last_state = this->state->state;
        }
      }
    }

    if (!this->sp.valid()) {
      this->state->initialized = false;
    }

    // tokenize the output from grbl
    while (this->sp.available()) {
      char c = this->sp.read();

      if (c == '\r') {
        continue;
      }

      this->rx_parser->read(c);

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


    uint64_t token_count = this->rx_parser->token_count();
    if (token_count > this->state->token_index) {

      for (; this->state->token_index < token_count; this->state->token_index++) {
        const grbl_response_token_t *token = this->rx_parser->token(this->state->token_index);
        switch (token->type) {
          case   GRBL_TOKEN_TYPE_WELCOME:
            this->state->last_fetch = 0.0;
            this->state->initialized = true;
            break;

          case   GRBL_TOKEN_TYPE_MACHINE_STATE:
            this->state->state = token->machine_state.value;
            break;

          case GRBL_TOKEN_TYPE_MACHINE_POSITION:
            vec3_copy(
              &this->state->machine_position,
              &token->vec3
            );
            this->state->last_fetch = now;
            break;

          case GRBL_TOKEN_TYPE_MESSAGE_POS_G28:
            vec3_copy(
              &this->state->pos_G28,
              &token->vec3
            );
            break;
          case GRBL_TOKEN_TYPE_MESSAGE_POS_G30:
            vec3_copy(
              &this->state->pos_G30,
              &token->vec3
            );
            break;
          case GRBL_TOKEN_TYPE_MESSAGE_WCO_G54:
            vec3_copy(
              &this->state->wcs_G54,
              &token->vec3
            );
            break;
          case GRBL_TOKEN_TYPE_MESSAGE_WCO_G55:
            vec3_copy(
              &this->state->wcs_G55,
              &token->vec3
            );
            break;
          case GRBL_TOKEN_TYPE_MESSAGE_WCO_G56:
            vec3_copy(
              &this->state->wcs_G56,
              &token->vec3
            );
            break;
          case GRBL_TOKEN_TYPE_MESSAGE_WCO_G57:
            vec3_copy(
              &this->state->wcs_G57,
              &token->vec3
            );
            break;
          case GRBL_TOKEN_TYPE_MESSAGE_WCO_G58:
            vec3_copy(
              &this->state->wcs_G58,
              &token->vec3
            );
            break;
          case GRBL_TOKEN_TYPE_MESSAGE_WCO_G59:
            vec3_copy(
              &this->state->wcs_G59,
              &token->vec3
            );
            break;
          case GRBL_TOKEN_TYPE_MESSAGE_CSO_G92:
            vec3_copy(
              &this->state->cso_G92,
              &token->vec3
            );
            break;
          case GRBL_TOKEN_TYPE_MESSAGE_TLO:
            this->state->TLO = token->f32;
            break;
          case GRBL_TOKEN_TYPE_MESSAGE_PRB:
            vec3_copy(
              &this->state->PRB,
              &token->vec3
            );
            break;
          
          case GRBL_TOKEN_TYPE_STATUS_REPORT:
            // reset the '?' ticker
            this->state->last_fetch = now;
            break;

          default:
            continue;
        }
      }
    }
  }

  void write(const char *str) {
    String tx;
    tx.append_c_str(">> ");
    tx.append_c_str(str);
    printf("WRITE: '%s'\n", str);
    if (this->tx_parser->push(str) == GCODE_RESULT_ERROR) {
      tx.append_c_str("  ERROR: invalid gcode");
      this->log->push(&tx);
      return;
    }

    this->log->push(&tx);
    this->sp.write(str);
  }

  void write(const float f) {
    char buffer [100];
    int cx;

    snprintf(buffer, 100, "%f", f);
    this->write((const char *)buffer);
  }

  void move_to(const vec3 pos, const float speed) {
    this->write("X");
    this->write(pos.x);
    this->write("Y");
    this->write(pos.y);
    this->write("Z");
    this->write(pos.z);
    this->write("F");
    this->write(speed);
    this->write("\n");
  }

  void probe(const vec3 from, const vec3 to, const float seek, const float feed) {
    AsyncProcess process;

    // TODO: move these operations into a

    // move into position
    this->write("G1");
    this->move_to(from, seek);

    // probe towards the target
    this->write("G38.2");
    this->move_to(to, seek);   

    // probe away from the target
    this->write("G38.4");
    this->move_to(from, feed);

    // now we have the result...

    // move back to where we started
    this->write("G1");
    this->move_to(from, seek);

  }

};

void setup() {

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

  if (igButton("PROBE", stopButtonSize)) {
    grbl->write("G1Z0F7000\n");
    grbl->write("G1X-196Y-20F7000\n");
    grbl->probe(
      (vec3){-196.0f, -20.0f, 0.0f},
      (vec3){-196.0f, -20.0f, -100.0f},
      400.0f,
      10.0f
    );
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

void loop() {
  // this implicitly ticks the underlying serialport
  GrblMachine grbl;

  panel_jog(&grbl);
  panel_terminal(&grbl);
  panel_status(&grbl);
}
