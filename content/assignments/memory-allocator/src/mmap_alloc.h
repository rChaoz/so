// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "helpers.h"

void *mmap_alloc(size_t size);

void *mmap_realloc(void *ptr, size_t size);

int mmap_free(void *ptr);
