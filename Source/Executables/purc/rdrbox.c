/*
** @file rdrbox.c
** @author Vincent Wei
** @date 2022/10/10
** @brief The implementation of renderring box.
**
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "rdrbox.h"

#include <assert.h>

struct _text_segment {
    struct list_head ln;

    unsigned i; // the index of first character
    unsigned n; // number of characters in this segment

    /* position of this segment in the containing block box */
    int x, y;

    /* rows taken by this segment (always be 1). */
    unsigned height;
    /* columns taken by this segment. */
    unsigned width;
};

struct _inline_box_data {
    /* the code points of text in Unicode (should be in visual order) */
    uint32_t *ucs;

    int letter_spacing;
    int word_spacing;

    /* text color */
    int color;

    /* the text segments */
    struct list_head segs;
};

struct _block_box_data {
    // margins
    int ml, mt, mr, mb;
    // paddings
    int pl, pt, pr, pb;
};

int foil_rdrbox_module_init(void)
{
    return 0;
}

void foil_rdrbox_module_cleanup(void)
{
}

purcth_rdrbox *foil_rdrbox_new_block(void)
{
    purcth_rdrbox *box = calloc(1, sizeof(*box));

    if (box) {
        box->type = PCTH_RDR_BOX_TYPE_BLOCK;
        box->block_data = calloc(1, sizeof(*box->block_data));
        if (box->block_data == NULL) {
            free(box);
            box = NULL;
        }
    }

    return box;
}

void foil_rdrbox_append_child(purcth_rdrbox *to, purcth_rdrbox *node)
{
    if (to->last != NULL) {
        to->last->next = node;
    }
    else {
        to->first = node;
    }

    node->parent = to;
    node->next = NULL;
    node->prev = to->last;

    to->last = node;
}

void foil_rdrbox_prepend_child(purcth_rdrbox *to, purcth_rdrbox *node)
{
    if (to->first != NULL) {
        to->first->prev = node;
    }
    else {
        to->last = node;
    }

    node->parent = to;
    node->next = to->first;
    node->prev = NULL;

    to->first = node;
}

void foil_rdrbox_insert_before(purcth_rdrbox *to, purcth_rdrbox *node)
{
    if (to->prev != NULL) {
        to->prev->next = node;
    }
    else {
        if (to->parent != NULL) {
            to->parent->first = node;
        }
    }

    node->parent = to->parent;
    node->next = to;
    node->prev = to->prev;

    to->prev = node;
}

void foil_rdrbox_insert_after(purcth_rdrbox *to, purcth_rdrbox *node)
{
    if (to->next != NULL) {
        to->next->prev = node;
    }
    else {
        if (to->parent != NULL) {
            to->parent->last = node;
        }
    }

    node->parent = to->parent;
    node->next = to->next;
    node->prev = to;
    to->next = node;
}

void foil_rdrbox_remove_from_tree(purcth_rdrbox *node)
{
    if (node->parent != NULL) {
        if (node->parent->first == node) {
            node->parent->first = node->next;
        }

        if (node->parent->last == node) {
            node->parent->last = node->prev;
        }
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }

    if (node->prev != NULL) {
        node->prev->next = node->next;
    }

    node->parent = NULL;
    node->next = NULL;
    node->prev = NULL;
}

void foil_rdrbox_delete(purcth_rdrbox *box)
{
    free(box->data);
    free(box);
}

void foil_rdrbox_delete_recursively(purcth_rdrbox *box)
{
    purcth_rdrbox *child = box->first;

    while (child) {
        purcth_rdrbox *next = child->next;
        if (child->first)
            foil_rdrbox_delete_recursively(child);
        else
            foil_rdrbox_delete(child);

        child = next;
    }
}

