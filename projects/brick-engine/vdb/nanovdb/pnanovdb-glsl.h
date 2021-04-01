// Copyright Contributors to the OpenVDB Project
// SPDX-License-Identifier: MPL-2.0

/*!
    \file   PNanoVDB.h

    \author Andrew Reidmeyer

    \brief  This file is a portable (e.g. pointer-less) C99/GLSL/HLSL port
	        of NanoVDB.h, which is compatible with most graphics APIs.
*/

// preprocessed using:  clang -DPNANOVDB_BUF_CUSTOM -DPNANOVDB_GLSL -E PNanoVDB.h

int pnanovdb_uint32_as_int32(uint v) {
  return int(v);
}
ivec2 pnanovdb_uint64_as_int64(uvec2 v) {
  return ivec2(v);
}
uvec2 pnanovdb_int64_as_uint64(ivec2 v) {
  return uvec2(v);
}
uint pnanovdb_int32_as_uint32(int v) {
  return uint(v);
}
float pnanovdb_uint32_as_float(uint v) {
  return uintBitsToFloat(v);
}
double pnanovdb_uint64_as_double(uvec2 v) {
  return packDouble2x32(uvec2(v.x, v.y));
}
uint pnanovdb_uint64_low(uvec2 v) {
  return v.x;
}
uint pnanovdb_uint64_high(uvec2 v) {
  return v.y;
}
uvec2 pnanovdb_uint32_as_uint64(uint x, uint y) {
  return uvec2(x, y);
}
uvec2 pnanovdb_uint32_as_uint64_low(uint x) {
  return uvec2(x, 0);
}
bool pnanovdb_uint64_is_equal(uvec2 a, uvec2 b) {
  return (a.x == b.x) && (a.y == b.y);
}
float pnanovdb_floor(float v) {
  return floor(v);
}
int pnanovdb_float_to_int32(float v) {
  return int(v);
}
float pnanovdb_int32_to_float(int v) {
  return float(v);
}
float pnanovdb_min(float a, float b) {
  return min(a, b);
}
float pnanovdb_max(float a, float b) {
  return max(a, b);
}
vec3 pnanovdb_vec3_uniform(float a) {
  return vec3(a, a, a);
}
vec3 pnanovdb_vec3_add(vec3 a, vec3 b) {
  return a + b;
}
vec3 pnanovdb_vec3_sub(vec3 a, vec3 b) {
  return a - b;
}
vec3 pnanovdb_vec3_mul(vec3 a, vec3 b) {
  return a * b;
}
vec3 pnanovdb_vec3_div(vec3 a, vec3 b) {
  return a / b;
}
vec3 pnanovdb_vec3_min(vec3 a, vec3 b) {
  return min(a, b);
}
vec3 pnanovdb_vec3_max(vec3 a, vec3 b) {
  return max(a, b);
}
vec3 pnanovdb_coord_to_vec3(const ivec3 coord) {
  return vec3(coord);
}
ivec3 pnanovdb_coord_uniform(int a) {
  return ivec3(a, a, a);
}
ivec3 pnanovdb_coord_add(ivec3 a, ivec3 b) {
  return a + b;
}

struct pnanovdb_address_t {
  uint byte_offset;
};

pnanovdb_address_t pnanovdb_address_offset(pnanovdb_address_t address, uint byte_offset) {
  pnanovdb_address_t ret = address;
  ret.byte_offset += byte_offset;
  return ret;
}
pnanovdb_address_t pnanovdb_address_offset_product(pnanovdb_address_t address, uint byte_offset, uint multiplier) {
  pnanovdb_address_t ret = address;
  ret.byte_offset += byte_offset * multiplier;
  return ret;
}
pnanovdb_address_t pnanovdb_address_offset64(pnanovdb_address_t address, uvec2 byte_offset) {
  pnanovdb_address_t ret = address;

  ret.byte_offset += pnanovdb_uint64_low(byte_offset);
  return ret;
}
uint pnanovdb_address_mask(pnanovdb_address_t address, uint mask) {
  return address.byte_offset & mask;
}
pnanovdb_address_t pnanovdb_address_mask_inv(pnanovdb_address_t address, uint mask) {
  pnanovdb_address_t ret = address;
  ret.byte_offset &= (~mask);
  return ret;
}
pnanovdb_address_t pnanovdb_address_null() {
  pnanovdb_address_t ret = {0};
  return ret;
}
bool pnanovdb_address_is_null(pnanovdb_address_t address) {
  return address.byte_offset == 0u;
}
bool pnanovdb_address_in_interval(pnanovdb_address_t address, pnanovdb_address_t min_address,
                                  pnanovdb_address_t max_address) {
  return address.byte_offset >= min_address.byte_offset && address.byte_offset < max_address.byte_offset;
}
uint pnanovdb_read_uint32(pnanovdb_buf_t buf, pnanovdb_address_t address) {
  return pnanovdb_buf_read_uint32(buf, address.byte_offset);
}
uvec2 pnanovdb_read_uint64(pnanovdb_buf_t buf, pnanovdb_address_t address) {
  return pnanovdb_buf_read_uint64(buf, address.byte_offset);
}
int pnanovdb_read_int32(pnanovdb_buf_t buf, pnanovdb_address_t address) {
  return pnanovdb_uint32_as_int32(pnanovdb_read_uint32(buf, address));
}
float pnanovdb_read_float(pnanovdb_buf_t buf, pnanovdb_address_t address) {
  return pnanovdb_uint32_as_float(pnanovdb_read_uint32(buf, address));
}
ivec2 pnanovdb_read_int64(pnanovdb_buf_t buf, pnanovdb_address_t address) {
  return pnanovdb_uint64_as_int64(pnanovdb_read_uint64(buf, address));
}
double pnanovdb_read_double(pnanovdb_buf_t buf, pnanovdb_address_t address) {
  return pnanovdb_uint64_as_double(pnanovdb_read_uint64(buf, address));
}
ivec3 pnanovdb_read_coord(pnanovdb_buf_t buf, pnanovdb_address_t address) {
  ivec3 ret;
  ret.x = pnanovdb_uint32_as_int32(pnanovdb_read_uint32(buf, pnanovdb_address_offset(address, 0u)));
  ret.y = pnanovdb_uint32_as_int32(pnanovdb_read_uint32(buf, pnanovdb_address_offset(address, 4u)));
  ret.z = pnanovdb_uint32_as_int32(pnanovdb_read_uint32(buf, pnanovdb_address_offset(address, 8u)));
  return ret;
}

bool pnanovdb_read_bit(pnanovdb_buf_t buf, pnanovdb_address_t address, uint bit_offset) {
  pnanovdb_address_t word_address = pnanovdb_address_mask_inv(address, 3u);
  uint bit_index = (pnanovdb_address_mask(address, 3u) << 3u) + bit_offset;
  uint value_word = pnanovdb_buf_read_uint32(buf, word_address.byte_offset);
  return ((value_word >> bit_index) & 1) != 0u;
}
float pnanovdb_read_half(pnanovdb_buf_t buf, pnanovdb_address_t address) {
  uint raw = pnanovdb_read_uint32(buf, address);
  return unpackHalf2x16(raw >> (pnanovdb_address_mask(address, 2) << 3)).x;
}
const uint pnanovdb_grid_type_value_strides_bits[13] = uint[](0, 32, 64, 16, 32, 64, 96, 192, 0, 16, 32, 1, 0);
const uint pnanovdb_grid_type_minmax_strides_bits[13] = uint[](0, 32, 64, 16, 32, 64, 96, 192, 8, 16, 32, 8, 0);
const uint pnanovdb_grid_type_minmax_aligns_bits[13] = uint[](0, 32, 64, 16, 32, 64, 32, 64, 8, 16, 32, 8, 0);
const uint pnanovdb_grid_type_stat_strides_bits[13] = uint[](0, 32, 64, 32, 32, 64, 32, 64, 8, 32, 32, 8, 0);
const uint pnanovdb_grid_type_leaf_lite[13] = uint[](0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0);
struct pnanovdb_map_t {
  float matf[9];
  float invmatf[9];
  float vecf[3];
  float taperf;
  double matd[9];
  double invmatd[9];
  double vecd[3];
  double taperd;
};

struct pnanovdb_map_handle_t {
  pnanovdb_address_t address;
};
float pnanovdb_map_get_matf(pnanovdb_buf_t buf, pnanovdb_map_handle_t p, uint index) {
  return pnanovdb_read_float(buf, pnanovdb_address_offset(p.address, 0 + 4u * index));
}
float pnanovdb_map_get_invmatf(pnanovdb_buf_t buf, pnanovdb_map_handle_t p, uint index) {
  return pnanovdb_read_float(buf, pnanovdb_address_offset(p.address, 36 + 4u * index));
}
float pnanovdb_map_get_vecf(pnanovdb_buf_t buf, pnanovdb_map_handle_t p, uint index) {
  return pnanovdb_read_float(buf, pnanovdb_address_offset(p.address, 72 + 4u * index));
}
float pnanovdb_map_get_taperf(pnanovdb_buf_t buf, pnanovdb_map_handle_t p, uint index) {
  return pnanovdb_read_float(buf, pnanovdb_address_offset(p.address, 84));
}
double pnanovdb_map_get_matd(pnanovdb_buf_t buf, pnanovdb_map_handle_t p, uint index) {
  return pnanovdb_read_double(buf, pnanovdb_address_offset(p.address, 88 + 8u * index));
}
double pnanovdb_map_get_invmatd(pnanovdb_buf_t buf, pnanovdb_map_handle_t p, uint index) {
  return pnanovdb_read_double(buf, pnanovdb_address_offset(p.address, 160 + 8u * index));
}
double pnanovdb_map_get_vecd(pnanovdb_buf_t buf, pnanovdb_map_handle_t p, uint index) {
  return pnanovdb_read_double(buf, pnanovdb_address_offset(p.address, 232 + 8u * index));
}
double pnanovdb_map_get_taperd(pnanovdb_buf_t buf, pnanovdb_map_handle_t p, uint index) {
  return pnanovdb_read_double(buf, pnanovdb_address_offset(p.address, 256));
}

