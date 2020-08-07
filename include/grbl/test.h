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

TEST_CASE("ALARM:\\d+") {
  GrblParser p;
  REQUIRE(p.read("ALARM:9\r\n") == GRBL_TRUE);
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_ALARM);
  REQUIRE(p.handle->tokens[0].alarm_code == GRBL_ALARM_HOMING_FAIL_NO_CONTACT);
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

TEST_CASE("settings") {
  GrblParser p;
  int r = p.read(
    "$0=10\r\n"
    "$1=25\r\n"
    "$2=2\r\n"
    "$3=3\r\n"
    "$4=0\r\n"
    "$5=1\r\n"
    "$6=0\r\n"
    "$10=1\r\n"
    "$11=0.010\r\n"
    "$12=0.002\r\n"
    "$13=0\r\n"
    "$20=0\r\n"
    "$21=0\r\n"
    "$22=1\r\n"
    "$23=23\r\n"
    "$24=100.000\r\n"
    "$25=2000.000\r\n"
    "$26=250\r\n"
    "$27=1.000\r\n"
    "$30=1000\r\n"
    "$31=0\r\n"
    "$32=0\r\n"
    "$100=40.000\r\n"
    "$101=32.000\r\n"
    "$102=302.360\r\n"
    "$110=7000.000\r\n"
    "$111=3000.000\r\n"
    "$112=3000.000\r\n"
    "$120=120.000\r\n"
    "$121=121.000\r\n"
    "$122=122.000\r\n"
    "$130=130.000\r\n"
    "$131=131.000\r\n"
    "$132=132.000\r\n"
    "ok\r\n"
  );
  REQUIRE(r == GRBL_TRUE);

  // $0 step pulse time
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[0].setting == GRBL_SETTING_STEP_PULSE_TIME);
  REQUIRE(p.handle->tokens[1].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[1].u32 == 10);

  // $1 step idle delay
  REQUIRE(p.handle->tokens[2].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[2].setting == GRBL_SETTING_STEP_IDLE_DELAY);
  REQUIRE(p.handle->tokens[3].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[3].u32 == 25);

  // $2 invert step pulse mask
  REQUIRE(p.handle->tokens[4].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[4].setting == GRBL_SETTING_INVERT_STEP_PULSE_MASK);
  REQUIRE(p.handle->tokens[5].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[5].u8 == 2);
  
  // $3 invert step direction mask
  REQUIRE(p.handle->tokens[6].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[6].setting == GRBL_SETTING_INVERT_STEP_DIRECTION_MASK);
  REQUIRE(p.handle->tokens[7].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[7].u8 == 3);

  // $4 invert step enable pin
  REQUIRE(p.handle->tokens[8].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[8].setting == GRBL_SETTING_INVERT_STEP_ENABLE_PIN);
  REQUIRE(p.handle->tokens[9].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[9].u8 == 0);

  // $5 invert limit pins
  REQUIRE(p.handle->tokens[10].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[10].setting == GRBL_SETTING_INVERT_LIMIT_PINS);
  REQUIRE(p.handle->tokens[11].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[11].u8 == 1);

  // $6 invert probe pin
  REQUIRE(p.handle->tokens[12].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[12].setting == GRBL_SETTING_INVERT_PROBE_PIN);
  REQUIRE(p.handle->tokens[13].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[13].u8 == 0);

  // $10 status report options mask
  REQUIRE(p.handle->tokens[14].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[14].setting == GRBL_SETTING_STATUS_REPORT_OPTIONS_MASK);
  REQUIRE(p.handle->tokens[15].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[15].u8 == 1);

  // $11 junction deviation
  REQUIRE(p.handle->tokens[16].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[16].setting == GRBL_SETTING_JUCTION_DEVIATION);
  REQUIRE(p.handle->tokens[17].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[17].f32 == 0.010f);

  // $12 arc tolerance
  REQUIRE(p.handle->tokens[18].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[18].setting == GRBL_SETTING_ARC_TOLERANCE);
  REQUIRE(p.handle->tokens[19].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[19].f32 == 0.002f);

  // $13 report in inches 
  REQUIRE(p.handle->tokens[20].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[20].setting == GRBL_SETTING_REPORT_IN_INCHES);
  REQUIRE(p.handle->tokens[21].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[21].u8 == 0);

  // $20 soft limits enable
  REQUIRE(p.handle->tokens[22].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[22].setting == GRBL_SETTING_SOFT_LIMITS_ENABLE);
  REQUIRE(p.handle->tokens[23].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[23].u8 == 0);

  // $21 hard limits enable
  REQUIRE(p.handle->tokens[24].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[24].setting == GRBL_SETTING_HARD_LIMITS_ENABLE);
  REQUIRE(p.handle->tokens[25].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[25].u8 == 0);

  // $22 homing cycle enable
  REQUIRE(p.handle->tokens[26].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[26].setting == GRBL_SETTING_HOMING_CYCLE_ENABLE);
  REQUIRE(p.handle->tokens[27].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[27].u8 == 1);

  // $23 invert homing direction mask
  REQUIRE(p.handle->tokens[28].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[28].setting == GRBL_SETTING_INVERT_HOMING_DIRECTION_MASK);
  REQUIRE(p.handle->tokens[29].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[29].u8 == 23);

  // $24 homing locate feed rate
  REQUIRE(p.handle->tokens[30].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[30].setting == GRBL_SETTING_HOMING_LOCATE_FEED_RATE);
  REQUIRE(p.handle->tokens[31].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[31].f32 == 100.000f);

  // $25 homing search seek rate
  REQUIRE(p.handle->tokens[32].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[32].setting == GRBL_SETTING_HOMING_SEARCH_SEEK_RATE);
  REQUIRE(p.handle->tokens[33].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[33].f32 == 2000.000f);

  // $26 homing switch debounce delay
  REQUIRE(p.handle->tokens[34].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[34].setting == GRBL_SETTING_HOMING_SWITCH_DEBOUNCE_DELAY);
  REQUIRE(p.handle->tokens[35].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[35].u32 == 250);

  // $27 homing switch pull off distance
  REQUIRE(p.handle->tokens[36].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[36].setting == GRBL_SETTING_HOMING_SWITCH_PULL_OFF_DISTANCE);
  REQUIRE(p.handle->tokens[37].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[37].f32 == 1.000f);

  // $30 maximum spindle speed
  REQUIRE(p.handle->tokens[38].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[38].setting == GRBL_SETTING_MAXIMUM_SPINDLE_SPEED);
  REQUIRE(p.handle->tokens[39].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[39].u32 == 1000);

  // $31 minimum spindle speed
  REQUIRE(p.handle->tokens[40].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[40].setting == GRBL_SETTING_MINIMUM_SPINDLE_SPEED);
  REQUIRE(p.handle->tokens[41].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[41].u32 == 0);

  // $32 laser mode enable
  REQUIRE(p.handle->tokens[42].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[42].setting == GRBL_SETTING_LASER_MODE_ENABLE);
  REQUIRE(p.handle->tokens[43].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[43].u8 == 0);

  // $100 X axis travel resolution
  REQUIRE(p.handle->tokens[44].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[44].setting == GRBL_SETTING_X_AXIS_TRAVEL_RESOLUTION);
  REQUIRE(p.handle->tokens[45].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[45].f32 == 40.0f);

  // $101 Y axis travel resolution
  REQUIRE(p.handle->tokens[46].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[46].setting == GRBL_SETTING_Y_AXIS_TRAVEL_RESOLUTION);
  REQUIRE(p.handle->tokens[47].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[47].f32 == 32.000f);

  // $102 Z axis travel resolution
  REQUIRE(p.handle->tokens[48].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[48].setting == GRBL_SETTING_Z_AXIS_TRAVEL_RESOLUTION);
  REQUIRE(p.handle->tokens[49].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[49].f32 == 302.360f);

  // $110 X axis max rate
  REQUIRE(p.handle->tokens[50].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[50].setting == GRBL_SETTING_X_AXIS_MAXIMUM_RATE);
  REQUIRE(p.handle->tokens[51].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[51].f32 == 7000.0f);

  // $111 Y axis max rate
  REQUIRE(p.handle->tokens[52].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[52].setting == GRBL_SETTING_Y_AXIS_MAXIMUM_RATE);
  REQUIRE(p.handle->tokens[53].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[53].f32 == 3000.0f);

  // $112 Z axis max rate
  REQUIRE(p.handle->tokens[54].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[54].setting == GRBL_SETTING_Z_AXIS_MAXIMUM_RATE);
  REQUIRE(p.handle->tokens[55].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[55].f32 == 3000.0f);

  // $120 X axis acceleration
  REQUIRE(p.handle->tokens[56].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[56].setting == GRBL_SETTING_X_AXIS_ACCELERATION);
  REQUIRE(p.handle->tokens[57].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[57].f32 == 120.0f);

  // $121 Y axis acceleration
  REQUIRE(p.handle->tokens[58].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[58].setting == GRBL_SETTING_Y_AXIS_ACCELERATION);
  REQUIRE(p.handle->tokens[59].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[59].f32 == 121.0f);

  // $122 Z axis acceleration
  REQUIRE(p.handle->tokens[60].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[60].setting == GRBL_SETTING_Z_AXIS_ACCELERATION);
  REQUIRE(p.handle->tokens[61].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[61].f32 == 122.0f);

  // $130 X axis max travel
  REQUIRE(p.handle->tokens[62].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[62].setting == GRBL_SETTING_X_AXIS_MAXIMUM_TRAVEL);
  REQUIRE(p.handle->tokens[63].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[63].f32 == 130.0f);

  // $131 Y axis max travel
  REQUIRE(p.handle->tokens[64].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[64].setting == GRBL_SETTING_Y_AXIS_MAXIMUM_TRAVEL);
  REQUIRE(p.handle->tokens[65].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[65].f32 == 131.0f);

  // $132 Z axis max travel
  REQUIRE(p.handle->tokens[66].type == GRBL_TOKEN_TYPE_SETTING_KEY);
  REQUIRE(p.handle->tokens[66].setting == GRBL_SETTING_Z_AXIS_MAXIMUM_TRAVEL);
  REQUIRE(p.handle->tokens[67].type == GRBL_TOKEN_TYPE_SETTING_VALUE);
  REQUIRE(p.handle->tokens[67].f32 == 132.0f);
}

