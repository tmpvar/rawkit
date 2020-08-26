#include <tinyfiledialogs.h>

void host_init_tinyfiledialogs(JitJob *job) {
  job->addExport("tinyfd_beep", (void *)tinyfd_beep);
  job->addExport("tinyfd_notifyPopup", (void *)tinyfd_notifyPopup);
  job->addExport("tinyfd_messageBox", (void *)tinyfd_messageBox);
  job->addExport("tinyfd_inputBox", (void *)tinyfd_inputBox);
  job->addExport("tinyfd_saveFileDialog", (void *)tinyfd_saveFileDialog);
  job->addExport("tinyfd_openFileDialog", (void *)tinyfd_openFileDialog);
  job->addExport("tinyfd_selectFolderDialog", (void *)tinyfd_selectFolderDialog);
  job->addExport("tinyfd_colorChooser", (void *)tinyfd_colorChooser);
}