struct pnanovdb_grid_t {
  uvec2 magic;
  uvec2 checksum;
  uint version;
  uint flags;
  uvec2 grid_size;
  uint grid_name[256 / 4];
  pnanovdb_map_t map;
  double world_bbox[6];
  double voxel_size[3];
  uint grid_class;
  uint grid_type;
  uvec2 blind_metadata_offset;
  uint blind_metadata_count;
  uint pad[6];
};

struct pnanovdb_grid_handle_t {
  pnanovdb_address_t address;
};
uvec2 pnanovdb_grid_get_magic(pnanovdb_buf_t buf, pnanovdb_grid_handle_t p) {
  return pnanovdb_read_uint64(buf, pnanovdb_address_offset(p.address, 0));
}
uvec2 pnanovdb_grid_get_checksum(pnanovdb_buf_t buf, pnanovdb_grid_handle_t p) {
  return pnanovdb_read_uint64(buf, pnanovdb_address_offset(p.address, 8));
}
uint pnanovdb_grid_get_version(pnanovdb_buf_t buf, pnanovdb_grid_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 16));
}
uint pnanovdb_grid_get_flags(pnanovdb_buf_t buf, pnanovdb_grid_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 20));
}
uvec2 pnanovdb_grid_get_grid_size(pnanovdb_buf_t buf, pnanovdb_grid_handle_t p) {
  return pnanovdb_read_uint64(buf, pnanovdb_address_offset(p.address, 24));
}
uint pnanovdb_grid_get_grid_name(pnanovdb_buf_t buf, pnanovdb_grid_handle_t p, uint index) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 32 + 4u * index));
}
pnanovdb_map_handle_t pnanovdb_grid_get_map(pnanovdb_buf_t buf, pnanovdb_grid_handle_t p) {
  pnanovdb_map_handle_t ret;
  ret.address = pnanovdb_address_offset(p.address, 288);
  return ret;
}
double pnanovdb_grid_get_world_bbox(pnanovdb_buf_t buf, pnanovdb_grid_handle_t p, uint index) {
  return pnanovdb_read_double(buf, pnanovdb_address_offset(p.address, 552 + 8u * index));
}
double pnanovdb_grid_get_voxel_size(pnanovdb_buf_t buf, pnanovdb_grid_handle_t p, uint index) {
  return pnanovdb_read_double(buf, pnanovdb_address_offset(p.address, 600 + 8u * index));
}
uint pnanovdb_grid_get_grid_class(pnanovdb_buf_t buf, pnanovdb_grid_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 624));
}
uint pnanovdb_grid_get_grid_type(pnanovdb_buf_t buf, pnanovdb_grid_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 628));
}
uvec2 pnanovdb_grid_get_blind_metadata_offset(pnanovdb_buf_t buf, pnanovdb_grid_handle_t p) {
  return pnanovdb_read_uint64(buf, pnanovdb_address_offset(p.address, 632));
}
uint pnanovdb_grid_get_blind_metadata_count(pnanovdb_buf_t buf, pnanovdb_grid_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 640));
}

uint pnanovdb_version_get_major(uint version) {
  return (version >> 21u) & ((1u << 11u) - 1u);
}
uint pnanovdb_version_get_minor(uint version) {
  return (version >> 10u) & ((1u << 11u) - 1u);
}
uint pnanovdb_version_get_patch(uint version) {
  return version & ((1u << 10u) - 1u);
}

struct pnanovdb_gridblindmetadata_t {
  ivec2 byte_offset;
  uvec2 element_count;
  uint flags;
  uint semantic;
  uint data_class;
  uint data_type;
  uint name[256 / 4];
};

struct pnanovdb_gridblindmetadata_handle_t {
  pnanovdb_address_t address;
};
ivec2 pnanovdb_gridblindmetadata_get_byte_offset(pnanovdb_buf_t buf, pnanovdb_gridblindmetadata_handle_t p) {
  return pnanovdb_read_int64(buf, pnanovdb_address_offset(p.address, 0));
}
uvec2 pnanovdb_gridblindmetadata_get_element_count(pnanovdb_buf_t buf, pnanovdb_gridblindmetadata_handle_t p) {
  return pnanovdb_read_uint64(buf, pnanovdb_address_offset(p.address, 8));
}
uint pnanovdb_gridblindmetadata_get_flags(pnanovdb_buf_t buf, pnanovdb_gridblindmetadata_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 16));
}
uint pnanovdb_gridblindmetadata_get_semantic(pnanovdb_buf_t buf, pnanovdb_gridblindmetadata_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 20));
}
uint pnanovdb_gridblindmetadata_get_data_class(pnanovdb_buf_t buf, pnanovdb_gridblindmetadata_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 24));
}
uint pnanovdb_gridblindmetadata_get_data_type(pnanovdb_buf_t buf, pnanovdb_gridblindmetadata_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 28));
}
uint pnanovdb_gridblindmetadata_get_name(pnanovdb_buf_t buf, pnanovdb_gridblindmetadata_handle_t p, uint index) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 32 + 4u * index));
}

struct pnanovdb_tree_t {
  uvec2 bytes0;
  uvec2 bytes1;
  uvec2 bytes2;
  uvec2 bytes3;
  uint count0;
  uint count1;
  uint count2;
  uint count3;
  uint pad[4u];
};

struct pnanovdb_tree_handle_t {
  pnanovdb_address_t address;
};
uvec2 pnanovdb_tree_get_bytes0(pnanovdb_buf_t buf, pnanovdb_tree_handle_t p) {
  return pnanovdb_read_uint64(buf, pnanovdb_address_offset(p.address, 0));
}
uvec2 pnanovdb_tree_get_bytes1(pnanovdb_buf_t buf, pnanovdb_tree_handle_t p) {
  return pnanovdb_read_uint64(buf, pnanovdb_address_offset(p.address, 8));
}
uvec2 pnanovdb_tree_get_bytes2(pnanovdb_buf_t buf, pnanovdb_tree_handle_t p) {
  return pnanovdb_read_uint64(buf, pnanovdb_address_offset(p.address, 16));
}
uvec2 pnanovdb_tree_get_bytes3(pnanovdb_buf_t buf, pnanovdb_tree_handle_t p) {
  return pnanovdb_read_uint64(buf, pnanovdb_address_offset(p.address, 24));
}
uint pnanovdb_tree_get_count0(pnanovdb_buf_t buf, pnanovdb_tree_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 32));
}
uint pnanovdb_tree_get_count1(pnanovdb_buf_t buf, pnanovdb_tree_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 36));
}
uint pnanovdb_tree_get_count2(pnanovdb_buf_t buf, pnanovdb_tree_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 40));
}
uint pnanovdb_tree_get_count3(pnanovdb_buf_t buf, pnanovdb_tree_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 44));
}

struct pnanovdb_root_t {
  ivec3 bbox_min;
  ivec3 bbox_max;
  uvec2 active_voxel_count;
  uint tile_count;
  uint pad1;
};

struct pnanovdb_root_handle_t {
  pnanovdb_address_t address;
};
ivec3 pnanovdb_root_get_bbox_min(pnanovdb_buf_t buf, pnanovdb_root_handle_t p) {
  return pnanovdb_read_coord(buf, pnanovdb_address_offset(p.address, 0));
}
ivec3 pnanovdb_root_get_bbox_max(pnanovdb_buf_t buf, pnanovdb_root_handle_t p) {
  return pnanovdb_read_coord(buf, pnanovdb_address_offset(p.address, 12));
}
uvec2 pnanovdb_root_get_active_voxel_count(pnanovdb_buf_t buf, pnanovdb_root_handle_t p) {
  return pnanovdb_read_uint64(buf, pnanovdb_address_offset(p.address, 24));
}
uint pnanovdb_root_get_tile_count(pnanovdb_buf_t buf, pnanovdb_root_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 32));
}

struct pnanovdb_root_tile_t {
  uvec2 key;
  int child_id;
  uint state;
};

struct pnanovdb_root_tile_handle_t {
  pnanovdb_address_t address;
};
uvec2 pnanovdb_root_tile_get_key(pnanovdb_buf_t buf, pnanovdb_root_tile_handle_t p) {
  return pnanovdb_read_uint64(buf, pnanovdb_address_offset(p.address, 0));
}
int pnanovdb_root_tile_get_child_id(pnanovdb_buf_t buf, pnanovdb_root_tile_handle_t p) {
  return pnanovdb_read_int32(buf, pnanovdb_address_offset(p.address, 8));
}
uint pnanovdb_root_tile_get_state(pnanovdb_buf_t buf, pnanovdb_root_tile_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 12));
}