TEST_CASE("[MSG:...]") {
  GrblParser p;
  int r = p.read("[MSG:'$H'|'$X' to unlock]\r\n");
  REQUIRE(r == GRBL_TRUE);
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_MESSAGE_MSG);
  REQUIRE(strcmp(p.handle->tokens[0].str, "'$H'|'$X' to unlock") == 0);
}

TEST_CASE("[VER:...]") {
  GrblParser p;
  int r = p.read("[VER:1.1h.20190830:]\r\n");
  REQUIRE(r == GRBL_TRUE);
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_MESSAGE_VER);
  REQUIRE(strcmp(p.handle->tokens[0].str, "1.1h.20190830:") == 0);
}

TEST_CASE("[OPT:...]") {
  GrblParser p;
  int r = p.read("[OPT:V,15,128]\r\n");
  REQUIRE(r == GRBL_TRUE);
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_MESSAGE_OPT);
  REQUIRE(
    strcmp(p.handle->tokens[0].str, "V,15,128") == 0
  );
}

TEST_CASE("[HLP:...]") {
  GrblParser p;
  int r = p.read(
    "[HLP:$$ $# $G $I $N $x=val $Nx=line $J=line $SLP $C $X $H ~ ! ? ctrl-x]\r\n"
  );
  REQUIRE(r == GRBL_TRUE);
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_MESSAGE_HLP);
  REQUIRE(
      strcmp(
        p.handle->tokens[0].str,
        "$$ $# $G $I $N $x=val $Nx=line $J=line $SLP $C $X $H ~ ! ? ctrl-x"
      ) == 0
  );
}

