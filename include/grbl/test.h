#include "parser.h"

TEST_CASE("basic parser tests") {
  GrblParser p;
  // empty lines are ignored
  REQUIRE(p.read('\n') == GRBL_FALSE);
  REQUIRE(p.handle->loc == 0);

  // carriage returns are ignored
  REQUIRE(p.read('\r') == GRBL_FALSE);
  REQUIRE(p.handle->loc == 0);

  // ok
  REQUIRE(p.read('o') == GRBL_FALSE);
  REQUIRE(p.handle->loc == 1);
  REQUIRE(p.read('k') == GRBL_FALSE);
  REQUIRE(p.handle->loc == 2);
  REQUIRE(p.read('\r') == GRBL_FALSE);
  REQUIRE(p.handle->loc == 2);
  REQUIRE(p.read('\n') == GRBL_TRUE);
  REQUIRE(p.handle->loc == 0);
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_STATUS);
  REQUIRE(p.handle->tokens[0].error_code == 0);
}

TEST_CASE("error:\\d+") {
  GrblParser p;

  REQUIRE(p.read('e') == GRBL_FALSE);
  REQUIRE(p.read('r') == GRBL_FALSE);
  REQUIRE(p.read('r') == GRBL_FALSE);
  REQUIRE(p.read('o') == GRBL_FALSE);
  REQUIRE(p.read('r') == GRBL_FALSE);
  REQUIRE(p.read(':') == GRBL_FALSE);
  REQUIRE(p.read('9') == GRBL_FALSE);
  REQUIRE(p.read('\r') == GRBL_FALSE);
  REQUIRE(p.read('\n') == GRBL_TRUE);
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_STATUS);
  REQUIRE(p.handle->tokens[0].error_code == 9);
}

TEST_CASE("welcome message") {
  GrblParser p;

  REQUIRE(p.read('G') == GRBL_FALSE);
  REQUIRE(p.read('r') == GRBL_FALSE);
  REQUIRE(p.read('b') == GRBL_FALSE);
  REQUIRE(p.read('l') == GRBL_FALSE);
  REQUIRE(p.read(' ') == GRBL_FALSE);
  REQUIRE(p.read('1') == GRBL_FALSE);
  REQUIRE(p.read('.') == GRBL_FALSE);
  REQUIRE(p.read('2') == GRBL_FALSE);
  REQUIRE(p.read('g') == GRBL_FALSE);
  REQUIRE(p.read('\r') == GRBL_FALSE);
  REQUIRE(p.read('\n') == GRBL_TRUE);
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_WELCOME);
  REQUIRE(p.handle->tokens[1].type == GRBL_TOKEN_TYPE_VERSION);
  REQUIRE(p.handle->tokens[1].version.major == 1);
  REQUIRE(p.handle->tokens[1].version.minor == 2);
  REQUIRE(p.handle->tokens[1].version.letter == 'g');
}