struct pnanovdb_node2_t {
  ivec3 bbox_min;
  ivec3 bbox_max;
  int offset;
  uint flags;
  uint value_mask[1024];
  uint child_mask[1024];
};

struct pnanovdb_node2_handle_t {
  pnanovdb_address_t address;
};
ivec3 pnanovdb_node2_get_bbox_min(pnanovdb_buf_t buf, pnanovdb_node2_handle_t p) {
  return pnanovdb_read_coord(buf, pnanovdb_address_offset(p.address, 0));
}
ivec3 pnanovdb_node2_get_bbox_max(pnanovdb_buf_t buf, pnanovdb_node2_handle_t p) {
  return pnanovdb_read_coord(buf, pnanovdb_address_offset(p.address, 12));
}
int pnanovdb_node2_get_offset(pnanovdb_buf_t buf, pnanovdb_node2_handle_t p) {
  return pnanovdb_read_int32(buf, pnanovdb_address_offset(p.address, 24));
}
uint pnanovdb_node2_get_flags(pnanovdb_buf_t buf, pnanovdb_node2_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 28));
}
bool pnanovdb_node2_get_value_mask(pnanovdb_buf_t buf, pnanovdb_node2_handle_t p, uint bit_index) {
  uint value = pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 32 + 4u * (bit_index >> 5u)));
  return ((value >> (bit_index & 31u)) & 1) != 0u;
}
bool pnanovdb_node2_get_child_mask(pnanovdb_buf_t buf, pnanovdb_node2_handle_t p, uint bit_index) {
  uint value = pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 4128 + 4u * (bit_index >> 5u)));
  return ((value >> (bit_index & 31u)) & 1) != 0u;
}

struct pnanovdb_node1_t {
  ivec3 bbox_min;
  ivec3 bbox_max;
  int offset;
  uint flags;
  uint value_mask[128];
  uint child_mask[128];
};

struct pnanovdb_node1_handle_t {
  pnanovdb_address_t address;
};
ivec3 pnanovdb_node1_get_bbox_min(pnanovdb_buf_t buf, pnanovdb_node1_handle_t p) {
  return pnanovdb_read_coord(buf, pnanovdb_address_offset(p.address, 0));
}
ivec3 pnanovdb_node1_get_bbox_max(pnanovdb_buf_t buf, pnanovdb_node1_handle_t p) {
  return pnanovdb_read_coord(buf, pnanovdb_address_offset(p.address, 12));
}
int pnanovdb_node1_get_offset(pnanovdb_buf_t buf, pnanovdb_node1_handle_t p) {
  return pnanovdb_read_int32(buf, pnanovdb_address_offset(p.address, 24));
}
uint pnanovdb_node1_get_flags(pnanovdb_buf_t buf, pnanovdb_node1_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 28));
}
bool pnanovdb_node1_get_value_mask(pnanovdb_buf_t buf, pnanovdb_node1_handle_t p, uint bit_index) {
  uint value = pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 32 + 4u * (bit_index >> 5u)));
  return ((value >> (bit_index & 31u)) & 1) != 0u;
}
bool pnanovdb_node1_get_child_mask(pnanovdb_buf_t buf, pnanovdb_node1_handle_t p, uint bit_index) {
  uint value = pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 544 + 4u * (bit_index >> 5u)));
  return ((value >> (bit_index & 31u)) & 1) != 0u;
}

struct pnanovdb_node0_t {
  ivec3 bbox_min;
  uint bbox_dif_and_flags;
  uint value_mask[16];
};

struct pnanovdb_node0_handle_t {
  pnanovdb_address_t address;
};
ivec3 pnanovdb_node0_get_bbox_min(pnanovdb_buf_t buf, pnanovdb_node0_handle_t p) {
  return pnanovdb_read_coord(buf, pnanovdb_address_offset(p.address, 0));
}
uint pnanovdb_node0_get_bbox_dif_and_flags(pnanovdb_buf_t buf, pnanovdb_node0_handle_t p) {
  return pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 12));
}
bool pnanovdb_node0_get_value_mask(pnanovdb_buf_t buf, pnanovdb_node0_handle_t p, uint bit_index) {
  uint value = pnanovdb_read_uint32(buf, pnanovdb_address_offset(p.address, 16 + 4u * (bit_index >> 5u)));
  return ((value >> (bit_index & 31u)) & 1) != 0u;
}

struct pnanovdb_grid_type_constants_t {
  uint root_off_background;
  uint root_off_min;
  uint root_off_max;
  uint root_off_ave;
  uint root_off_stddev;
  uint root_size;
  uint value_stride_bits;
  uint table_stride;
  uint root_tile_off_value;
  uint root_tile_size;
  uint node2_off_min;
  uint node2_off_max;
  uint node2_off_ave;
  uint node2_off_stddev;
  uint node2_off_table;
  uint node2_size;
  uint node1_off_min;
  uint node1_off_max;
  uint node1_off_ave;
  uint node1_off_stddev;
  uint node1_off_table;
  uint node1_size;
  uint node0_off_min;
  uint node0_off_max;
  uint node0_off_ave;
  uint node0_off_stddev;
  uint node0_off_table;
  uint node0_size;
};

const pnanovdb_grid_type_constants_t pnanovdb_grid_type_constants[13] = {
    {36,   36,     36,   36,   36,   64,   0,    4,     16, 32, 8224, 8224, 8224, 8224,
     8224, 139296, 1056, 1056, 1056, 1056, 1056, 17440, 80, 80, 80,   80,   96,   96},
    {36,   40,     44,   48,   52,   64,   32,   4,     16, 32, 8224, 8228, 8232, 8236,
     8256, 139328, 1056, 1060, 1064, 1068, 1088, 17472, 80, 84, 88,   92,   96,   2144},
    {40,   48,     56,   64,   72,   96,   64,   8,     16, 32, 8224, 8232, 8240, 8248,
     8256, 270400, 1056, 1064, 1072, 1080, 1088, 33856, 80, 88, 96,   104,  128,  4224},
    {36,   38,     40,   44,   48,   64,   16,   4,     16, 32, 8224, 8226, 8228, 8232,
     8256, 139328, 1056, 1058, 1060, 1064, 1088, 17472, 80, 82, 84,   88,   96,   1120},
    {36,   40,     44,   48,   52,   64,   32,   4,     16, 32, 8224, 8228, 8232, 8236,
     8256, 139328, 1056, 1060, 1064, 1068, 1088, 17472, 80, 84, 88,   92,   96,   2144},
    {40,   48,     56,   64,   72,   96,   64,   8,     16, 32, 8224, 8232, 8240, 8248,
     8256, 270400, 1056, 1064, 1072, 1080, 1088, 33856, 80, 88, 96,   104,  128,  4224},
    {36,   48,     60,   72,   76,   96,   96,   12,    16, 32, 8224, 8236, 8248, 8252,
     8256, 401472, 1056, 1068, 1080, 1084, 1088, 50240, 80, 92, 104,  108,  128,  6272},
    {40,   64,     88,   112,  120,  128,  192,  24,    16, 64,  8224, 8248, 8272, 8280,
     8288, 794720, 1056, 1080, 1104, 1112, 1120, 99424, 80, 104, 128,  136,  160,  12448},
    {36,   37,     38,   39,   40,   64,   0,    4,     16, 32, 8224, 8225, 8226, 8227,
     8256, 139328, 1056, 1057, 1058, 1059, 1088, 17472, 80, 80, 80,   80,   96,   96},
    {36,   38,     40,   44,   48,   64,   16,   4,     16, 32, 8224, 8226, 8228, 8232,
     8256, 139328, 1056, 1058, 1060, 1064, 1088, 17472, 80, 82, 84,   88,   96,   1120},
    {36,   40,     44,   48,   52,   64,   32,   4,     16, 32, 8224, 8228, 8232, 8236,
     8256, 139328, 1056, 1060, 1064, 1068, 1088, 17472, 80, 84, 88,   92,   96,   2144},
    {36,   37,     38,   39,   40,   64,   1,    4,     16, 32, 8224, 8225, 8226, 8227,
     8256, 139328, 1056, 1057, 1058, 1059, 1088, 17472, 80, 80, 80,   80,   96,   160},
    {36,   36,     36,   36,   36,   64,   0,    4,     16, 32, 8224, 8224, 8224, 8224,
     8224, 139296, 1056, 1056, 1056, 1056, 1056, 17440, 80, 80, 80,   80,   96,   96},
};

pnanovdb_gridblindmetadata_handle_t pnanovdb_grid_get_gridblindmetadata(pnanovdb_buf_t buf, pnanovdb_grid_handle_t grid,
                                                                        uint index) {
  pnanovdb_gridblindmetadata_handle_t meta = {grid.address};
  uvec2 byte_offset = pnanovdb_grid_get_blind_metadata_offset(buf, grid);
  meta.address = pnanovdb_address_offset64(meta.address, byte_offset);
  meta.address = pnanovdb_address_offset_product(meta.address, 288, index);
  return meta;
}

