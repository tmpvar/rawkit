#pragma once

#include <stdint.h>
#include <rawkit/serial.h>
#include <rawkit/grbl/parser.h>
#include <rawkit/gcode/parser.h>

#define GRBL_MACHINE_HOT_STATE_OFFSET 0xFF000000

typedef grbl_vec3_t vec3;

void vec3_copy(vec3 *dst, const vec3 *src) {
  dst->x = src->x;
  dst->y = src->y;
  dst->z = src->z;
}


bool vec3_eq(const vec3 *a, const vec3 *b) {
  return (
    a->x == b->x &&
    a->y == b->y &&
    a->z == b->z
  );
}

enum grbl_probing_status {
  PROBING_NONE,
  PROBING_MOVE_TO_START,
  PROBING_PROBE_FAST_TO_END,
  PROBING_PROBE_FAST_TO_START,
  PROBING_PROBE_SLOW_TO_END,
  PROBING_PROBE_SLOW_TO_START,
  PROBING_MOVE_BACK_TO_START,
  PROBING_COMPLETE,
  PROBING_ERROR,
  PROBING_STATUS_COUNT
};

const char *grbl_probing_status_names[PROBING_STATUS_COUNT] = {
  "NONE",
  "MOVE_TO_START",
  "PROBE_FAST_TO_END",
  "PROBE_FAST_TO_START",
  "PROBE_SLOW_TO_END",
  "PROBE_SLOW_TO_START",
  "MOVE_BACK_TO_START",
  "COMPLETE",
  "ERROR"
};

typedef int64_t grbl_action_id;

struct grbl_probing_state {
  grbl_probing_status status;
  grbl_action_id action_id;
  vec3 from;
  vec3 to;
  vec3 result;
  float feed;
  float seek;
  float rapid;
};

struct GrblState {
  uint64_t token_index;
  double last_fetch;
  bool initialized;

  // track pairs of requests and responses in a queue. This can be used
  // for blocking operations like $h, probing, etc.. and can also inform
  // when we need to send $# to update WCS and similar (e.g., after a user updates
  // the working coordinates via G10)

  grbl_machine_state last_state;
  grbl_machine_state state;
  grbl_probing_state probing;

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

  grbl_action_id action_pending;
  grbl_action_id action_complete;

  String tx_line;
};

