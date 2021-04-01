
#ifdef CPU_HOST
  #define PNANOVDB_C
#else
  #include "shared.h"
  #define PNANOVDB_BUF_CUSTOM
  #define PNANOVDB_GLSL
  #define pnanovdb_buf_t int
  #define pnanovdb_grid_type_t uint
  #define PNANOVDB_GRID_TYPE_GET(grid_typeIn, nameIn) pnanovdb_grid_type_constants[grid_typeIn].nameIn
  uint pnanovdb_buf_read_uint32(pnanovdb_buf_t buf, uint byte_offset) {
    uint idx = (byte_offset >> 2u);
    if (idx >= ubo.scene.data_size) {
      return 0;
    }
    return pnanovdb_buf_data[idx];
  }

  uvec2 pnanovdb_buf_read_uint64(pnanovdb_buf_t buf, uint byte_offset) {
    uvec2 ret;
    ret.x = pnanovdb_buf_read_uint32(buf, byte_offset + 0u);
    ret.y = pnanovdb_buf_read_uint32(buf, byte_offset + 4u);
    return ret;
  }
  #include "nanovdb/pnanovdb-glsl.h"
#endif

