// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "helpers.h"

void *brk_alloc(size_t size);

void *brk_realloc(void *ptr, size_t size);

int brk_free(void *ptr);