#define GRBL_ACTION_INVALID -1

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
    double pollingRate = (
      this->state->state == GRBL_MACHINE_STATE_JOG ||
      // reduce the amount of time we need to wait during
      // a job abort, which uses feed hold to dequeue the
      // current motion.
      this->state->state == GRBL_MACHINE_STATE_HOLD
    )
      ? .001
      : .2;

    if (this->state->initialized) {
      if (last_fetch == 0.0) {
        this->write("$#");
        this->end_action();
      }

      if (last_fetch == 0.0 || now - last_fetch > pollingRate) {
        this->status();
        this->state->last_fetch = now + 10.0f;
      }

      if (this->state->state != GRBL_MACHINE_STATE_ALARM) {
        if (this->state->last_state != this->state->state) {
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

        // TODO: add a UI button for filtering out status responses
        if (this->line->handle[0] != '<') {
          this->log->push(String("<< ").append_string(this->line));
        }
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
          case GRBL_TOKEN_TYPE_ALARM:
            this->state->initialized = false;
            this->state->state = GRBL_MACHINE_STATE_ALARM;
            break;
          case GRBL_TOKEN_TYPE_STATUS:
            this->state->action_complete++;
            // TODO: sending commands while unconditionally paused sometimes
            // causes the complete count to be larger than the pending count
            if (this->state->action_complete > this->state->action_pending) {
              this->state->action_complete = this->state->action_pending;
            }
            break;

          case   GRBL_TOKEN_TYPE_WELCOME:
            this->state->last_fetch = 0.0;
            this->state->initialized = true;
            break;

          case GRBL_TOKEN_TYPE_MACHINE_STATE:
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

  void status() {
    // bypass the logging mechanisms
    // TODO: these messages should be collected for debugging purposes,
    //       but the ui should provide a way to filter them out.
    this->sp.write("?");
  }

  void write(const char *str) {
    const size_t l = strlen(str);
    for (size_t i=0; i<l; i++) {
      this->state->tx_line.append_char(str[i]);
      if (str[i] != '\n') {
        continue;
      }
      // printf("line: %s", this->state->tx_line.handle);
      this->tx_parser->push(this->state->tx_line.handle);
      this->sp.write(this->state->tx_line.handle);
      this->log->push(&this->state->tx_line);
      this->state->tx_line.clear();
    }
  }

  void write(const float f) {
    char buffer [100];
    int cx;

    snprintf(buffer, 100, "%.3f", f);
    this->write((const char *)buffer);
  }

  void user_input(const char *str) {
    this->state->action_pending += 1;
    this->write(str);
    this->write("\n");
  }

  grbl_action_id end_action() {
    this->write("\n");
    if (this->state->initialized) {
      this->state->action_pending += 1;
    }
    return this->state->action_pending;
  }

  bool is_action_complete(grbl_action_id id) {
    return this->state->action_complete >= id;
  }

  grbl_action_id move_to(const vec3 pos, const float speed) {
    this->write("X");
    this->write(pos.x);
    this->write("Y");
    this->write(pos.y);
    this->write("Z");
    this->write(pos.z);
    this->write("F");
    this->write(speed);
    return this->end_action();
  }

  grbl_action_id move_line(const vec3 from, const vec3 to, const float speed) {
    if (to.x != from.x) {
      this->write("X");
      this->write(to.x);
    }

    if (to.y != from.y) {
      this->write("Y");
      this->write(to.y);
    }

    if (to.z != from.z) {
      this->write("Z");
      this->write(to.z);
    }

    this->write("F");
    this->write(speed);
    return this->end_action();
  }

  grbl_action_id home() {
    this->write("$h");
    this->state->state = GRBL_MACHINE_STATE_HOME;
    return this->end_action();
  }

  void write_command(const uint8_t c) {
    this->sp.write(&c, 1);
  }

  void soft_reset() {
    this->write_command(0x18);
    this->state->action_complete = 0;
    this->state->action_pending = 0;
    this->state->probing.status = PROBING_NONE;
  }

  void feed_hold() {
    this->write("!");
    this->end_action();
  }

  void cycle_start() {
    this->write("~");
    this->end_action();
  }

  void jog(const vec3 dir, const float speed) {
    if (
      this->state->state == GRBL_MACHINE_STATE_IDLE ||
      this->state->state == GRBL_MACHINE_STATE_JOG
    ) {
      this->write("$J=G91");
      this->move_to(dir, speed);
      this->state->state = GRBL_MACHINE_STATE_JOG;
    }
  }

  void unlock() {
    this->user_input("$X");
  }

  bool probe_in_progress() {
    return (
  		this->state->probing.status > PROBING_NONE &&
	  	this->state->probing.status <= PROBING_COMPLETE
	  );
  }

  void probe_init(const vec3 from, const vec3 to, const float rapid, const float seek, const float feed) {
    this->state->probing.from = from;
    this->state->probing.to = to;
    this->state->probing.feed = feed;
    this->state->probing.seek = seek;
    this->state->probing.rapid = rapid;

    this->state->probing.status = PROBING_MOVE_TO_START;
    this->write("G53");
    this->state->probing.action_id = this->move_to(
      from,
      rapid
    );
  }

  bool probe() {
    if (this->state == NULL || !this->probe_in_progress()) {
      return false ;
    }

    if (!this->is_action_complete(state->probing.action_id)) {
      return false;
    }

    switch (state->probing.status) {
      case PROBING_ERROR:
        // TODO: how to clear this error?
        break;

      case PROBING_COMPLETE:
        this->state->probing.status = PROBING_NONE;
        return true;

      case PROBING_NONE:
        // TODO: arriving here should probably show a warning as the caller
        //       of this function is responsible for calling probe_in_progress()
        break;

    case PROBING_MOVE_TO_START:
        this->state->probing.status = PROBING_PROBE_FAST_TO_END;
        this->write("G38.2"); // move until contact
        this->state->probing.action_id = this->move_line(
          this->state->probing.from,
          this->state->probing.to,
          this->state->probing.seek
        );
        break;

      case PROBING_PROBE_FAST_TO_END:
        this->state->probing.status = PROBING_PROBE_FAST_TO_START;
        this->write("G38.4"); // move until contact is cleared
        this->state->probing.action_id = this->move_line(
          this->state->probing.to,
          this->state->probing.from,
          this->state->probing.seek
        );
        break;

      case PROBING_PROBE_FAST_TO_START:
        this->state->probing.status = PROBING_PROBE_SLOW_TO_END;
        this->write("G38.2"); // move until contact is cleared
        this->state->probing.action_id = this->move_line(
          this->state->probing.from,
          this->state->probing.to,
          this->state->probing.feed
        );
        break;

      case PROBING_PROBE_SLOW_TO_END:
        this->state->probing.status = PROBING_PROBE_SLOW_TO_START;
        this->write("G38.4"); // move until contact is cleared
        this->state->probing.action_id = this->move_line(
          this->state->probing.to,
          this->state->probing.from,
          this->state->probing.feed * 0.1f
        );
        break;

      case PROBING_PROBE_SLOW_TO_START:
        this->state->probing.status = PROBING_MOVE_BACK_TO_START;
        this->write("G53G1"); // coordinated move (aka feed)
        this->state->probing.action_id = this->move_to(
          this->state->probing.from,
          this->state->probing.rapid
        );
        break;

      case PROBING_MOVE_BACK_TO_START:
        this->state->probing.status = PROBING_COMPLETE;
        this->state->probing.action_id = -1;
        vec3_copy(
          &this->state->probing.result,
          &this->state->PRB
        );
        break;

      default:
        this->state->probing.status = PROBING_ERROR;
        printf("ERROR: probing FSM went out of bounds\n");
        return true;
    }

    return false;
  }

  const char *tx_parser_state_debug() {
    if (this->tx_parser == nullptr) {
      return nullptr;
    }

    return this->tx_parser->state_debug();
  }

  const char *tx_line_parser_state_debug(uint64_t line_number) {
    if (this->tx_parser == nullptr) {
      return nullptr;
    }

    return this->tx_parser->line_state_debug(line_number);
  }
};