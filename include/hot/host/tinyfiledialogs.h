#pragma once

#include <rawkit/jit.h>

#include <tinyfiledialogs.h>

static void host_init_tinyfiledialogs(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "tinyfd_beep", tinyfd_beep);
  rawkit_jit_add_export(jit, "tinyfd_notifyPopup", tinyfd_notifyPopup);
  rawkit_jit_add_export(jit, "tinyfd_messageBox", tinyfd_messageBox);
  rawkit_jit_add_export(jit, "tinyfd_inputBox", tinyfd_inputBox);
  rawkit_jit_add_export(jit, "tinyfd_saveFileDialog", tinyfd_saveFileDialog);
  rawkit_jit_add_export(jit, "tinyfd_openFileDialog", tinyfd_openFileDialog);
  rawkit_jit_add_export(jit, "tinyfd_selectFolderDialog", tinyfd_selectFolderDialog);
  rawkit_jit_add_export(jit, "tinyfd_colorChooser", tinyfd_colorChooser);
}
