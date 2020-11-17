#include <rawkit/core.h>

#include <string>
using namespace std;

#include "stb_sb.h"
#include <stdarg.h>

bool _rawkit_resource_sources(rawkit_resource_t *res, uint32_t source_count, ...) {
  if (!res) {
    return false;
  }

  rawkit_resource_t *sources[256] = {};

  va_list vl;
  va_start(vl, source_count);
  for (uint32_t i=0; i<source_count; i++) {
    sources[i] = va_arg(vl, rawkit_resource_t *);
  }

  va_end(vl);

  return rawkit_resource_sources_array(res, source_count, sources);
}

bool rawkit_resource_sources_array(rawkit_resource_t *res, uint32_t source_count, rawkit_resource_t **sources) {
  bool dirty = false;

  uint32_t l = sb_count(res->resource_source_refs);
  // Dirty when the source counts do not match
  if (source_count != l) {
    if (l) {
      stb__sbraw(res->resource_source_refs)[1] = 0;
    }

    for (uint32_t i=0; i<source_count; i++) {
      rawkit_resource_t *source = sources[i];

      if (!source || source == res) {
        return false;
      }

      if (source->resource_version != 0) {
        dirty = true;
      }

      rawkit_resource_ref_t ref = {};
      ref.id = source->resource_id;
      ref.version = source->resource_version;
      sb_push(res->resource_source_refs, ref);
    }

    return dirty;
  }


  for (uint32_t i=0; i<source_count; i++) {
    const rawkit_resource_t *source = sources[i];
    rawkit_resource_ref_t *ref = &res->resource_source_refs[i];

    if (source->resource_id != ref->id) {
      dirty = true;
    }

    if (source->resource_version != 0 && source->resource_version != ref->version) {
      dirty = true;
    }

    // immediately clean the resource so we can avoid making the caller hoopjump
    ref->version = source->resource_version;
    ref->id  = source->resource_id;
  }

  return dirty;
}
