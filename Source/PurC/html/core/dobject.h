/**
 * @file dobject.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for dobject.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PCHTML_DOBJECT_H
#define PCHTML_DOBJECT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/base.h"
#include "html/core/mem.h"
#include "private/array.h"


typedef struct {
    pchtml_mem_t   *mem;
    pcutils_array_t *cache;

    size_t         allocated;
    size_t         struct_size;
}
pchtml_dobject_t;


pchtml_dobject_t *
pchtml_dobject_create(void) WTF_INTERNAL;

unsigned int
pchtml_dobject_init(pchtml_dobject_t *dobject,
                    size_t chunk_size, size_t struct_size) WTF_INTERNAL;

void
pchtml_dobject_clean(pchtml_dobject_t *dobject) WTF_INTERNAL;

pchtml_dobject_t *
pchtml_dobject_destroy(pchtml_dobject_t *dobject, bool destroy_self) WTF_INTERNAL;

uint8_t *
pchtml_dobject_init_list_entries(pchtml_dobject_t *dobject, size_t pos) WTF_INTERNAL;

void *
pchtml_dobject_alloc(pchtml_dobject_t *dobject) WTF_INTERNAL;

void *
pchtml_dobject_calloc(pchtml_dobject_t *dobject) WTF_INTERNAL;

void *
pchtml_dobject_free(pchtml_dobject_t *dobject, void *data) WTF_INTERNAL;

void *
pchtml_dobject_by_absolute_position(pchtml_dobject_t *dobject, size_t pos) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline size_t
pchtml_dobject_allocated(pchtml_dobject_t *dobject)
{
    return dobject->allocated;
}

static inline size_t
pchtml_dobject_cache_length(pchtml_dobject_t *dobject)
{
    return pcutils_array_length(dobject->cache);
}

/*
 * No inline functions for ABI.
 */
size_t
pchtml_dobject_allocated_noi(pchtml_dobject_t *dobject);

size_t
pchtml_dobject_cache_length_noi(pchtml_dobject_t *dobject);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_DOBJECT_H */


