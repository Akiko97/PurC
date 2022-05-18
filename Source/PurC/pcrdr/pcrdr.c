/*
 * @file pcrdr.c
 * @date 2022/02/21
 * @brief The initializer of the PCRDR module.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2021, 2022
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

#include "config.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/pcrdr.h"

#include "connect.h"

#include "pcrdr_err_msgs.inc"

/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(msgs,
        PCA_TABLESIZE(pcrdr_err_msgs) == PCRDR_ERROR_NR);

#undef _COMPILE_TIME_ASSERT

static struct err_msg_seg _pcrdr_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_PCRDR,
    PURC_ERROR_FIRST_PCRDR + PCA_TABLESIZE(pcrdr_err_msgs) - 1,
    pcrdr_err_msgs
};

static struct pcrdr_opatom {
    const char *op;
    purc_atom_t atom;
} pcrdr_opatoms[] = {
    { PCRDR_OPERATION_STARTSESSION,         0 }, // "startSession"
    { PCRDR_OPERATION_ENDSESSION,           0 }, // "endSession"
    { PCRDR_OPERATION_CREATEWORKSPACE,      0 }, // "createWorkspace"
    { PCRDR_OPERATION_UPDATEWORKSPACE,      0 }, // "updateWorkspace"
    { PCRDR_OPERATION_DESTROYWORKSPACE,     0 }, // "destroyWorkspace"
    { PCRDR_OPERATION_CREATEPLAINWINDOW,    0 }, // "createPlainWindow"
    { PCRDR_OPERATION_UPDATEPLAINWINDOW,    0 }, // "updatePlainWindow"
    { PCRDR_OPERATION_DESTROYPLAINWINDOW,   0 }, // "destroyPlainWindow"
    { PCRDR_OPERATION_CREATETABBEDWINDOW,   0 }, // "createTabbedWindow"
    { PCRDR_OPERATION_UPDATETABBEDWINDOW,   0 }, // "updateTabbedWindow"
    { PCRDR_OPERATION_DESTROYTABBEDWINDOW,  0 }, // "destroyTabbedWindow"
    { PCRDR_OPERATION_CREATETABPAGE,        0 }, // "createTabpage"
    { PCRDR_OPERATION_UPDATETABPAGE,        0 }, // "updateTabpage"
    { PCRDR_OPERATION_DESTROYTABPAGE,       0 }, // "destroyTabpage"
    { PCRDR_OPERATION_LOAD,                 0 }, // "load"
    { PCRDR_OPERATION_WRITEBEGIN,           0 }, // "writeBegin"
    { PCRDR_OPERATION_WRITEMORE,            0 }, // "writeMore"
    { PCRDR_OPERATION_WRITEEND,             0 }, // "writeEnd"
    { PCRDR_OPERATION_APPEND,               0 }, // "append"
    { PCRDR_OPERATION_PREPEND,              0 }, // "prepend"
    { PCRDR_OPERATION_INSERTBEFORE,         0 }, // "insertBefore"
    { PCRDR_OPERATION_INSERTAFTER,          0 }, // "insertAfter"
    { PCRDR_OPERATION_DISPLACE,             0 }, // "displace"
    { PCRDR_OPERATION_UPDATE,               0 }, // "update"
    { PCRDR_OPERATION_ERASE,                0 }, // "erase"
    { PCRDR_OPERATION_CLEAR,                0 }, // "clear"
};

/* make sure the number of operations matches the enumulators */
#define _COMPILE_TIME_ASSERT(name, x)           \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(ops,
        PCA_TABLESIZE(pcrdr_opatoms) == PCRDR_NR_OPERATIONS);
#undef _COMPILE_TIME_ASSERT

static int renderer_init_once(void)
{
    pcinst_register_error_message_segment(&_pcrdr_err_msgs_seg);

    // put the operations into ATOM_BUCKET_RDROP bucket
    for (size_t i = 0; i < PCA_TABLESIZE(pcrdr_opatoms); i++) {
        pcrdr_opatoms[i].atom =
            purc_atom_from_static_string_ex(ATOM_BUCKET_RDROP,
                    pcrdr_opatoms[i].op);

        if (!pcrdr_opatoms[i].atom)
            return -1;
    }

    return 0;
}

struct pcmodule _module_renderer = {
    .id              = PURC_HAVE_VARIANT | PURC_HAVE_PCRDR,
    .module_inited   = 0,

    .init_once       = renderer_init_once,
    .init_instance   = NULL,
};


const char *pcrdr_operation_from_atom(purc_atom_t op_atom, unsigned int *id)
{
    if (op_atom >= pcrdr_opatoms[0].atom &&
            op_atom <= pcrdr_opatoms[PCRDR_K_OPERATION_LAST].atom) {
        *id = op_atom - pcrdr_opatoms[0].atom;
        return pcrdr_opatoms[*id].op;
    }

    return NULL;
}