pnanovdb_address_t pnanovdb_grid_get_gridblindmetadata_value_address(pnanovdb_buf_t buf, pnanovdb_grid_handle_t grid,
                                                                     uint index) {
  pnanovdb_gridblindmetadata_handle_t meta = pnanovdb_grid_get_gridblindmetadata(buf, grid, index);
  ivec2 byte_offset = pnanovdb_gridblindmetadata_get_byte_offset(buf, meta);
  pnanovdb_address_t address = grid.address;
  address = pnanovdb_address_offset64(address, pnanovdb_int64_as_uint64(byte_offset));
  return address;
}

pnanovdb_tree_handle_t pnanovdb_grid_get_tree(pnanovdb_buf_t buf, pnanovdb_grid_handle_t grid) {
  pnanovdb_tree_handle_t tree = {grid.address};
  tree.address = pnanovdb_address_offset(tree.address, 672);
  return tree;
}

pnanovdb_root_handle_t pnanovdb_tree_get_root(pnanovdb_buf_t buf, pnanovdb_tree_handle_t tree) {
  pnanovdb_root_handle_t root = {tree.address};
  uvec2 byte_offset = pnanovdb_tree_get_bytes3(buf, tree);
  root.address = pnanovdb_address_offset64(root.address, byte_offset);
  return root;
}

pnanovdb_root_tile_handle_t pnanovdb_root_get_tile(pnanovdb_grid_type_t grid_type, pnanovdb_root_handle_t root,
                                                   uint n) {
  pnanovdb_root_tile_handle_t tile = {root.address};
  tile.address = pnanovdb_address_offset(tile.address, PNANOVDB_GRID_TYPE_GET(grid_type, root_size));
  tile.address = pnanovdb_address_offset_product(tile.address, PNANOVDB_GRID_TYPE_GET(grid_type, root_tile_size), n);
  return tile;
}

pnanovdb_root_tile_handle_t pnanovdb_root_get_tile_zero(pnanovdb_grid_type_t grid_type, pnanovdb_root_handle_t root) {
  pnanovdb_root_tile_handle_t tile = {root.address};
  tile.address = pnanovdb_address_offset(tile.address, PNANOVDB_GRID_TYPE_GET(grid_type, root_size));
  return tile;
}

pnanovdb_node2_handle_t pnanovdb_root_get_child(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                pnanovdb_root_handle_t root, pnanovdb_root_tile_handle_t tile) {
  pnanovdb_node2_handle_t node2 = {root.address};
  node2.address = pnanovdb_address_offset(node2.address, PNANOVDB_GRID_TYPE_GET(grid_type, root_size));
  node2.address = pnanovdb_address_offset_product(node2.address, PNANOVDB_GRID_TYPE_GET(grid_type, root_tile_size),
                                                  pnanovdb_root_get_tile_count(buf, root));
  node2.address = pnanovdb_address_offset_product(node2.address, PNANOVDB_GRID_TYPE_GET(grid_type, node2_size),
                                                  pnanovdb_root_tile_get_child_id(buf, tile));
  return node2;
}

uvec2 pnanovdb_coord_to_key(ivec3 ijk) {

  uint iu = pnanovdb_int32_as_uint32(ijk.x) >> 12u;
  uint ju = pnanovdb_int32_as_uint32(ijk.y) >> 12u;
  uint ku = pnanovdb_int32_as_uint32(ijk.z) >> 12u;
  uint key_x = ku | (ju << 21);
  uint key_y = (iu << 10) | (ju >> 11);
  return pnanovdb_uint32_as_uint64(key_x, key_y);
}

pnanovdb_root_tile_handle_t pnanovdb_root_find_tile(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                    pnanovdb_root_handle_t root, ivec3 ijk) {
  uint tile_count = pnanovdb_uint32_as_int32(pnanovdb_root_get_tile_count(buf, root));
  pnanovdb_root_tile_handle_t tile = pnanovdb_root_get_tile_zero(grid_type, root);
  uvec2 key = pnanovdb_coord_to_key(ijk);
  for (uint i = 0u; i < tile_count; i++) {
    if (pnanovdb_uint64_is_equal(key, pnanovdb_root_tile_get_key(buf, tile))) {
      return tile;
    }
    tile.address = pnanovdb_address_offset(tile.address, PNANOVDB_GRID_TYPE_GET(grid_type, root_tile_size));
  }
  pnanovdb_root_tile_handle_t null_handle = {pnanovdb_address_null()};
  return null_handle;
}

uint pnanovdb_node0_coord_to_offset(ivec3 ijk) {
  return (((ijk.x & 7) >> 0) << (2 * 3)) + (((ijk.y & 7) >> 0) << (3)) + ((ijk.z & 7) >> 0);
}

pnanovdb_address_t pnanovdb_node0_get_min_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                  pnanovdb_node0_handle_t node) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, node0_off_min);
  return pnanovdb_address_offset(node.address, byte_offset);
}

pnanovdb_address_t pnanovdb_node0_get_max_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                  pnanovdb_node0_handle_t node) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, node0_off_max);
  return pnanovdb_address_offset(node.address, byte_offset);
}

pnanovdb_address_t pnanovdb_node0_get_ave_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                  pnanovdb_node0_handle_t node) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, node0_off_ave);
  return pnanovdb_address_offset(node.address, byte_offset);
}

pnanovdb_address_t pnanovdb_node0_get_stdddev_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                      pnanovdb_node0_handle_t node) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, node0_off_stddev);
  return pnanovdb_address_offset(node.address, byte_offset);
}

pnanovdb_address_t pnanovdb_node0_get_table_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                    pnanovdb_node0_handle_t node, uint n) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, node0_off_table) +
                     ((PNANOVDB_GRID_TYPE_GET(grid_type, value_stride_bits) * n) >> 3u);
  return pnanovdb_address_offset(node.address, byte_offset);
}

pnanovdb_address_t pnanovdb_node0_get_value_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                    pnanovdb_node0_handle_t node0, ivec3 ijk) {
  uint n = pnanovdb_node0_coord_to_offset(ijk);
  return pnanovdb_node0_get_table_address(grid_type, buf, node0, n);
}

uint pnanovdb_node1_coord_to_offset(ivec3 ijk) {
  return (((ijk.x & 127) >> 3) << (2 * 4)) + (((ijk.y & 127) >> 3) << (4)) + ((ijk.z & 127) >> 3);
}

pnanovdb_address_t pnanovdb_node1_get_min_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                  pnanovdb_node1_handle_t node) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, node1_off_min);
  return pnanovdb_address_offset(node.address, byte_offset);
}

pnanovdb_address_t pnanovdb_node1_get_max_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                  pnanovdb_node1_handle_t node) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, node1_off_max);
  return pnanovdb_address_offset(node.address, byte_offset);
}

pnanovdb_address_t pnanovdb_node1_get_ave_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                  pnanovdb_node1_handle_t node) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, node1_off_ave);
  return pnanovdb_address_offset(node.address, byte_offset);
}

pnanovdb_address_t pnanovdb_node1_get_stddev_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                     pnanovdb_node1_handle_t node) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, node1_off_stddev);
  return pnanovdb_address_offset(node.address, byte_offset);
}

pnanovdb_address_t pnanovdb_node1_get_table_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                    pnanovdb_node1_handle_t node, uint n) {
  uint byte_offset =
      PNANOVDB_GRID_TYPE_GET(grid_type, node1_off_table) + PNANOVDB_GRID_TYPE_GET(grid_type, table_stride) * n;
  return pnanovdb_address_offset(node.address, byte_offset);
}

uint pnanovdb_node1_get_table_child_id(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf, pnanovdb_node1_handle_t node,
                                       uint n) {
  pnanovdb_address_t table_address = pnanovdb_node1_get_table_address(grid_type, buf, node, n);
  return pnanovdb_read_uint32(buf, table_address);
}

pnanovdb_node0_handle_t pnanovdb_node1_get_child(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                 pnanovdb_node1_handle_t node1, uint n) {
  pnanovdb_node0_handle_t node0 = {node1.address};
  node0.address = pnanovdb_address_offset_product(node0.address, PNANOVDB_GRID_TYPE_GET(grid_type, node1_size),
                                                  pnanovdb_node1_get_offset(buf, node1));
  node0.address = pnanovdb_address_offset_product(node0.address, PNANOVDB_GRID_TYPE_GET(grid_type, node0_size),
                                                  pnanovdb_node1_get_table_child_id(grid_type, buf, node1, n));
  return node0;
}

pnanovdb_address_t pnanovdb_node1_get_value_address_and_level(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                              pnanovdb_node1_handle_t node1, ivec3 ijk,
                                                              inout uint level) {
  uint n = pnanovdb_node1_coord_to_offset(ijk);
  pnanovdb_address_t value_address;
  if (pnanovdb_node1_get_child_mask(buf, node1, n)) {
    pnanovdb_node0_handle_t child = pnanovdb_node1_get_child(grid_type, buf, node1, n);
    value_address = pnanovdb_node0_get_value_address(grid_type, buf, child, ijk);
    level = 0u;
  } else {
    value_address = pnanovdb_node1_get_table_address(grid_type, buf, node1, n);
    level = 1u;
  }
  return value_address;
}

pnanovdb_address_t pnanovdb_node1_get_value_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                    pnanovdb_node1_handle_t node1, ivec3 ijk) {
  uint level;
  return pnanovdb_node1_get_value_address_and_level(grid_type, buf, node1, ijk, level);
}

