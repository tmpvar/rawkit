#include <rawkit-core-internal.h>

#include <flags/flags.h>


#include <string>
#include <vector>
#include <string.h>


flags::args *program_args = nullptr;
bool skip_first = false;
void rawkit_core_init_args(int argc, char **argv) {
  program_args = new flags::args(argc, argv);
}

void rawkit_core_init_args(int argc, char **argv, std::string &program_file) {
  skip_first = true;
  program_args = new flags::args(argc, argv);
  program_file = program_args->get<std::string>(size_t(0)).value_or("");
}

size_t rawkit_arg_pos_count() {
  if (!program_args) {
    return 0;
  }
  auto l = program_args->positional().size();
  return skip_first ?  l - 1 : l;
}

bool rawkit_arg_pos_bool(size_t key, bool default_value) {
  if (skip_first) {
    key += 1;
  }
  return program_args->get<bool>(key).value_or(default_value);
}

i8 rawkit_arg_pos_i8(size_t key, i8 default_value) {
  if (skip_first) {
    key += 1;
  }

  return program_args->get<i8>(key).value_or(default_value);
}

u8 rawkit_arg_pos_u8(size_t key, u8 default_value) {
  if (skip_first) {
    key += 1;
  }

  return program_args->get<u8>(key).value_or(default_value);
}

i16 rawkit_arg_pos_i16(size_t key, i16 default_value) {
  if (skip_first) {
    key += 1;
  }

  return program_args->get<i16>(key).value_or(default_value);
}

u16 rawkit_arg_pos_u16(size_t key, u16 default_value) {
  if (skip_first) {
    key += 1;
  }

  return program_args->get<u16>(key).value_or(default_value);
}

i32 rawkit_arg_pos_i32(size_t key, i32 default_value) {
  if (skip_first) {
    key += 1;
  }

  return program_args->get<i32>(key).value_or(default_value);
}

u32 rawkit_arg_pos_u32(size_t key, u32 default_value) {
  if (skip_first) {
    key += 1;
  }

  return program_args->get<u32>(key).value_or(default_value);
}

i64 rawkit_arg_pos_i64(size_t key, i64 default_value) {
  if (skip_first) {
    key += 1;
  }

  return program_args->get<i64>(key).value_or(default_value);
}

u64 rawkit_arg_pos_u64(size_t key, u64 default_value) {
  if (skip_first) {
    key += 1;
  }

  return program_args->get<u64>(key).value_or(default_value);
}

f32 rawkit_arg_pos_f32(size_t key, f32 default_value) {
  if (skip_first) {
    key += 1;
  }

  return program_args->get<f32>(key).value_or(default_value);
}

f64 rawkit_arg_pos_f64(size_t key, f64 default_value) {
  if (skip_first) {
    key += 1;
  }

  return program_args->get<f64>(key).value_or(default_value);
}

u32 rawkit_arg_pos_string(size_t key, char *value, size_t max_len) {
  if (!program_args) {
    return 0;
  }

  if (skip_first) {
    key += 1;
  }

  auto str = program_args->get<std::string>(key).value_or("");
  auto l = str.size() > max_len - 1
    ? max_len - 1
    : str.size();

  strncpy(value, str.data(), l);
  value[l] = 0;

  return l;
}



bool rawkit_arg_bool(const char *key, bool default_value) {
  if (!program_args) {
    return default_value;
  }

  return program_args->get<bool>(key).value_or(default_value);
}

i8 rawkit_arg_i8(const char *key, i8 default_value) {
  if (!program_args) {
    return default_value;
  }

  return program_args->get<i8>(key).value_or(default_value);
}

u8 rawkit_arg_u8(const char *key, u8 default_value) {
  if (!program_args) {
    return default_value;
  }

  return program_args->get<u8>(key).value_or(default_value);
}

i16 rawkit_arg_i16(const char *key, i16 default_value) {
  if (!program_args) {
    return default_value;
  }

  return program_args->get<i16>(key).value_or(default_value);
}

u16 rawkit_arg_u16(const char *key, u16 default_value) {
  if (!program_args) {
    return default_value;
  }

  return program_args->get<u16>(key).value_or(default_value);
}

i32 rawkit_arg_i32(const char *key, i32 default_value) {
  if (!program_args) {
    return default_value;
  }

  return program_args->get<i32>(key).value_or(default_value);
}

u32 rawkit_arg_u32(const char *key, u32 default_value) {
  if (!program_args) {
    return default_value;
  }

  return program_args->get<u32>(key).value_or(default_value);
}

i64 rawkit_arg_i64(const char *key, i64 default_value) {
  if (!program_args) {
    return default_value;
  }

  return program_args->get<i64>(key).value_or(default_value);
}

u64 rawkit_arg_u64(const char *key, u64 default_value) {
  if (!program_args) {
    return default_value;
  }

  return program_args->get<u64>(key).value_or(default_value);
}

f32 rawkit_arg_f32(const char *key, f32 default_value) {
  if (!program_args) {
    return default_value;
  }

  return program_args->get<f32>(key).value_or(default_value);
}

f64 rawkit_arg_f64(const char *key, f64 default_value) {
  if (!program_args) {
    return default_value;
  }

  return program_args->get<f64>(key).value_or(default_value);
}

u32 rawkit_arg_string(const char *key, char *value, size_t max_len) {
  if (!program_args) {
    return 0;
  }

  auto str = program_args->get<std::string>(key).value_or("");
  auto l = str.size() >= max_len - 1
    ? max_len - 1
    : str.size();

  strncpy(value, str.data(), l);
  value[l] = 0;

  return l;
}