static const char *prot_names[] = {
    PURC_RDRPROT_NAME_HEADLESS,
    PURC_RDRPROT_NAME_THREAD,
    PURC_RDRPROT_NAME_PURCMC,
    PURC_RDRPROT_NAME_HIBUS,
};

static const int prot_vers[] = {
    PURC_RDRPROT_VERSION_HEADLESS,
    PURC_RDRPROT_VERSION_THREAD,
    PURC_RDRPROT_VERSION_PURCMC,
    PURC_RDRPROT_VERSION_HIBUS,
};

int pcrdr_init_instance(struct pcinst* inst,
        const purc_instance_extra_info *extra_info)
{
    pcrdr_msg *msg = NULL, *response_msg = NULL;
    purc_variant_t session_data;
    purc_rdrprot_t rdr_prot;

    if (extra_info == NULL ||
            extra_info->renderer_prot == PURC_RDRPROT_HEADLESS) {
        rdr_prot = PURC_RDRPROT_HEADLESS;
        msg = pcrdr_headless_connect(
            extra_info ? extra_info->renderer_uri : NULL,
            inst->app_name, inst->runner_name, &inst->conn_to_rdr);
    }
    else if (extra_info->renderer_prot == PURC_RDRPROT_PURCMC) {
        rdr_prot = PURC_RDRPROT_PURCMC;
        msg = pcrdr_purcmc_connect(extra_info->renderer_uri,
            inst->app_name, inst->runner_name, &inst->conn_to_rdr);
    }
    else {
        // TODO: other protocol
        return PURC_ERROR_NOT_SUPPORTED;
    }

    if (msg == NULL) {
        inst->conn_to_rdr = NULL;
        goto failed;
    }

    if (msg->type == PCRDR_MSG_TYPE_RESPONSE && msg->retCode == PCRDR_SC_OK) {
        inst->rdr_caps =
            pcrdr_parse_renderer_capabilities(
                    purc_variant_get_string_const(msg->data));
        if (inst->rdr_caps == NULL) {
            goto failed;
        }
    }
    pcrdr_release_message(msg);

    /* send startSession request and wait for the response */
    msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_SESSION, 0,
            PCRDR_OPERATION_STARTSESSION, NULL,
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
    if (msg == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    purc_variant_t vs[10] = { NULL };
    vs[0] = purc_variant_make_string_static("protocolName", false);
    vs[1] = purc_variant_make_string_static(prot_names[rdr_prot], false);
    vs[2] = purc_variant_make_string_static("protocolVersion", false);
    vs[3] = purc_variant_make_ulongint(prot_vers[rdr_prot]);
    vs[4] = purc_variant_make_string_static("hostName", false);
    vs[5] = purc_variant_make_string_static(inst->conn_to_rdr->own_host_name,
            false);
    vs[6] = purc_variant_make_string_static("appName", false);
    vs[7] = purc_variant_make_string_static(inst->app_name, false);
    vs[8] = purc_variant_make_string_static("runnerName", false);
    vs[9] = purc_variant_make_string_static(inst->runner_name, false);

    session_data = purc_variant_make_object(0, NULL, NULL);
    if (session_data == PURC_VARIANT_INVALID || vs[9] == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }
    for (int i = 0; i < 5; i++) {
        purc_variant_object_set(session_data, vs[i * 2], vs[i * 2 + 1]);
        purc_variant_unref(vs[i * 2]);
        purc_variant_unref(vs[i * 2 + 1]);
    }

    msg->dataType = PCRDR_MSG_DATA_TYPE_EJSON;
    msg->data = session_data;

    if (pcrdr_send_request_and_wait_response(inst->conn_to_rdr,
            msg, PCRDR_TIME_DEF_EXPECTED, &response_msg) < 0) {
        goto failed;
    }
    pcrdr_release_message(msg);
    msg = NULL;

    int ret_code = response_msg->retCode;
    if (ret_code == PCRDR_SC_OK) {
        inst->rdr_caps->session_handle = response_msg->resultValue;
    }

    pcrdr_release_message(response_msg);

    if (ret_code != PCRDR_SC_OK) {
        purc_set_error(PCRDR_ERROR_SERVER_REFUSED);
        goto failed;
    }

    return PURC_ERROR_OK;

failed:
    if (msg)
        pcrdr_release_message(msg);

    if (inst->conn_to_rdr) {
        pcrdr_disconnect(inst->conn_to_rdr);
        inst->conn_to_rdr = NULL;
    }

    return purc_get_last_error();
}

void pcrdr_cleanup_instance(struct pcinst* inst)
{
    if (inst->rdr_caps) {
        pcrdr_release_renderer_capabilities(inst->rdr_caps);
        inst->rdr_caps = NULL;
    }

    if (inst->conn_to_rdr) {
        pcrdr_disconnect(inst->conn_to_rdr);
        inst->conn_to_rdr = NULL;
    }
}

