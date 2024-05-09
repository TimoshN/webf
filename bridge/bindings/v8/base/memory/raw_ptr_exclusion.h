/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#ifndef BASE_MEMORY_RAW_PTR_EXCLUSION_H_
#define BASE_MEMORY_RAW_PTR_EXCLUSION_H_

// Although `raw_ptr` is part of the standalone PA distribution, it is
// easier to use the shorter path in `//base/memory`. We retain this
// facade header for ease of typing.
#include "bindings/v8/base/allocator/partition_allocator/src/partition_alloc/pointers/raw_ptr_exclusion.h"  // IWYU pragma: export

#endif  // BASE_MEMORY_RAW_PTR_EXCLUSION_H_