TEST_CASE("status report (Idle)") {
  GrblParser p;

  int r = p.read(
    "<Idle"                     // machine state
    "|MPos:1.234,5.678,9.012"   // Working coordinate position
    "|WPos:9.012,5.678,1.234"   // Working coordinate position
    "|Bf:15,128"                // buffer state
    "|Ln:99999"                 // line number
    "|F:500"                    // feed rate (spindle disabled in config.h)
    "|FS:1000,12000"            // feed + spindle
    "|WCO:-100.000,0.000,0.000" // working coordinate offset
    "|Pn:XYZAPDHRS"             // pin inputs
    "|Ov:123,213,121"           // overrides
    "|A:SFM"                    // accessory state
    "|A:C"                      // accessory state
    ">\r\n"
  );
  REQUIRE(r == GRBL_TRUE);
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_STATUS_REPORT);
  
  REQUIRE(p.handle->tokens[1].type == GRBL_TOKEN_TYPE_MACHINE_STATE);
  REQUIRE(p.handle->tokens[1].machine_state.value == GRBL_MACHINE_STATE_IDLE);
  REQUIRE(p.handle->tokens[1].machine_state.code == 0);

  REQUIRE(p.handle->tokens[2].type == GRBL_TOKEN_TYPE_MACHINE_POSITION);
  REQUIRE(p.handle->tokens[2].vec3.x == 1.234f);
  REQUIRE(p.handle->tokens[2].vec3.y == 5.678f);
  REQUIRE(p.handle->tokens[2].vec3.z == 9.012f);

  REQUIRE(p.handle->tokens[3].type == GRBL_TOKEN_TYPE_WCO_POSITION);
  REQUIRE(p.handle->tokens[3].vec3.x == 9.012f);
  REQUIRE(p.handle->tokens[3].vec3.y == 5.678f);
  REQUIRE(p.handle->tokens[3].vec3.z == 1.234f);

  REQUIRE(p.handle->tokens[4].type == GRBL_TOKEN_TYPE_BUFFER_STATE);
  REQUIRE(p.handle->tokens[4].buffer_state.available_blocks == 15);
  REQUIRE(p.handle->tokens[4].buffer_state.available_bytes == 128);

  REQUIRE(p.handle->tokens[5].type == GRBL_TOKEN_TYPE_LINE_NUMBER);
  REQUIRE(p.handle->tokens[5].u32 == 99999);

  REQUIRE(p.handle->tokens[6].type == GRBL_TOKEN_TYPE_CURRENT_FEED_AND_SPINDLE);
  REQUIRE(p.handle->tokens[6].feed_and_spindle.feed == 500);
  REQUIRE(p.handle->tokens[6].feed_and_spindle.spindle == 0);

  REQUIRE(p.handle->tokens[7].type == GRBL_TOKEN_TYPE_CURRENT_FEED_AND_SPINDLE);
  REQUIRE(p.handle->tokens[7].feed_and_spindle.feed == 1000);
  REQUIRE(p.handle->tokens[7].feed_and_spindle.spindle == 12000);

  REQUIRE(p.handle->tokens[8].type == GRBL_TOKEN_TYPE_WCO_OFFSET);
  REQUIRE(p.handle->tokens[8].vec3.x == -100.000f);
  REQUIRE(p.handle->tokens[8].vec3.y ==  0.000f);
  REQUIRE(p.handle->tokens[8].vec3.z ==  0.000f);

  REQUIRE(p.handle->tokens[9].type == GRBL_TOKEN_TYPE_INPUT_PIN_STATE);
  REQUIRE(p.handle->tokens[9].pins.limit_x == 1);
  REQUIRE(p.handle->tokens[9].pins.limit_y == 1);
  REQUIRE(p.handle->tokens[9].pins.limit_z == 1);
  REQUIRE(p.handle->tokens[9].pins.limit_a == 1);
  REQUIRE(p.handle->tokens[9].pins.probe == 1);
  REQUIRE(p.handle->tokens[9].pins.door == 1);
  REQUIRE(p.handle->tokens[9].pins.hold == 1);
  REQUIRE(p.handle->tokens[9].pins.soft_reset == 1);
  REQUIRE(p.handle->tokens[9].pins.cycle_start == 1);

  REQUIRE(p.handle->tokens[10].type == GRBL_TOKEN_TYPE_OVERRIDES);
  REQUIRE(p.handle->tokens[10].overrides.feed == 1.23f);
  REQUIRE(p.handle->tokens[10].overrides.rapids == 2.13f);
  REQUIRE(p.handle->tokens[10].overrides.spindle == 1.21f);

  REQUIRE(p.handle->tokens[11].type == GRBL_TOKEN_TYPE_ACCESSORY_STATE);
  REQUIRE(p.handle->tokens[11].accessories.spindle == GRBL_SPINDLE_DIRECTION_CW);
  REQUIRE(p.handle->tokens[11].accessories.flood_coolant == 1);
  REQUIRE(p.handle->tokens[11].accessories.mist_coolant == 1);

  REQUIRE(p.handle->tokens[12].type == GRBL_TOKEN_TYPE_ACCESSORY_STATE);
  REQUIRE(p.handle->tokens[12].accessories.spindle == GRBL_SPINDLE_DIRECTION_CCW);
  REQUIRE(p.handle->tokens[12].accessories.flood_coolant == 0);
  REQUIRE(p.handle->tokens[12].accessories.mist_coolant == 0);
}

// TEST_CASE("status report (Idle WPos $10=0)") {
//   GrblParser p;

//   int r = p.read(
//     "<Idle"                     // machine state
//     "|WPos:1.234,5.678,9.012"   // Working coordinate position
//     "|Bf:15,128"                // buffer state
//     "|Ln:99999"                 // line number
//     "|F:500"                    // feed rate (spindle disabled in config.h)
//     "|FS:1000, 12000"           // feed + spindle
//     "|WCO:-100.000,0.000,0.000" // working coordinate offset
//     "|Pn:XYZPDHRS"
//     "|Ov:100,100,100"
//     ">"
//   );

//   REQUIRE(r == GRBL_TRUE);
//   REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_STATUS_REPORT);
  
//   REQUIRE(p.handle->tokens[1].type == GRBL_TOKEN_TYPE_MACHINE_STATE);
//   REQUIRE(p.handle->tokens[1].machine_state.value == GRBL_MACHINE_STATE_IDLE);
//   REQUIRE(p.handle->tokens[1].machine_state.code == 0);

//   REQUIRE(p.handle->tokens[2].type == GRBL_TOKEN_TYPE_WCO_POSITION);
//   REQUIRE(p.handle->tokens[2].vec3.x == 1.234f);
//   REQUIRE(p.handle->tokens[2].vec3.y == 5.678f);
//   REQUIRE(p.handle->tokens[2].vec3.z == 9.012f);
// }

// TEST_CASE("status report (Door:1)") {
//   GrblParser p;

//   int r = p.read("<Door:1|MPos:1.234,5.678,9.012|FS:0,0|WCO:-100.000,0.000,0.000>\r\n");
//   REQUIRE(r == GRBL_TRUE);
//   REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_STATUS_REPORT);
//   REQUIRE(p.handle->tokens[1].type == GRBL_TOKEN_TYPE_MACHINE_STATE);
//   REQUIRE(p.handle->tokens[1].machine_state.value == GRBL_MACHINE_STATE_DOOR);
//   REQUIRE(p.handle->tokens[1].machine_state.code == 1);

//   REQUIRE(p.handle->tokens[2].type == GRBL_TOKEN_TYPE_MACHINE_POSITION);
//   REQUIRE(p.handle->tokens[2].vec3.x == 1.234f);
//   REQUIRE(p.handle->tokens[2].vec3.y == 5.678f);
//   REQUIRE(p.handle->tokens[2].vec3.z == 9.012f);
// }

// <Idle|MPos:0.000,0.000,0.000|FS:0,0|Ov:100,100,100>
// <Idle|MPos:0.000,0.000,0.000|FS:0,0>