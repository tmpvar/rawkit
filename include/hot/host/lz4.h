
#pragma once
#include <rawkit/jit.h>
#include <lz4.h>

void host_init_lz4(rawkit_jit_t *jit) {
  rawkit_jit_add_export(jit, "LZ4_versionNumber", LZ4_versionNumber);
  rawkit_jit_add_export(jit, "LZ4_versionString", LZ4_versionString);
  rawkit_jit_add_export(jit, "LZ4_compress_default", LZ4_compress_default);
  rawkit_jit_add_export(jit, "LZ4_decompress_safe", LZ4_decompress_safe);
  rawkit_jit_add_export(jit, "LZ4_compressBound", LZ4_compressBound);
  rawkit_jit_add_export(jit, "LZ4_compress_fast", LZ4_compress_fast);
  rawkit_jit_add_export(jit, "LZ4_sizeofState", LZ4_sizeofState);
  rawkit_jit_add_export(jit, "LZ4_compress_fast_extState", LZ4_compress_fast_extState);
  rawkit_jit_add_export(jit, "LZ4_compress_destSize", LZ4_compress_destSize);
  rawkit_jit_add_export(jit, "LZ4_decompress_safe_partial", LZ4_decompress_safe_partial);
  rawkit_jit_add_export(jit, "LZ4_createStream", LZ4_createStream);
  rawkit_jit_add_export(jit, "LZ4_freeStream", LZ4_freeStream);
  rawkit_jit_add_export(jit, "LZ4_resetStream_fast", LZ4_resetStream_fast);
  rawkit_jit_add_export(jit, "LZ4_loadDict", LZ4_loadDict);
  rawkit_jit_add_export(jit, "LZ4_compress_fast_continue", LZ4_compress_fast_continue);
  rawkit_jit_add_export(jit, "LZ4_saveDict", LZ4_saveDict);
  rawkit_jit_add_export(jit, "LZ4_createStreamDecode", LZ4_createStreamDecode);
  rawkit_jit_add_export(jit, "LZ4_freeStreamDecode", LZ4_freeStreamDecode);
  rawkit_jit_add_export(jit, "LZ4_setStreamDecode", LZ4_setStreamDecode);
  rawkit_jit_add_export(jit, "LZ4_decoderRingBufferSize", LZ4_decoderRingBufferSize);
  rawkit_jit_add_export(jit, "LZ4_decompress_safe_continue", LZ4_decompress_safe_continue);
  rawkit_jit_add_export(jit, "LZ4_decompress_safe_usingDict", LZ4_decompress_safe_usingDict);
  rawkit_jit_add_export(jit, "LZ4_initStream", LZ4_initStream);
  rawkit_jit_add_export(jit, "LZ4_decompress_fast", LZ4_decompress_fast);
  rawkit_jit_add_export(jit, "LZ4_decompress_fast_continue", LZ4_decompress_fast_continue);
  rawkit_jit_add_export(jit, "LZ4_decompress_fast_usingDict", LZ4_decompress_fast_usingDict);
  rawkit_jit_add_export(jit, "LZ4_resetStream", LZ4_resetStream);
}

