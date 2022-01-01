#pragma once
struct rawkit_jit_t {};
static void _rawkit_jit_add_export(rawkit_jit_t *jit, const char *name, void *addr) {}
#define rawkit_jit_add_export(jit, name, addr) (_rawkit_jit_add_export(jit, name, (void *)&addr))