uint pnanovdb_node2_coord_to_offset(ivec3 ijk) {
  return (((ijk.x & 4095) >> 7) << (2 * 5)) + (((ijk.y & 4095) >> 7) << (5)) + ((ijk.z & 4095) >> 7);
}

pnanovdb_address_t pnanovdb_node2_get_min_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                  pnanovdb_node2_handle_t node) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, node2_off_min);
  return pnanovdb_address_offset(node.address, byte_offset);
}

pnanovdb_address_t pnanovdb_node2_get_max_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                  pnanovdb_node2_handle_t node) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, node2_off_max);
  return pnanovdb_address_offset(node.address, byte_offset);
}

pnanovdb_address_t pnanovdb_node2_get_ave_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                  pnanovdb_node2_handle_t node) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, node2_off_ave);
  return pnanovdb_address_offset(node.address, byte_offset);
}

pnanovdb_address_t pnanovdb_node2_get_stddev_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                     pnanovdb_node2_handle_t node) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, node2_off_stddev);
  return pnanovdb_address_offset(node.address, byte_offset);
}

pnanovdb_address_t pnanovdb_node2_get_table_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                    pnanovdb_node2_handle_t node, uint n) {
  uint byte_offset =
      PNANOVDB_GRID_TYPE_GET(grid_type, node2_off_table) + PNANOVDB_GRID_TYPE_GET(grid_type, table_stride) * n;
  return pnanovdb_address_offset(node.address, byte_offset);
}

uint pnanovdb_node2_get_table_child_id(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf, pnanovdb_node2_handle_t node,
                                       uint n) {
  pnanovdb_address_t bufAddress = pnanovdb_node2_get_table_address(grid_type, buf, node, n);
  return pnanovdb_read_uint32(buf, bufAddress);
}

pnanovdb_node1_handle_t pnanovdb_node2_get_child(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                 pnanovdb_node2_handle_t node2, uint n) {
  pnanovdb_node1_handle_t node1 = {node2.address};
  node1.address = pnanovdb_address_offset_product(node1.address, PNANOVDB_GRID_TYPE_GET(grid_type, node2_size),
                                                  pnanovdb_node2_get_offset(buf, node2));
  node1.address = pnanovdb_address_offset_product(node1.address, PNANOVDB_GRID_TYPE_GET(grid_type, node1_size),
                                                  pnanovdb_node2_get_table_child_id(grid_type, buf, node2, n));
  return node1;
}

pnanovdb_address_t pnanovdb_node2_get_value_address_and_level(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                              pnanovdb_node2_handle_t node2, ivec3 ijk,
                                                              inout uint level) {
  uint n = pnanovdb_node2_coord_to_offset(ijk);
  pnanovdb_address_t value_address;
  if (pnanovdb_node2_get_child_mask(buf, node2, n)) {
    pnanovdb_node1_handle_t child = pnanovdb_node2_get_child(grid_type, buf, node2, n);
    value_address = pnanovdb_node1_get_value_address_and_level(grid_type, buf, child, ijk, level);
  } else {
    value_address = pnanovdb_node2_get_table_address(grid_type, buf, node2, n);
    level = 2u;
  }
  return value_address;
}

pnanovdb_address_t pnanovdb_node2_get_value_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                    pnanovdb_node2_handle_t node2, ivec3 ijk) {
  uint level;
  return pnanovdb_node2_get_value_address_and_level(grid_type, buf, node2, ijk, level);
}

pnanovdb_address_t pnanovdb_root_get_min_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                 pnanovdb_root_handle_t root) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, root_off_min);
  return pnanovdb_address_offset(root.address, byte_offset);
}

pnanovdb_address_t pnanovdb_root_get_max_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                 pnanovdb_root_handle_t root) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, root_off_max);
  return pnanovdb_address_offset(root.address, byte_offset);
}

pnanovdb_address_t pnanovdb_root_get_ave_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                 pnanovdb_root_handle_t root) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, root_off_ave);
  return pnanovdb_address_offset(root.address, byte_offset);
}

pnanovdb_address_t pnanovdb_root_get_stddev_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                    pnanovdb_root_handle_t root) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, root_off_stddev);
  return pnanovdb_address_offset(root.address, byte_offset);
}

pnanovdb_address_t pnanovdb_root_tile_get_value_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                        pnanovdb_root_tile_handle_t root_tile) {
  uint byte_offset = PNANOVDB_GRID_TYPE_GET(grid_type, root_tile_off_value);
  return pnanovdb_address_offset(root_tile.address, byte_offset);
}

pnanovdb_address_t pnanovdb_root_get_value_address_and_level(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                             pnanovdb_root_handle_t root, ivec3 ijk, inout uint level) {
  pnanovdb_root_tile_handle_t tile = pnanovdb_root_find_tile(grid_type, buf, root, ijk);
  pnanovdb_address_t ret;
  if (pnanovdb_address_is_null(tile.address)) {
    ret = pnanovdb_address_offset(root.address, PNANOVDB_GRID_TYPE_GET(grid_type, root_off_background));
    level = 4u;
  } else if (pnanovdb_root_tile_get_child_id(buf, tile) < 0) {
    ret = pnanovdb_address_offset(tile.address, PNANOVDB_GRID_TYPE_GET(grid_type, root_tile_off_value));
    level = 3u;
  } else {
    pnanovdb_node2_handle_t child = pnanovdb_root_get_child(grid_type, buf, root, tile);
    ret = pnanovdb_node2_get_value_address_and_level(grid_type, buf, child, ijk, level);
  }
  return ret;
}

pnanovdb_address_t pnanovdb_root_get_value_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                   pnanovdb_root_handle_t root, ivec3 ijk) {
  uint level;
  return pnanovdb_root_get_value_address_and_level(grid_type, buf, root, ijk, level);
}

pnanovdb_address_t pnanovdb_root_get_value_address_bit(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                       pnanovdb_root_handle_t root, ivec3 ijk, inout uint bit_index) {
  uint level;
  pnanovdb_address_t address = pnanovdb_root_get_value_address_and_level(grid_type, buf, root, ijk, level);
  bit_index = level == 0u ? pnanovdb_int32_as_uint32(ijk.x & 7) : 0u;
  return address;
}

struct pnanovdb_readaccessor_t {
  ivec3 key;
  pnanovdb_node0_handle_t node0;
  pnanovdb_node1_handle_t node1;
  pnanovdb_node2_handle_t node2;
  pnanovdb_root_handle_t root;
};

void pnanovdb_readaccessor_init(inout pnanovdb_readaccessor_t acc, pnanovdb_root_handle_t root) {
  acc.key.x = 0x7FFFFFFF;
  acc.key.y = 0x7FFFFFFF;
  acc.key.z = 0x7FFFFFFF;
  acc.node0.address = pnanovdb_address_null();
  acc.node1.address = pnanovdb_address_null();
  acc.node2.address = pnanovdb_address_null();
  acc.root = root;
}

bool pnanovdb_readaccessor_iscached0(inout pnanovdb_readaccessor_t acc, int dirty) {
  if (pnanovdb_address_is_null(acc.node0.address)) {
    return false;
  }
  if ((dirty & ~((1u << 3) - 1u)) != 0) {
    acc.node0.address = pnanovdb_address_null();
    return false;
  }
  return true;
}
bool pnanovdb_readaccessor_iscached1(inout pnanovdb_readaccessor_t acc, int dirty) {
  if (pnanovdb_address_is_null(acc.node1.address)) {
    return false;
  }
  if ((dirty & ~((1u << 7) - 1u)) != 0) {
    acc.node1.address = pnanovdb_address_null();
    return false;
  }
  return true;
}
bool pnanovdb_readaccessor_iscached2(inout pnanovdb_readaccessor_t acc, int dirty) {
  if (pnanovdb_address_is_null(acc.node2.address)) {
    return false;
  }
  if ((dirty & ~((1u << 12) - 1u)) != 0) {
    acc.node2.address = pnanovdb_address_null();
    return false;
  }
  return true;
}
int pnanovdb_readaccessor_computedirty(inout pnanovdb_readaccessor_t acc, ivec3 ijk) {
  return (ijk.x ^ acc.key.x) | (ijk.y ^ acc.key.y) | (ijk.z ^ acc.key.z);
}

pnanovdb_address_t pnanovdb_node0_get_value_address_and_cache(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                              pnanovdb_node0_handle_t node0, ivec3 ijk,
                                                              inout pnanovdb_readaccessor_t acc) {
  uint n = pnanovdb_node0_coord_to_offset(ijk);
  return pnanovdb_node0_get_table_address(grid_type, buf, node0, n);
}

pnanovdb_address_t pnanovdb_node1_get_value_address_and_level_and_cache(pnanovdb_grid_type_t grid_type,
                                                                        pnanovdb_buf_t buf,
                                                                        pnanovdb_node1_handle_t node1, ivec3 ijk,
                                                                        inout pnanovdb_readaccessor_t acc,
                                                                        inout uint level) {
  uint n = pnanovdb_node1_coord_to_offset(ijk);
  pnanovdb_address_t value_address;
  if (pnanovdb_node1_get_child_mask(buf, node1, n)) {
    pnanovdb_node0_handle_t child = pnanovdb_node1_get_child(grid_type, buf, node1, n);
    acc.node0 = child;
    acc.key = ijk;
    value_address = pnanovdb_node0_get_value_address_and_cache(grid_type, buf, child, ijk, acc);
    level = 0u;
  } else {
    value_address = pnanovdb_node1_get_table_address(grid_type, buf, node1, n);
    level = 1u;
  }
  return value_address;
}

