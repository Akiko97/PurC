/**
 * @file archetype.c
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

#include "../internal.h"

#include "private/debug.h"
#include "purc-runloop.h"
#include "private/stringbuilder.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>

struct ctxt_for_archetype {
    struct pcvdom_node           *curr;
    purc_variant_t                name;

    purc_variant_t                src;
    purc_variant_t                param;
    purc_variant_t                method;

    purc_variant_t                sync_id;
    pcintr_coroutine_t            co;

    int                           ret_code;
    int                           err;
    purc_rwstream_t               resp;

    purc_variant_t                contents;

    unsigned int                  under_head:1;
};

static void
ctxt_for_archetype_destroy(struct ctxt_for_archetype *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->name);
        PURC_VARIANT_SAFE_CLEAR(ctxt->src);
        PURC_VARIANT_SAFE_CLEAR(ctxt->param);
        PURC_VARIANT_SAFE_CLEAR(ctxt->method);
        PURC_VARIANT_SAFE_CLEAR(ctxt->sync_id);
        PURC_VARIANT_SAFE_CLEAR(ctxt->contents);
        if (ctxt->resp) {
            purc_rwstream_destroy(ctxt->resp);
            ctxt->resp = NULL;
        }
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_archetype_destroy((struct ctxt_for_archetype*)ctxt);
}

static int
process_attr_name(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    if (ctxt->name != PURC_VARIANT_INVALID) {
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
    ctxt->name = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_src(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    if (ctxt->src != PURC_VARIANT_INVALID) {
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
    ctxt->src = purc_variant_ref(val);

    return 0;
}

static int
process_attr_param(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    if (ctxt->param != PURC_VARIANT_INVALID) {
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
    if (!purc_variant_is_object(val)) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> is not object",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->param = purc_variant_ref(val);

    return 0;
}

static int
process_attr_method(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    if (ctxt->method != PURC_VARIANT_INVALID) {
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
    ctxt->method = purc_variant_ref(val);

    return 0;
}

static int
process_attr_raw(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);
    UNUSED_PARAM(name);
    UNUSED_PARAM(val);
    return 0;
}

static int
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(attr);
    UNUSED_PARAM(ud);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, NAME)) == name) {
        return process_attr_name(frame, element, name, val);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SRC)) == name) {
        return process_attr_src(frame, element, name, val);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, PARAM)) == name) {
        return process_attr_param(frame, element, name, val);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, METHOD)) == name) {
        return process_attr_method(frame, element, name, val);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, RAW)) == name) {
        return process_attr_raw(frame, element, name, val);
    }

    purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
            "vdom attribute '%s' for element <%s>",
            purc_atom_to_string(name), element->tag_name);

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

static int
method_by_method(const char *s_method, enum pcfetcher_request_method *method)
{
    if (strcmp(s_method, "GET") == 0) {
        *method = PCFETCHER_REQUEST_METHOD_GET;
    }
    else if (strcmp(s_method, "POST") == 0) {
        *method = PCFETCHER_REQUEST_METHOD_POST;
    }
    else if (strcmp(s_method, "DELETE") == 0) {
        *method = PCFETCHER_REQUEST_METHOD_DELETE;
    }
    else {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "unknown method `%s`", s_method);
        return -1;
    }

    return 0;
}

static void on_sync_complete_on_frame(struct ctxt_for_archetype *ctxt,
        const struct pcfetcher_resp_header *resp_header,
        purc_rwstream_t resp)
{
    UNUSED_PARAM(resp_header);
    UNUSED_PARAM(resp);

    PC_DEBUG("load_async|callback|ret_code=%d\n", resp_header->ret_code);
    PC_DEBUG("load_async|callback|mime_type=%s\n", resp_header->mime_type);
    PC_DEBUG("load_async|callback|sz_resp=%ld\n", resp_header->sz_resp);

    ctxt->ret_code = resp_header->ret_code;
    ctxt->resp = resp;
    PC_ASSERT(purc_get_last_error() == PURC_ERROR_OK);

    pcintr_resume(NULL);
}

static void on_sync_complete(purc_variant_t request_id, void *ud,
        const struct pcfetcher_resp_header *resp_header,
        purc_rwstream_t resp)
{
    UNUSED_PARAM(ud);
    UNUSED_PARAM(resp_header);
    UNUSED_PARAM(resp);

    pcintr_heap_t heap = pcintr_get_heap();
    PC_ASSERT(heap);
    PC_ASSERT(pcintr_get_coroutine() == NULL);

    pcintr_stack_frame_t frame;
    frame = (pcintr_stack_frame_t)ud;
    PC_ASSERT(frame);
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    PC_ASSERT(ctxt);

    pcintr_coroutine_t co = ctxt->co;
    PC_ASSERT(co);
    PC_ASSERT(co->owner == heap);
    PC_ASSERT(ctxt->sync_id == request_id);

    pcintr_set_current_co(co);
    on_sync_complete_on_frame(ctxt, resp_header, resp);
    pcintr_set_current_co(NULL);
}

static void on_sync_continuation(void *ud, void *extra)
{
    UNUSED_PARAM(extra);

    struct pcintr_stack_frame *frame;
    frame = (struct pcintr_stack_frame*)ud;
    PC_ASSERT(frame);

    pcintr_coroutine_t co = pcintr_get_coroutine();
    PC_ASSERT(co);
    PC_ASSERT(co->state == CO_STATE_RUN);
    pcintr_stack_t stack = &co->stack;
    PC_ASSERT(frame == pcintr_stack_get_bottom_frame(stack));

    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    PC_ASSERT(ctxt);
    PC_ASSERT(ctxt->co == co);

    struct pcvdom_element *element = frame->pos;

    if (ctxt->ret_code == RESP_CODE_USER_STOP) {
        frame->next_step = NEXT_STEP_ON_POPPING;
        goto clean_rws;
    }

    bool has_except = false;
    if (!ctxt->resp || ctxt->ret_code != 200) {
        has_except = true;
        purc_set_error(PURC_ERROR_NO_DATA);
        goto dispatch_except;
    }

    bool ok;
    purc_variant_t ret = purc_variant_load_from_json_stream(ctxt->resp);
    PRINT_VARIANT(ret);
    PC_ASSERT(0); // Not implemented yet
    const char *s_name = purc_variant_get_string_const(ctxt->src);
    if (ret != PURC_VARIANT_INVALID) {
        if (ctxt->under_head) {
            ok = purc_bind_document_variable(stack->vdom, s_name, ret);
        } else {
            element = pcvdom_element_parent(element);
            ok = pcintr_bind_scope_variable(element, s_name, ret);
            PC_ASSERT(ok);
            PC_ASSERT(0);
        }
        purc_variant_unref(ret);
        if (ok) {
            PC_ASSERT(purc_get_last_error()==0);
            goto clean_rws;
        }
        has_except = true;
        goto dispatch_except;
    }
    else {
        has_except = true;
        goto dispatch_except;
    }

dispatch_except:
    if (has_except) {
        PC_ASSERT(purc_get_last_error());
    }

clean_rws:
    if (ctxt->resp) {
        purc_rwstream_destroy(ctxt->resp);
        ctxt->resp = NULL;
    }
    // TODO: NEXT_STEP_SELECT_CHILD in some failure cases
    frame->next_step = NEXT_STEP_ON_POPPING;
}

static void
process_by_src(pcintr_stack_t stack, struct pcintr_stack_frame *frame)
{
    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;

    const char *s_src = purc_variant_get_string_const(ctxt->src);
    PC_ASSERT(s_src);

    const char *s_method = "GET";
    if (ctxt->method != PURC_VARIANT_INVALID) {
        s_method = purc_variant_get_string_const(ctxt->method);
    }

    int r;

    enum pcfetcher_request_method method;
    r = method_by_method(s_method, &method);
    if (r)
        return;

    purc_variant_t param;
    if (ctxt->param == PURC_VARIANT_INVALID) {
        param = purc_variant_make_object_0();
        if (param == PURC_VARIANT_INVALID)
            return;
    }
    else {
        param = purc_variant_ref(ctxt->param);
    }

    ctxt->co = stack->co;
    purc_variant_t v;
    v = pcintr_load_from_uri_async(stack, s_src, method, param,
            on_sync_complete, frame);
    purc_variant_unref(param);

    if (v == PURC_VARIANT_INVALID)
        return;

    ctxt->sync_id = purc_variant_ref(v);

    pcintr_yield(frame, on_sync_continuation);
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    PC_ASSERT(stack == pcintr_get_stack());

    if (stack->except)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    PC_ASSERT(frame->ctnt_var == PURC_VARIANT_INVALID);

    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    frame->pos = pos; // ATTENTION!!

    frame->attr_vars = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (frame->attr_vars == PURC_VARIANT_INVALID)
        return ctxt;

    ctxt->contents = pcintr_template_make();
    if (!ctxt->contents)
        return ctxt;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    int r;
    r = pcintr_vdom_walk_attrs(frame, element, NULL, attr_found);
    if (r)
        return ctxt;

    while ((element=pcvdom_element_parent(element))) {
        if (element->tag_id == PCHVML_TAG_HEAD) {
            ctxt->under_head = 1;
        }
    }

    purc_clr_error();

    if (ctxt->name == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                    "lack of vdom attribute 'name' for element <%s>",
                    frame->pos->tag_name);

        return ctxt;
    }

    if (ctxt->src != PURC_VARIANT_INVALID) {
        process_by_src(stack, frame);
        PC_ASSERT(purc_get_last_error() == 0);
        return ctxt;
    }

    PC_ASSERT(frame->ctnt_var == PURC_VARIANT_INVALID);

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

    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
    if (ctxt) {
        ctxt_for_archetype_destroy(ctxt);
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

    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;
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

    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;

    purc_variant_t contents = ctxt->contents;
    if (!contents)
        return -1;

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

    struct pcvdom_element *parent = pcvdom_element_parent(frame->pos);

    bool ok;
    ok = pcintr_bind_scope_variable(parent, s_name, frame->ctnt_var);
    if (!ok)
        return -1;

    return 0;
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

    struct ctxt_for_archetype *ctxt;
    ctxt = (struct ctxt_for_archetype*)frame->ctxt;

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

struct pcintr_element_ops* pcintr_get_archetype_ops(void)
{
    return &ops;
}

