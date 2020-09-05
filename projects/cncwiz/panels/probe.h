#include <cimgui.h>
#include "../grbl/machine.h"

void panel_probe(GrblMachine *grbl) {

  if (!grbl->is_idle()) {
    return;
  }

  igBegin("Probe", nullptr, 0);

  ImVec2 wideButtonSize = {0.0, 0.0};
  igGetContentRegionAvail(&wideButtonSize);
  wideButtonSize.y = 40.0;

  if (!grbl->probe_in_progress()) {
    if (igButton("Probe Tool Height", wideButtonSize)) {
      grbl->probe_init(
        { -196.0f, -20.0f, 0.0f },
        { -196.0f, -20.0f, -100.0f },
        10000.0f,
        800.0f,
        100.0f
      );
    }
  } else {


    if (grbl->probe()) {
      printf("probe complete: %f, %f, %f\n",
        grbl->state->probing.result.x,
        grbl->state->probing.result.y,
        grbl->state->probing.result.z
      );
    }
  }
  igText("probing status: %s\n", grbl_probing_status_names[grbl->state->probing.status]);
  igEnd();
}