pnanovdb_address_t pnanovdb_node1_get_value_address_and_cache(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                              pnanovdb_node1_handle_t node1, ivec3 ijk,
                                                              inout pnanovdb_readaccessor_t acc) {
  uint level;
  return pnanovdb_node1_get_value_address_and_level_and_cache(grid_type, buf, node1, ijk, acc, level);
}

pnanovdb_address_t pnanovdb_node2_get_value_address_and_level_and_cache(pnanovdb_grid_type_t grid_type,
                                                                        pnanovdb_buf_t buf,
                                                                        pnanovdb_node2_handle_t node2, ivec3 ijk,
                                                                        inout pnanovdb_readaccessor_t acc,
                                                                        inout uint level) {
  uint n = pnanovdb_node2_coord_to_offset(ijk);
  pnanovdb_address_t value_address;
  if (pnanovdb_node2_get_child_mask(buf, node2, n)) {
    pnanovdb_node1_handle_t child = pnanovdb_node2_get_child(grid_type, buf, node2, n);
    acc.node1 = child;
    acc.key = ijk;
    value_address = pnanovdb_node1_get_value_address_and_level_and_cache(grid_type, buf, child, ijk, acc, level);
  } else {
    value_address = pnanovdb_node2_get_table_address(grid_type, buf, node2, n);
    level = 2u;
  }
  return value_address;
}

pnanovdb_address_t pnanovdb_node2_get_value_address_and_cache(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                              pnanovdb_node2_handle_t node2, ivec3 ijk,
                                                              inout pnanovdb_readaccessor_t acc) {
  uint level;
  return pnanovdb_node2_get_value_address_and_level_and_cache(grid_type, buf, node2, ijk, acc, level);
}

pnanovdb_address_t pnanovdb_root_get_value_address_and_level_and_cache(pnanovdb_grid_type_t grid_type,
                                                                       pnanovdb_buf_t buf, pnanovdb_root_handle_t root,
                                                                       ivec3 ijk, inout pnanovdb_readaccessor_t acc,
                                                                       inout uint level) {
  pnanovdb_root_tile_handle_t tile = pnanovdb_root_find_tile(grid_type, buf, root, ijk);
  pnanovdb_address_t ret;
  if (pnanovdb_address_is_null(tile.address)) {
    ret = pnanovdb_address_offset(root.address, PNANOVDB_GRID_TYPE_GET(grid_type, root_off_background));
    level = 4u;
  } else if (pnanovdb_root_tile_get_child_id(buf, tile) < 0) {
    ret = pnanovdb_address_offset(tile.address, PNANOVDB_GRID_TYPE_GET(grid_type, root_tile_off_value));
    level = 3u;
  } else {
    pnanovdb_node2_handle_t child = pnanovdb_root_get_child(grid_type, buf, root, tile);
    acc.node2 = child;
    acc.key = ijk;
    ret = pnanovdb_node2_get_value_address_and_level_and_cache(grid_type, buf, child, ijk, acc, level);
  }
  return ret;
}

pnanovdb_address_t pnanovdb_root_get_value_address_and_cache(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                             pnanovdb_root_handle_t root, ivec3 ijk,
                                                             inout pnanovdb_readaccessor_t acc) {
  uint level;
  return pnanovdb_root_get_value_address_and_level_and_cache(grid_type, buf, root, ijk, acc, level);
}

pnanovdb_address_t pnanovdb_readaccessor_get_value_address_and_level(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                                     inout pnanovdb_readaccessor_t acc, ivec3 ijk,
                                                                     inout uint level) {
  int dirty = pnanovdb_readaccessor_computedirty(acc, ijk);

  pnanovdb_address_t value_address;
  if (pnanovdb_readaccessor_iscached0(acc, dirty)) {
    value_address = pnanovdb_node0_get_value_address_and_cache(grid_type, buf, acc.node0, ijk, acc);
    level = 0u;
  } else if (pnanovdb_readaccessor_iscached1(acc, dirty)) {
    value_address = pnanovdb_node1_get_value_address_and_level_and_cache(grid_type, buf, acc.node1, ijk, acc, level);
  } else if (pnanovdb_readaccessor_iscached2(acc, dirty)) {
    value_address = pnanovdb_node2_get_value_address_and_level_and_cache(grid_type, buf, acc.node2, ijk, acc, level);
  } else {
    value_address = pnanovdb_root_get_value_address_and_level_and_cache(grid_type, buf, acc.root, ijk, acc, level);
  }
  return value_address;
}

pnanovdb_address_t pnanovdb_readaccessor_get_value_address(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                           inout pnanovdb_readaccessor_t acc, ivec3 ijk) {
  uint level;
  return pnanovdb_readaccessor_get_value_address_and_level(grid_type, buf, acc, ijk, level);
}

pnanovdb_address_t pnanovdb_readaccessor_get_value_address_bit(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                                               inout pnanovdb_readaccessor_t acc, ivec3 ijk,
                                                               inout uint bit_index) {
  uint level;
  pnanovdb_address_t address = pnanovdb_readaccessor_get_value_address_and_level(grid_type, buf, acc, ijk, level);
  bit_index = level == 0u ? pnanovdb_int32_as_uint32(ijk.x & 7) : 0u;
  return address;
}

uint pnanovdb_node0_get_dim_and_cache(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf, pnanovdb_node0_handle_t node0,
                                      ivec3 ijk, inout pnanovdb_readaccessor_t acc) {
  uint n = pnanovdb_node0_coord_to_offset(ijk);
  return 1u;
}

uint pnanovdb_node1_get_dim_and_cache(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf, pnanovdb_node1_handle_t node1,
                                      ivec3 ijk, inout pnanovdb_readaccessor_t acc) {
  uint n = pnanovdb_node1_coord_to_offset(ijk);
  uint ret;
  if (pnanovdb_node1_get_child_mask(buf, node1, n)) {
    pnanovdb_node0_handle_t child = pnanovdb_node1_get_child(grid_type, buf, node1, n);
    acc.node0 = child;
    acc.key = ijk;
    ret = pnanovdb_node0_get_dim_and_cache(grid_type, buf, child, ijk, acc);
  } else {
    ret = (1u << (3u));
  }
  return ret;
}

uint pnanovdb_node2_get_dim_and_cache(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf, pnanovdb_node2_handle_t node2,
                                      ivec3 ijk, inout pnanovdb_readaccessor_t acc) {
  uint n = pnanovdb_node2_coord_to_offset(ijk);
  uint ret;
  if (pnanovdb_node2_get_child_mask(buf, node2, n)) {
    pnanovdb_node1_handle_t child = pnanovdb_node2_get_child(grid_type, buf, node2, n);
    acc.node1 = child;
    acc.key = ijk;
    ret = pnanovdb_node1_get_dim_and_cache(grid_type, buf, child, ijk, acc);
  } else {
    ret = (1u << (4u + 3u));
  }
  return ret;
}

uint pnanovdb_root_get_dim_and_cache(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf, pnanovdb_root_handle_t root,
                                     ivec3 ijk, inout pnanovdb_readaccessor_t acc) {
  pnanovdb_root_tile_handle_t tile = pnanovdb_root_find_tile(grid_type, buf, root, ijk);
  uint ret;
  if (pnanovdb_address_is_null(tile.address)) {
    ret = 1u << (5u + 4u + 3u);
  } else if (pnanovdb_root_tile_get_child_id(buf, tile) < 0) {
    ret = 1u << (5u + 4u + 3u);
  } else {
    pnanovdb_node2_handle_t child = pnanovdb_root_get_child(grid_type, buf, root, tile);
    acc.node2 = child;
    acc.key = ijk;
    ret = pnanovdb_node2_get_dim_and_cache(grid_type, buf, child, ijk, acc);
  }
  return ret;
}

uint pnanovdb_readaccessor_get_dim(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                   inout pnanovdb_readaccessor_t acc, ivec3 ijk) {
  int dirty = pnanovdb_readaccessor_computedirty(acc, ijk);

  uint dim;
  if (pnanovdb_readaccessor_iscached0(acc, dirty)) {
    dim = pnanovdb_node0_get_dim_and_cache(grid_type, buf, acc.node0, ijk, acc);
  } else if (pnanovdb_readaccessor_iscached1(acc, dirty)) {
    dim = pnanovdb_node1_get_dim_and_cache(grid_type, buf, acc.node1, ijk, acc);
  } else if (pnanovdb_readaccessor_iscached2(acc, dirty)) {
    dim = pnanovdb_node2_get_dim_and_cache(grid_type, buf, acc.node2, ijk, acc);
  } else {
    dim = pnanovdb_root_get_dim_and_cache(grid_type, buf, acc.root, ijk, acc);
  }
  return dim;
}

bool pnanovdb_node0_is_active_and_cache(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                        pnanovdb_node0_handle_t node0, ivec3 ijk, inout pnanovdb_readaccessor_t acc) {
  uint n = pnanovdb_node0_coord_to_offset(ijk);
  return pnanovdb_node0_get_value_mask(buf, node0, n);
}

