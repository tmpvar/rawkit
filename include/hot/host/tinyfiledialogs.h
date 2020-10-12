#include <rawkit/jit.h>

#include <tinyfiledialogs.h>

void host_init_tinyfiledialogs(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "tinyfd_beep", (void *)tinyfd_beep);
  rawkit_jit_add_export(jit, "tinyfd_notifyPopup", (void *)tinyfd_notifyPopup);
  rawkit_jit_add_export(jit, "tinyfd_messageBox", (void *)tinyfd_messageBox);
  rawkit_jit_add_export(jit, "tinyfd_inputBox", (void *)tinyfd_inputBox);
  rawkit_jit_add_export(jit, "tinyfd_saveFileDialog", (void *)tinyfd_saveFileDialog);
  rawkit_jit_add_export(jit, "tinyfd_openFileDialog", (void *)tinyfd_openFileDialog);
  rawkit_jit_add_export(jit, "tinyfd_selectFolderDialog", (void *)tinyfd_selectFolderDialog);
  rawkit_jit_add_export(jit, "tinyfd_colorChooser", (void *)tinyfd_colorChooser);
}
