/**
 * @file except.c
 * @author Xu Xiaohong
 * @date 2021/12/06
 * @brief
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
 *
 */

#include "purc.h"

#include "internal.h"

#include "private/debug.h"
#include "purc-runloop.h"

#include "ops.h"

#include <pthread.h>
#include <unistd.h>

struct ctxt_for_except {
    struct pcvdom_node           *curr;

    purc_variant_t                contents;
};

static void
ctxt_for_except_destroy(struct ctxt_for_except *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->contents);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_except_destroy((struct ctxt_for_except*)ctxt);
}

static int
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);
    UNUSED_PARAM(val);
    UNUSED_PARAM(ud);

    PC_ASSERT(attr);

    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    if (name) {
        if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, RAW)) == name) {
            return 0;
        }
        if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TYPE)) == name) {
            return 0;
        }
        PC_DEBUGX("name: %s", purc_atom_to_string(name));
        PC_ASSERT(0);
        return -1;
    }

    PC_DEBUGX("name: %s", attr->key);
    PC_ASSERT(0);
    return -1;
}

static int
attr_found(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name,
        struct pcvdom_attr *attr,
        void *ud)
{
    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    purc_variant_t val = pcintr_eval_vdom_attr(pcintr_get_stack(), attr);
    if (val == PURC_VARIANT_INVALID)
        return -1;

    int r = attr_found_val(frame, element, name, val, attr, ud);
    purc_variant_unref(val);

    return r ? -1 : 0;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    PC_ASSERT(stack == pcintr_get_stack());

    if (stack->except)
        return NULL;

    if (pcintr_check_insertion_mode_for_normal_element(stack))
        return NULL;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_except *ctxt;
    ctxt = (struct ctxt_for_except*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    frame->pos = pos; // ATTENTION!!

    ctxt->contents = pcintr_template_make();
    if (!ctxt->contents)
        return NULL;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    int r;
    r = pcintr_vdom_walk_attrs(frame, element, NULL, attr_found);
    if (r)
        return NULL;

    purc_clr_error();

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == pcintr_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    if (frame->ctxt == NULL)
        return true;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_except *ctxt;
    ctxt = (struct ctxt_for_except*)frame->ctxt;
    if (ctxt) {
        ctxt_for_except_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    PC_ASSERT(content);

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_except *ctxt;
    ctxt = (struct ctxt_for_except*)frame->ctxt;
    PC_ASSERT(ctxt);

    struct pcvcm_node *vcm = content->vcm;
    if (!vcm)
        return 0;

    // NOTE: element is still the owner of vcm_content
    PC_ASSERT(ctxt->contents);
    return pcintr_template_append(ctxt->contents, vcm);
}

static int
on_child_finished(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);

    struct ctxt_for_except *ctxt;
    ctxt = (struct ctxt_for_except*)frame->ctxt;

    purc_variant_t contents = ctxt->contents;
    if (!contents)
        return -1;

    return 0;

#if 0
    // TODO:
    PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
    frame->ctnt_var = contents;
    purc_variant_ref(contents);

    purc_variant_t name;
    name = ctxt->name;
    if (name == PURC_VARIANT_INVALID)
        return -1;

    const char *s_name = purc_variant_get_string_const(name);
    if (s_name == NULL)
        return -1;

    struct pcvdom_element *scope = frame->scope;
    PC_ASSERT(scope);

    bool ok;
    ok = pcintr_bind_scope_variable(scope, s_name, frame->ctnt_var);
    if (!ok)
        return -1;

    D("[%s] bounded", s_name);
    return 0;
#endif
}

static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == pcintr_get_stack());

    pcintr_coroutine_t co = stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(ud == frame->ctxt);

    if (stack->back_anchor == frame)
        stack->back_anchor = NULL;

    if (frame->ctxt == NULL)
        return NULL;

    if (stack->back_anchor)
        return NULL;

    struct ctxt_for_except *ctxt;
    ctxt = (struct ctxt_for_except*)frame->ctxt;

    struct pcvdom_node *curr;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_element *element = frame->pos;
        struct pcvdom_node *node = &element->node;
        node = pcvdom_node_first_child(node);
        curr = node;
    }
    else {
        curr = pcvdom_node_next_sibling(curr);
    }

    ctxt->curr = curr;

    if (curr == NULL) {
        purc_clr_error();
        PC_ASSERT(0 == on_child_finished(co, frame));
        return NULL;
    }

    switch (curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            PC_ASSERT(0); // Not implemented yet
            break;
        case PCVDOM_NODE_ELEMENT:
            PC_ASSERT(0); // Not implemented yet
            break;
        case PCVDOM_NODE_CONTENT:
            if (on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr)))
                return NULL;
            goto again;
        case PCVDOM_NODE_COMMENT:
            PC_ASSERT(0); // Not implemented yet
            goto again;
        default:
            PC_ASSERT(0); // Not implemented yet
    }

    PC_ASSERT(0);
    return NULL; // NOTE: never reached here!!!
}
static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = NULL,
    .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_get_except_ops(void)
{
    return &ops;
}