bool pnanovdb_node1_is_active_and_cache(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                        pnanovdb_node1_handle_t node1, ivec3 ijk, inout pnanovdb_readaccessor_t acc) {
  uint n = pnanovdb_node1_coord_to_offset(ijk);
  bool is_active;
  if (pnanovdb_node1_get_child_mask(buf, node1, n)) {
    pnanovdb_node0_handle_t child = pnanovdb_node1_get_child(grid_type, buf, node1, n);
    acc.node0 = child;
    acc.key = ijk;
    is_active = pnanovdb_node0_is_active_and_cache(grid_type, buf, child, ijk, acc);
  } else {
    is_active = pnanovdb_node1_get_value_mask(buf, node1, n);
  }
  return is_active;
}

bool pnanovdb_node2_is_active_and_cache(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                        pnanovdb_node2_handle_t node2, ivec3 ijk, inout pnanovdb_readaccessor_t acc) {
  uint n = pnanovdb_node2_coord_to_offset(ijk);
  bool is_active;
  if (pnanovdb_node2_get_child_mask(buf, node2, n)) {
    pnanovdb_node1_handle_t child = pnanovdb_node2_get_child(grid_type, buf, node2, n);
    acc.node1 = child;
    acc.key = ijk;
    is_active = pnanovdb_node1_is_active_and_cache(grid_type, buf, child, ijk, acc);
  } else {
    is_active = pnanovdb_node2_get_value_mask(buf, node2, n);
  }
  return is_active;
}

bool pnanovdb_root_is_active_and_cache(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf, pnanovdb_root_handle_t root,
                                       ivec3 ijk, inout pnanovdb_readaccessor_t acc) {
  pnanovdb_root_tile_handle_t tile = pnanovdb_root_find_tile(grid_type, buf, root, ijk);
  bool is_active;
  if (pnanovdb_address_is_null(tile.address)) {
    is_active = false;
  } else if (pnanovdb_root_tile_get_child_id(buf, tile) < 0) {
    uint state = pnanovdb_root_tile_get_state(buf, tile);
    is_active = state != 0u;
  } else {
    pnanovdb_node2_handle_t child = pnanovdb_root_get_child(grid_type, buf, root, tile);
    acc.node2 = child;
    acc.key = ijk;
    is_active = pnanovdb_node2_is_active_and_cache(grid_type, buf, child, ijk, acc);
  }
  return is_active;
}

bool pnanovdb_readaccessor_is_active(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf,
                                     inout pnanovdb_readaccessor_t acc, ivec3 ijk) {
  int dirty = pnanovdb_readaccessor_computedirty(acc, ijk);

  bool is_active;
  if (pnanovdb_readaccessor_iscached0(acc, dirty)) {
    is_active = pnanovdb_node0_is_active_and_cache(grid_type, buf, acc.node0, ijk, acc);
  } else if (pnanovdb_readaccessor_iscached1(acc, dirty)) {
    is_active = pnanovdb_node1_is_active_and_cache(grid_type, buf, acc.node1, ijk, acc);
  } else if (pnanovdb_readaccessor_iscached2(acc, dirty)) {
    is_active = pnanovdb_node2_is_active_and_cache(grid_type, buf, acc.node2, ijk, acc);
  } else {
    is_active = pnanovdb_root_is_active_and_cache(grid_type, buf, acc.root, ijk, acc);
  }
  return is_active;
}

vec3 pnanovdb_map_apply(pnanovdb_buf_t buf, pnanovdb_map_handle_t map, vec3 src) {
  vec3 dst;
  float sx = src.x;
  float sy = src.y;
  float sz = src.z;
  dst.x = sx * pnanovdb_map_get_matf(buf, map, 0) + sy * pnanovdb_map_get_matf(buf, map, 1) +
          sz * pnanovdb_map_get_matf(buf, map, 2) + pnanovdb_map_get_vecf(buf, map, 0);
  dst.y = sx * pnanovdb_map_get_matf(buf, map, 3) + sy * pnanovdb_map_get_matf(buf, map, 4) +
          sz * pnanovdb_map_get_matf(buf, map, 5) + pnanovdb_map_get_vecf(buf, map, 1);
  dst.z = sx * pnanovdb_map_get_matf(buf, map, 6) + sy * pnanovdb_map_get_matf(buf, map, 7) +
          sz * pnanovdb_map_get_matf(buf, map, 8) + pnanovdb_map_get_vecf(buf, map, 2);
  return dst;
}

vec3 pnanovdb_map_apply_inverse(pnanovdb_buf_t buf, pnanovdb_map_handle_t map, vec3 src) {
  vec3 dst;
  float sx = src.x - pnanovdb_map_get_vecf(buf, map, 0);
  float sy = src.y - pnanovdb_map_get_vecf(buf, map, 1);
  float sz = src.z - pnanovdb_map_get_vecf(buf, map, 2);
  dst.x = sx * pnanovdb_map_get_invmatf(buf, map, 0) + sy * pnanovdb_map_get_invmatf(buf, map, 1) +
          sz * pnanovdb_map_get_invmatf(buf, map, 2);
  dst.y = sx * pnanovdb_map_get_invmatf(buf, map, 3) + sy * pnanovdb_map_get_invmatf(buf, map, 4) +
          sz * pnanovdb_map_get_invmatf(buf, map, 5);
  dst.z = sx * pnanovdb_map_get_invmatf(buf, map, 6) + sy * pnanovdb_map_get_invmatf(buf, map, 7) +
          sz * pnanovdb_map_get_invmatf(buf, map, 8);
  return dst;
}

vec3 pnanovdb_map_apply_jacobi(pnanovdb_buf_t buf, pnanovdb_map_handle_t map, vec3 src) {
  vec3 dst;
  float sx = src.x;
  float sy = src.y;
  float sz = src.z;
  dst.x = sx * pnanovdb_map_get_matf(buf, map, 0) + sy * pnanovdb_map_get_matf(buf, map, 1) +
          sz * pnanovdb_map_get_matf(buf, map, 2);
  dst.y = sx * pnanovdb_map_get_matf(buf, map, 3) + sy * pnanovdb_map_get_matf(buf, map, 4) +
          sz * pnanovdb_map_get_matf(buf, map, 5);
  dst.z = sx * pnanovdb_map_get_matf(buf, map, 6) + sy * pnanovdb_map_get_matf(buf, map, 7) +
          sz * pnanovdb_map_get_matf(buf, map, 8);
  return dst;
}

vec3 pnanovdb_map_apply_inverse_jacobi(pnanovdb_buf_t buf, pnanovdb_map_handle_t map, vec3 src) {
  vec3 dst;
  float sx = src.x;
  float sy = src.y;
  float sz = src.z;
  dst.x = sx * pnanovdb_map_get_invmatf(buf, map, 0) + sy * pnanovdb_map_get_invmatf(buf, map, 1) +
          sz * pnanovdb_map_get_invmatf(buf, map, 2);
  dst.y = sx * pnanovdb_map_get_invmatf(buf, map, 3) + sy * pnanovdb_map_get_invmatf(buf, map, 4) +
          sz * pnanovdb_map_get_invmatf(buf, map, 5);
  dst.z = sx * pnanovdb_map_get_invmatf(buf, map, 6) + sy * pnanovdb_map_get_invmatf(buf, map, 7) +
          sz * pnanovdb_map_get_invmatf(buf, map, 8);
  return dst;
}

vec3 pnanovdb_grid_world_to_indexf(pnanovdb_buf_t buf, pnanovdb_grid_handle_t grid, vec3 src) {
  pnanovdb_map_handle_t map = pnanovdb_grid_get_map(buf, grid);
  return pnanovdb_map_apply_inverse(buf, map, src);
}

vec3 pnanovdb_grid_index_to_worldf(pnanovdb_buf_t buf, pnanovdb_grid_handle_t grid, vec3 src) {
  pnanovdb_map_handle_t map = pnanovdb_grid_get_map(buf, grid);
  return pnanovdb_map_apply(buf, map, src);
}

vec3 pnanovdb_grid_world_to_index_dirf(pnanovdb_buf_t buf, pnanovdb_grid_handle_t grid, vec3 src) {
  pnanovdb_map_handle_t map = pnanovdb_grid_get_map(buf, grid);
  return pnanovdb_map_apply_inverse_jacobi(buf, map, src);
}

vec3 pnanovdb_grid_index_to_world_dirf(pnanovdb_buf_t buf, pnanovdb_grid_handle_t grid, vec3 src) {
  pnanovdb_map_handle_t map = pnanovdb_grid_get_map(buf, grid);
  return pnanovdb_map_apply_jacobi(buf, map, src);
}
struct pnanovdb_hdda_t {
  int dim;
  float tmin;
  float tmax;
  ivec3 voxel;
  ivec3 step;
  vec3 delta;
  vec3 next;
};

ivec3 pnanovdb_hdda_pos_to_ijk(vec3 pos) {
  ivec3 voxel;
  voxel.x = pnanovdb_float_to_int32(pnanovdb_floor(pos.x));
  voxel.y = pnanovdb_float_to_int32(pnanovdb_floor(pos.y));
  voxel.z = pnanovdb_float_to_int32(pnanovdb_floor(pos.z));
  return voxel;
}

ivec3 pnanovdb_hdda_pos_to_voxel(vec3 pos, int dim) {
  ivec3 voxel;
  voxel.x = pnanovdb_float_to_int32(pnanovdb_floor(pos.x)) & (~(dim - 1));
  voxel.y = pnanovdb_float_to_int32(pnanovdb_floor(pos.y)) & (~(dim - 1));
  voxel.z = pnanovdb_float_to_int32(pnanovdb_floor(pos.z)) & (~(dim - 1));
  return voxel;
}