TEST_CASE("[GC:...]") {
  GrblParser p;
  int r = p.read("[GC:G0 G54 G17 G21 G90 G94 M5 M9 T4 F1200 S24000]\r\n");
  REQUIRE(r == GRBL_TRUE);
  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_MESSAGE_GC);

  grbl_gcode_state *s = &p.handle->tokens[0].gcode_state;

  REQUIRE(s->motion_mode == G0);
  REQUIRE(s->coordinate_system == G54);
  REQUIRE(s->plane == G17);
  REQUIRE(s->units == G21);
  REQUIRE(s->distance_mode == G90);
  REQUIRE(s->feed_rate_mode == G94);
  REQUIRE(s->spindle_state == M5);
  REQUIRE(s->coolant_state == M9);
  REQUIRE(s->tool_index == 4);
  REQUIRE(s->feed == 1200);
  REQUIRE(s->spindle_rpm == 24000);
}

TEST_CASE("$# response") {
  GrblParser p;
  int r = p.read(
    "[G54:-100.000,0.000,0.000]\r\n"
    "[G55:102.000,1.000,0.999]\r\n"
    "[G56:70.000,10.000,0.999]\r\n"
    "[G57:70.000,10.000,0.999]\r\n"
    "[G58:70.000,1.000,33.599]\r\n"
    "[G59:70.000,1.000,0.999]\r\n"
    "[G28:0.000,0.000,0.000]\r\n"
    "[G30:-100.000,0.000,-20.000]\r\n"
    "[G92:0.000,0.000,0.000]\r\n"
    "[TLO:0.000]\r\n"
    "[PRB:1.000,2.000,3.000:1]\r\n"
    "ok\r\n"
  );
  REQUIRE(r == GRBL_TRUE);

  REQUIRE(p.handle->tokens[0].type == GRBL_TOKEN_TYPE_MESSAGE_WCO_G54);
  REQUIRE(p.handle->tokens[0].vec3.x == -100.0f);
  REQUIRE(p.handle->tokens[0].vec3.y == 0.0f);
  REQUIRE(p.handle->tokens[0].vec3.z == 0.0f);

  REQUIRE(p.handle->tokens[1].type == GRBL_TOKEN_TYPE_MESSAGE_WCO_G55);
  REQUIRE(p.handle->tokens[1].vec3.x == 102.0f);
  REQUIRE(p.handle->tokens[1].vec3.y == 1.0f);
  REQUIRE(p.handle->tokens[1].vec3.z == 0.999f);

  REQUIRE(p.handle->tokens[2].type == GRBL_TOKEN_TYPE_MESSAGE_WCO_G56);
  REQUIRE(p.handle->tokens[2].vec3.x == 70.0f);
  REQUIRE(p.handle->tokens[2].vec3.y == 10.0f);
  REQUIRE(p.handle->tokens[2].vec3.z == 0.999f);

  REQUIRE(p.handle->tokens[3].type == GRBL_TOKEN_TYPE_MESSAGE_WCO_G57);
  REQUIRE(p.handle->tokens[3].vec3.x == 70.0f);
  REQUIRE(p.handle->tokens[3].vec3.y == 10.0f);
  REQUIRE(p.handle->tokens[3].vec3.z == 0.999f);

  REQUIRE(p.handle->tokens[4].type == GRBL_TOKEN_TYPE_MESSAGE_WCO_G58);
  REQUIRE(p.handle->tokens[4].vec3.x == 70.0f);
  REQUIRE(p.handle->tokens[4].vec3.y == 1.0f);
  REQUIRE(p.handle->tokens[4].vec3.z == 33.599f);

  REQUIRE(p.handle->tokens[5].type == GRBL_TOKEN_TYPE_MESSAGE_WCO_G59);
  REQUIRE(p.handle->tokens[5].vec3.x == 70.0f);
  REQUIRE(p.handle->tokens[5].vec3.y == 1.0f);
  REQUIRE(p.handle->tokens[5].vec3.z == 0.999f);

  REQUIRE(p.handle->tokens[6].type == GRBL_TOKEN_TYPE_MESSAGE_POS_G28);
  REQUIRE(p.handle->tokens[6].vec3.x == 0.0f);
  REQUIRE(p.handle->tokens[6].vec3.y == 0.0f);
  REQUIRE(p.handle->tokens[6].vec3.z == 0.0f);

  REQUIRE(p.handle->tokens[7].type == GRBL_TOKEN_TYPE_MESSAGE_POS_G30);
  REQUIRE(p.handle->tokens[7].vec3.x == -100.0f);
  REQUIRE(p.handle->tokens[7].vec3.y == 0.0f);
  REQUIRE(p.handle->tokens[7].vec3.z == -20.0f);

  REQUIRE(p.handle->tokens[8].type == GRBL_TOKEN_TYPE_MESSAGE_CSO_G92);
  REQUIRE(p.handle->tokens[8].vec3.x == 0.0f);
  REQUIRE(p.handle->tokens[8].vec3.y == 0.0f);
  REQUIRE(p.handle->tokens[8].vec3.z == 0.0f);

  REQUIRE(p.handle->tokens[9].type == GRBL_TOKEN_TYPE_MESSAGE_TLO);
  REQUIRE(p.handle->tokens[9].f32 == 0.0f);

  REQUIRE(p.handle->tokens[10].type == GRBL_TOKEN_TYPE_MESSAGE_PRB);
  REQUIRE(p.handle->tokens[10].probe_state.vec3.x == 1.0f);
  REQUIRE(p.handle->tokens[10].probe_state.vec3.y == 2.0f);
  REQUIRE(p.handle->tokens[10].probe_state.vec3.z == 3.0f);
  REQUIRE(p.handle->tokens[10].probe_state.success == 1);
}
