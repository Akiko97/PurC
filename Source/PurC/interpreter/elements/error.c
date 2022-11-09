/**
 * @file error.c
 * @author Xue Shuming
 * @date 2021/05/27
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

#include "../internal.h"

#include "private/debug.h"
#include "purc-runloop.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>

struct ctxt_for_error {
    struct pcvdom_node           *curr;

    purc_variant_t                type;
    purc_variant_t                contents;
};

static void
ctxt_for_error_destroy(struct ctxt_for_error *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->type);
        PURC_VARIANT_SAFE_CLEAR(ctxt->contents);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_error_destroy((struct ctxt_for_error*)ctxt);
}

static int
process_attr_type(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_error *ctxt;
    ctxt = (struct ctxt_for_error*)frame->ctxt;
    if (ctxt->type != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (!purc_variant_is_string(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not string",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    ctxt->type = purc_variant_ref(val);

    return 0;
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
            return process_attr_type(frame, element, name, val);
        }

        if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
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

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);

    if (stack->except)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_error *ctxt;
    ctxt = (struct ctxt_for_error*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    frame->pos = pos; // ATTENTION!!

    if (0 != pcintr_stack_frame_eval_attr_and_content(stack, frame, false)) {
        return NULL;
    }

    ctxt->contents = pcintr_template_make();
    if (!ctxt->contents)
        return ctxt;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    int r;
    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

    purc_clr_error();

    if (ctxt->type == PURC_VARIANT_INVALID) {
        ctxt->type = purc_variant_make_string("*", false);
    }

    if (ctxt->type == PURC_VARIANT_INVALID)
        return ctxt;

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    if (frame->ctxt == NULL)
        return true;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_error *ctxt;
    ctxt = (struct ctxt_for_error*)frame->ctxt;
    if (ctxt) {
        ctxt_for_error_destroy(ctxt);
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

    struct ctxt_for_error *ctxt;
    ctxt = (struct ctxt_for_error*)frame->ctxt;
    PC_ASSERT(ctxt);

    struct pcvcm_node *vcm = content->vcm;
    if (!vcm)
        return 0;

    // NOTE: element is still the owner of vcm_content
    PC_ASSERT(ctxt->contents);
    bool to_free = false;
    return pcintr_template_set(ctxt->contents, vcm, PURC_VARIANT_INVALID, to_free);
}

static int
on_child_finished(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);

    struct ctxt_for_error *ctxt;
    ctxt = (struct ctxt_for_error*)frame->ctxt;

    purc_variant_t contents = ctxt->contents;
    if (!contents)
        return -1;

    PC_ASSERT(ctxt->type != PURC_VARIANT_INVALID);
    int r;
    r = pcintr_bind_template(frame->error_templates,
            ctxt->type, ctxt->contents);

    return r ? -1 : 0;

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

    struct ctxt_for_error *ctxt;
    ctxt = (struct ctxt_for_error*)frame->ctxt;

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
        int r = on_child_finished(co, frame);
        PC_ASSERT(0 == r);
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

struct pcintr_element_ops* pcintr_get_error_ops(void)
{
    return &ops;
}

