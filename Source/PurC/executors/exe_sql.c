/*
 * @file exe_sql.c
 * @author Xu Xiaohong
 * @date 2021/10/10
 * @brief The implementation of public part for SQL executor.
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

#include "exe_sql.h"

#include "private/executor.h"

#include "private/debug.h"
#include "private/errors.h"

struct pcexec_exe_sql_inst {
    struct purc_exec_inst       super;
};

// 创建一个执行器实例
static purc_exec_inst_t
exe_sql_create(enum purc_exec_type type, purc_variant_t input, bool asc_desc)
{
    if (!purc_variant_is_object(input))
        return NULL;

    struct pcexec_exe_sql_inst *inst;
    inst = calloc(1, sizeof(*inst));
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_OOM);
        return NULL;
    }

    inst->super.type        = type;
    inst->super.input       = input;
    inst->super.asc_desc    = asc_desc;

    purc_variant_ref(input);

    return &inst->super;
}

static inline bool
exe_sql_parse_rule(purc_exec_inst_t inst, const char* rule)
{
    // parse and fill the internal fields from rule
    // for example, generating the `selected_keys` which contains all
    // selected keys.

    // clear previously-selected-keys
    if (inst->selected_keys) {
        purc_variant_unref(inst->selected_keys);
        inst->selected_keys = PURC_VARIANT_INVALID;
    }

    // TODO: parse rule and eval to selected_keys

    UNUSED_PARAM(inst);
    UNUSED_PARAM(rule);

    pcinst_set_error(PCEXECUTOR_ERROR_NOT_IMPLEMENTED);
    return false;
}

// 用于执行选择
static purc_variant_t
exe_sql_choose(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    if (!exe_sql_parse_rule(inst, rule))
        return PURC_VARIANT_INVALID;

    size_t sz = purc_variant_array_get_size(inst->selected_keys);

    purc_variant_t vals = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (vals == PURC_VARIANT_INVALID) {
        return vals;
    }

    bool ok = true;

    for (size_t i=0; i<sz; ++i) {
        purc_variant_t k;
        k = purc_variant_array_get(inst->selected_keys, i);
        purc_variant_t v;
        v = purc_variant_object_get(inst->input, k, false);
        if (v==PURC_VARIANT_INVALID)
            continue;
        ok = purc_variant_array_append(vals, v);
        if (!ok)
            break;
    }

    if (ok)
        return vals;

    purc_variant_unref(vals);
    return PURC_VARIANT_INVALID;
}

// 获得用于迭代的初始迭代子
static purc_exec_iter_t
exe_sql_it_begin(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    inst->it.curr = 0;
    if (!exe_sql_parse_rule(inst, rule))
        return NULL;

    size_t sz = purc_variant_array_get_size(inst->selected_keys);
    if (sz<=0) {
        pcinst_set_error(PCEXECUTOR_ERROR_NO_KEYS_SELECTED);
        return NULL;
    }

    return &inst->it;
}

// 根据迭代子获得对应的变体值
static purc_variant_t
exe_sql_it_value(purc_exec_inst_t inst, purc_exec_iter_t it)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);
    PC_ASSERT(inst->selected_keys != PURC_VARIANT_INVALID);
    PC_ASSERT(inst->input != PURC_VARIANT_INVALID);

    purc_variant_t k;
    k = purc_variant_array_get(inst->selected_keys, it->curr);
    purc_variant_t v;
    v = purc_variant_object_get(inst->input, k, false);

    return v;
}

// 获得下一个迭代子
// 注意: 规则字符串可能在前后两次迭代中发生变化，比如在规则中引用了变量的情形下。
// 如果规则并没有发生变化，则对 `rule` 参数传递 NULL。
static purc_exec_iter_t
exe_sql_it_next(purc_exec_inst_t inst, purc_exec_iter_t it, const char* rule)
{
    if (!inst || !it) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return NULL;
    }

    PC_ASSERT(&inst->it == it);

    if (rule) {
        // clear previously-selected-keys
        if (inst->selected_keys) {
            purc_variant_unref(inst->selected_keys);
            inst->selected_keys = PURC_VARIANT_INVALID;
        }

        if (!exe_sql_parse_rule(inst, rule))
            return NULL;
    }

    ++it->curr;

    size_t sz = purc_variant_array_get_size(inst->selected_keys);
    if (it->curr >= sz) {
        it->curr = sz;
        return NULL;
    }

    return it;
}

// 用于执行规约
static purc_variant_t
exe_sql_reduce(purc_exec_inst_t inst, const char* rule)
{
    if (!inst || !rule) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return PURC_VARIANT_INVALID;
    }

    if (!exe_sql_parse_rule(inst, rule))
        return PURC_VARIANT_INVALID;

    size_t sz = purc_variant_array_get_size(inst->selected_keys);

    purc_variant_t objs;
    objs = purc_variant_make_object(0,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (objs == PURC_VARIANT_INVALID) {
        return objs;
    }

    bool ok = false;

    for (size_t i=0; i<sz; ++i) {
        purc_variant_t k;
        k = purc_variant_array_get(inst->selected_keys, i);
        purc_variant_t v;
        v = purc_variant_object_get(inst->input, k, false);
        if (v==PURC_VARIANT_INVALID)
            continue;
        ok = purc_variant_object_set(objs, k, v);
        if (!ok)
            break;
    }

    if (ok)
        return objs;

    purc_variant_unref(objs);
    return PURC_VARIANT_INVALID;
}

// 销毁一个执行器实例
static bool
exe_sql_destroy(purc_exec_inst_t inst)
{
    if (!inst) {
        pcinst_set_error(PCEXECUTOR_ERROR_BAD_ARG);
        return false;
    }

    struct pcexec_exe_sql_inst *exe_sql_inst = (struct pcexec_exe_sql_inst*)inst;

    if (exe_sql_inst->super.input) {
        purc_variant_unref(exe_sql_inst->super.input);
        exe_sql_inst->super.input = PURC_VARIANT_INVALID;
    }
    if (exe_sql_inst->super.selected_keys) {
        purc_variant_unref(exe_sql_inst->super.selected_keys);
        exe_sql_inst->super.selected_keys = PURC_VARIANT_INVALID;
    }

    free(exe_sql_inst);
    return true;
}

static struct purc_exec_ops exe_sql_ops = {
    exe_sql_create,
    exe_sql_choose,
    exe_sql_it_begin,
    exe_sql_it_value,
    exe_sql_it_next,
    exe_sql_reduce,
    exe_sql_destroy,
};

int pcexec_exe_sql_register(void)
{
    bool ok = purc_register_executor("SQL", &exe_sql_ops);
    return ok ? 0 : -1;
}

