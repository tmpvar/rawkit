#pragma once

#include <rawkit/core.h>
#include <string>

// when parsing args while running w/ jit
void rawkit_core_init_args(int argc, char **argv, std::string &program_file);

// without a jit
void rawkit_core_init_args(int argc, char **argv);