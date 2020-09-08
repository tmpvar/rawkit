
#pragma once
#include <hot/jitjob.h>
#include <roaring/roaring.h>

void host_croaring_init(JitJob *job) {
  job->addExport("roaring_bitmap_create", (void *)&roaring_bitmap_create);
  job->addExport("roaring_bitmap_from_range", (void *)&roaring_bitmap_from_range);
  job->addExport("roaring_bitmap_create_with_capacity", (void *)&roaring_bitmap_create_with_capacity);
  job->addExport("roaring_bitmap_of_ptr", (void *)&roaring_bitmap_of_ptr);
  job->addExport("roaring_bitmap_get_copy_on_write", (void *)&roaring_bitmap_get_copy_on_write);
  job->addExport("roaring_bitmap_set_copy_on_write", (void *)&roaring_bitmap_set_copy_on_write);
  job->addExport("roaring_bitmap_printf_describe", (void *)&roaring_bitmap_printf_describe);
  job->addExport("roaring_bitmap_of", (void *)&roaring_bitmap_of);
  job->addExport("roaring_bitmap_copy", (void *)&roaring_bitmap_copy);
  job->addExport("roaring_bitmap_overwrite", (void *)&roaring_bitmap_overwrite);
  job->addExport("roaring_bitmap_printf", (void *)&roaring_bitmap_printf);
  job->addExport("roaring_bitmap_and", (void *)&roaring_bitmap_and);
  job->addExport("roaring_bitmap_and_cardinality", (void *)&roaring_bitmap_and_cardinality);
  job->addExport("roaring_bitmap_intersect", (void *)&roaring_bitmap_intersect);
  job->addExport("roaring_bitmap_jaccard_index", (void *)&roaring_bitmap_jaccard_index);
  job->addExport("roaring_bitmap_or_cardinality", (void *)&roaring_bitmap_or_cardinality);
  job->addExport("roaring_bitmap_andnot_cardinality", (void *)&roaring_bitmap_andnot_cardinality);
  job->addExport("roaring_bitmap_xor_cardinality", (void *)&roaring_bitmap_xor_cardinality);
  job->addExport("roaring_bitmap_and_inplace", (void *)&roaring_bitmap_and_inplace);
  job->addExport("roaring_bitmap_or", (void *)&roaring_bitmap_or);
  job->addExport("roaring_bitmap_or_inplace", (void *)&roaring_bitmap_or_inplace);
  job->addExport("roaring_bitmap_or_many", (void *)&roaring_bitmap_or_many);
  job->addExport("roaring_bitmap_or_many_heap", (void *)&roaring_bitmap_or_many_heap);
  job->addExport("roaring_bitmap_xor", (void *)&roaring_bitmap_xor);
  job->addExport("roaring_bitmap_xor_inplace", (void *)&roaring_bitmap_xor_inplace);
  job->addExport("roaring_bitmap_xor_many", (void *)&roaring_bitmap_xor_many);
  job->addExport("roaring_bitmap_andnot", (void *)&roaring_bitmap_andnot);
  job->addExport("roaring_bitmap_andnot_inplace", (void *)&roaring_bitmap_andnot_inplace);
  job->addExport("roaring_bitmap_free", (void *)&roaring_bitmap_free);
  job->addExport("roaring_bitmap_add_many", (void *)&roaring_bitmap_add_many);
  job->addExport("roaring_bitmap_add", (void *)&roaring_bitmap_add);
  job->addExport("roaring_bitmap_add_checked", (void *)&roaring_bitmap_add_checked);
  job->addExport("roaring_bitmap_add_range_closed", (void *)&roaring_bitmap_add_range_closed);
  job->addExport("roaring_bitmap_add_range", (void *)&roaring_bitmap_add_range);
  job->addExport("roaring_bitmap_add_range_closed", (void *)&roaring_bitmap_add_range_closed);
  job->addExport("roaring_bitmap_remove", (void *)&roaring_bitmap_remove);
  job->addExport("roaring_bitmap_remove_range_closed", (void *)&roaring_bitmap_remove_range_closed);
  job->addExport("roaring_bitmap_remove_range", (void *)&roaring_bitmap_remove_range);
  job->addExport("roaring_bitmap_remove_range_closed", (void *)&roaring_bitmap_remove_range_closed);
  job->addExport("roaring_bitmap_remove_many", (void *)&roaring_bitmap_remove_many);
  job->addExport("roaring_bitmap_remove_checked", (void *)&roaring_bitmap_remove_checked);
  job->addExport("roaring_bitmap_contains", (void *)&roaring_bitmap_contains);
  job->addExport("roaring_bitmap_contains_range", (void *)&roaring_bitmap_contains_range);
  job->addExport("roaring_bitmap_get_cardinality", (void *)&roaring_bitmap_get_cardinality);
  job->addExport("roaring_bitmap_range_cardinality", (void *)&roaring_bitmap_range_cardinality);
  job->addExport("roaring_bitmap_is_empty", (void *)&roaring_bitmap_is_empty);
  job->addExport("roaring_bitmap_clear", (void *)&roaring_bitmap_clear);
  job->addExport("roaring_bitmap_to_uint32_array", (void *)&roaring_bitmap_to_uint32_array);
  job->addExport("roaring_bitmap_range_uint32_array", (void *)&roaring_bitmap_range_uint32_array);
  job->addExport("roaring_bitmap_remove_run_compression", (void *)&roaring_bitmap_remove_run_compression);
  job->addExport("roaring_bitmap_run_optimize", (void *)&roaring_bitmap_run_optimize);
  job->addExport("roaring_bitmap_shrink_to_fit", (void *)&roaring_bitmap_shrink_to_fit);
  job->addExport("roaring_bitmap_size_in_bytes", (void *)&roaring_bitmap_size_in_bytes);
  job->addExport("roaring_bitmap_size_in_bytes", (void *)&roaring_bitmap_size_in_bytes);
  job->addExport("roaring_bitmap_serialize", (void *)&roaring_bitmap_serialize);
  job->addExport("roaring_bitmap_deserialize", (void *)&roaring_bitmap_deserialize);
  job->addExport("roaring_bitmap_size_in_bytes", (void *)&roaring_bitmap_size_in_bytes);
  job->addExport("roaring_bitmap_portable_deserialize", (void *)&roaring_bitmap_portable_deserialize);
  job->addExport("roaring_bitmap_portable_deserialize_safe", (void *)&roaring_bitmap_portable_deserialize_safe);
  job->addExport("roaring_bitmap_portable_deserialize_size", (void *)&roaring_bitmap_portable_deserialize_size);
  job->addExport("roaring_bitmap_portable_size_in_bytes", (void *)&roaring_bitmap_portable_size_in_bytes);
  job->addExport("roaring_bitmap_portable_serialize", (void *)&roaring_bitmap_portable_serialize);
  job->addExport("roaring_bitmap_frozen_size_in_bytes", (void *)&roaring_bitmap_frozen_size_in_bytes);
  job->addExport("roaring_bitmap_frozen_serialize", (void *)&roaring_bitmap_frozen_serialize);
  job->addExport("roaring_bitmap_frozen_view", (void *)&roaring_bitmap_frozen_view);
  job->addExport("roaring_iterate", (void *)&roaring_iterate);
  job->addExport("roaring_iterate64", (void *)&roaring_iterate64);
  job->addExport("roaring_bitmap_equals", (void *)&roaring_bitmap_equals);
  job->addExport("roaring_bitmap_is_subset", (void *)&roaring_bitmap_is_subset);
  job->addExport("roaring_bitmap_is_strict_subset", (void *)&roaring_bitmap_is_strict_subset);
  job->addExport("roaring_bitmap_lazy_or", (void *)&roaring_bitmap_lazy_or);
  job->addExport("roaring_bitmap_lazy_or_inplace", (void *)&roaring_bitmap_lazy_or_inplace);
  job->addExport("roaring_bitmap_repair_after_lazy", (void *)&roaring_bitmap_repair_after_lazy);
  job->addExport("roaring_bitmap_lazy_xor", (void *)&roaring_bitmap_lazy_xor);
  job->addExport("roaring_bitmap_lazy_xor_inplace", (void *)&roaring_bitmap_lazy_xor_inplace);
  job->addExport("roaring_bitmap_flip", (void *)&roaring_bitmap_flip);
  job->addExport("roaring_bitmap_flip_inplace", (void *)&roaring_bitmap_flip_inplace);
  job->addExport("roaring_bitmap_select", (void *)&roaring_bitmap_select);
  job->addExport("roaring_bitmap_rank", (void *)&roaring_bitmap_rank);
  job->addExport("roaring_bitmap_minimum", (void *)&roaring_bitmap_minimum);
  job->addExport("roaring_bitmap_maximum", (void *)&roaring_bitmap_maximum);
  job->addExport("roaring_bitmap_statistics", (void *)&roaring_bitmap_statistics);
  job->addExport("roaring_advance_uint32_iterator", (void *)&roaring_advance_uint32_iterator);
  job->addExport("roaring_init_iterator", (void *)&roaring_init_iterator);
  job->addExport("roaring_init_iterator_last", (void *)&roaring_init_iterator_last);
  job->addExport("roaring_create_iterator", (void *)&roaring_create_iterator);
  job->addExport("roaring_advance_uint32_iterator", (void *)&roaring_advance_uint32_iterator);
  job->addExport("roaring_previous_uint32_iterator", (void *)&roaring_previous_uint32_iterator);
  job->addExport("roaring_move_uint32_iterator_equalorlarger", (void *)&roaring_move_uint32_iterator_equalorlarger);
  job->addExport("roaring_copy_uint32_iterator", (void *)&roaring_copy_uint32_iterator);
  job->addExport("roaring_free_uint32_iterator", (void *)&roaring_free_uint32_iterator);
  job->addExport("roaring_read_uint32_iterator", (void *)&roaring_read_uint32_iterator);
}