vec3 pnanovdb_hdda_ray_start(vec3 origin, float tmin, vec3 direction) {
  vec3 pos = pnanovdb_vec3_add(pnanovdb_vec3_mul(direction, pnanovdb_vec3_uniform(tmin)), origin);
  return pos;
}

void pnanovdb_hdda_init(inout pnanovdb_hdda_t hdda, vec3 origin, float tmin, vec3 direction, float tmax, int dim) {
  hdda.dim = dim;
  hdda.tmin = tmin;
  hdda.tmax = tmax;

  vec3 pos = pnanovdb_hdda_ray_start(origin, tmin, direction);
  vec3 dir_inv = pnanovdb_vec3_div(pnanovdb_vec3_uniform(1.f), direction);

  hdda.voxel = pnanovdb_hdda_pos_to_voxel(pos, dim);

  if (direction.x == 0.f) {
    hdda.next.x = 1e38f;
    hdda.step.x = 0;
    hdda.delta.x = 0.f;
  } else if (dir_inv.x > 0.f) {
    hdda.step.x = 1;
    hdda.next.x = hdda.tmin + (hdda.voxel.x + dim - pos.x) * dir_inv.x;
    hdda.delta.x = dir_inv.x;
  } else {
    hdda.step.x = -1;
    hdda.next.x = hdda.tmin + (hdda.voxel.x - pos.x) * dir_inv.x;
    hdda.delta.x = -dir_inv.x;
  }

  if (direction.y == 0.f) {
    hdda.next.y = 1e38f;
    hdda.step.y = 0;
    hdda.delta.y = 0.f;
  } else if (dir_inv.y > 0.f) {
    hdda.step.y = 1;
    hdda.next.y = hdda.tmin + (hdda.voxel.y + dim - pos.y) * dir_inv.y;
    hdda.delta.y = dir_inv.y;
  } else {
    hdda.step.y = -1;
    hdda.next.y = hdda.tmin + (hdda.voxel.y - pos.y) * dir_inv.y;
    hdda.delta.y = -dir_inv.y;
  }

  if (direction.z == 0.f) {
    hdda.next.z = 1e38f;
    hdda.step.z = 0;
    hdda.delta.z = 0.f;
  } else if (dir_inv.z > 0.f) {
    hdda.step.z = 1;
    hdda.next.z = hdda.tmin + (hdda.voxel.z + dim - pos.z) * dir_inv.z;
    hdda.delta.z = dir_inv.z;
  } else {
    hdda.step.z = -1;
    hdda.next.z = hdda.tmin + (hdda.voxel.z - pos.z) * dir_inv.z;
    hdda.delta.z = -dir_inv.z;
  }
}

bool pnanovdb_hdda_update(inout pnanovdb_hdda_t hdda, vec3 origin, vec3 direction, int dim) {
  if (hdda.dim == dim) {
    return false;
  }
  hdda.dim = dim;

  vec3 pos = pnanovdb_vec3_add(pnanovdb_vec3_mul(direction, pnanovdb_vec3_uniform(hdda.tmin)), origin);
  vec3 dir_inv = pnanovdb_vec3_div(pnanovdb_vec3_uniform(1.f), direction);

  hdda.voxel = pnanovdb_hdda_pos_to_voxel(pos, dim);

  if (hdda.step.x != 0) {
    hdda.next.x = hdda.tmin + (hdda.voxel.x - pos.x) * dir_inv.x;
    if (hdda.step.x > 0) {
      hdda.next.x += dim * dir_inv.x;
    }
  }
  if (hdda.step.y != 0) {
    hdda.next.y = hdda.tmin + (hdda.voxel.y - pos.y) * dir_inv.y;
    if (hdda.step.y > 0) {
      hdda.next.y += dim * dir_inv.y;
    }
  }
  if (hdda.step.z != 0) {
    hdda.next.z = hdda.tmin + (hdda.voxel.z - pos.z) * dir_inv.z;
    if (hdda.step.z > 0) {
      hdda.next.z += dim * dir_inv.z;
    }
  }

  return true;
}

bool pnanovdb_hdda_step(inout pnanovdb_hdda_t hdda) {
  bool ret;
  if (hdda.next.x < hdda.next.y && hdda.next.x < hdda.next.z) {

    if (hdda.next.x <= hdda.tmin) {
      hdda.next.x += hdda.tmin - 0.999999f * hdda.next.x + 1.0e-6f;
    }

    hdda.tmin = hdda.next.x;
    hdda.next.x += hdda.dim * hdda.delta.x;
    hdda.voxel.x += hdda.dim * hdda.step.x;
    ret = hdda.tmin <= hdda.tmax;
  } else if (hdda.next.y < hdda.next.z) {

    if (hdda.next.y <= hdda.tmin) {
      hdda.next.y += hdda.tmin - 0.999999f * hdda.next.y + 1.0e-6f;
    }

    hdda.tmin = hdda.next.y;
    hdda.next.y += hdda.dim * hdda.delta.y;
    hdda.voxel.y += hdda.dim * hdda.step.y;
    ret = hdda.tmin <= hdda.tmax;
  } else {

    if (hdda.next.z <= hdda.tmin) {
      hdda.next.z += hdda.tmin - 0.999999f * hdda.next.z + 1.0e-6f;
    }

    hdda.tmin = hdda.next.z;
    hdda.next.z += hdda.dim * hdda.delta.z;
    hdda.voxel.z += hdda.dim * hdda.step.z;
    ret = hdda.tmin <= hdda.tmax;
  }
  return ret;
}

bool pnanovdb_hdda_ray_clip(vec3 bbox_min, vec3 bbox_max, vec3 origin, inout float tmin, vec3 direction,
                            inout float tmax) {
  vec3 dir_inv = pnanovdb_vec3_div(pnanovdb_vec3_uniform(1.f), direction);
  vec3 t0 = pnanovdb_vec3_mul(pnanovdb_vec3_sub(bbox_min, origin), dir_inv);
  vec3 t1 = pnanovdb_vec3_mul(pnanovdb_vec3_sub(bbox_max, origin), dir_inv);
  vec3 tmin3 = pnanovdb_vec3_min(t0, t1);
  vec3 tmax3 = pnanovdb_vec3_max(t0, t1);
  float tnear = pnanovdb_max(tmin3.x, pnanovdb_max(tmin3.y, tmin3.z));
  float tfar = pnanovdb_min(tmax3.x, pnanovdb_min(tmax3.y, tmax3.z));
  bool hit = tnear <= tfar;
  tmin = pnanovdb_max(tmin, tnear);
  tmax = pnanovdb_min(tmax, tfar);
  return hit;
}

bool pnanovdb_hdda_zero_crossing(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf, inout pnanovdb_readaccessor_t acc,
                                 vec3 origin, float tmin, vec3 direction, float tmax, inout float thit, inout float v) {
  ivec3 bbox_min = pnanovdb_root_get_bbox_min(buf, acc.root);
  ivec3 bbox_max = pnanovdb_root_get_bbox_max(buf, acc.root);
  vec3 bbox_minf = pnanovdb_coord_to_vec3(bbox_min);
  vec3 bbox_maxf = pnanovdb_coord_to_vec3(pnanovdb_coord_add(bbox_max, pnanovdb_coord_uniform(1)));

  bool hit = pnanovdb_hdda_ray_clip(bbox_minf, bbox_maxf, origin, tmin, direction, tmax);
  if (!hit || tmax > 1.0e20f) {
    return false;
  }

  vec3 pos = pnanovdb_hdda_ray_start(origin, tmin, direction);
  ivec3 ijk = pnanovdb_hdda_pos_to_ijk(pos);

  pnanovdb_address_t address = pnanovdb_readaccessor_get_value_address(1, buf, acc, ijk);
  float v0 = pnanovdb_read_float(buf, address);

  int dim = pnanovdb_uint32_as_int32(pnanovdb_readaccessor_get_dim(1, buf, acc, ijk));
  pnanovdb_hdda_t hdda;
  pnanovdb_hdda_init(hdda, origin, tmin, direction, tmax, dim);
  while (pnanovdb_hdda_step(hdda)) {
    vec3 pos_start = pnanovdb_hdda_ray_start(origin, hdda.tmin + 1.0001f, direction);
    ijk = pnanovdb_hdda_pos_to_ijk(pos_start);
    dim = pnanovdb_uint32_as_int32(pnanovdb_readaccessor_get_dim(1, buf, acc, ijk));
    pnanovdb_hdda_update(hdda, origin, direction, dim);
    if (hdda.dim > 1 || !pnanovdb_readaccessor_is_active(grid_type, buf, acc, ijk)) {
      continue;
    }
    while (pnanovdb_hdda_step(hdda) && pnanovdb_readaccessor_is_active(grid_type, buf, acc, hdda.voxel)) {
      ijk = hdda.voxel;
      pnanovdb_address_t address = pnanovdb_readaccessor_get_value_address(1, buf, acc, ijk);
      v = pnanovdb_read_float(buf, address);
      if (v * v0 < 0.f) {
        thit = hdda.tmin;
        return true;
      }
    }
  }
  return false;
}
