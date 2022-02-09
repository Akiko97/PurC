/*
 * @file tokenizer.c
 * @author Xue Shuming
 * @date 2022/02/08
 * @brief The implementation of hvml tokenizer.
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

#include "config.h"

#include "tokenizer.h"

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/dom.h"
#include "private/hvml.h"

#include "hvml-buffer.h"
#include "hvml-rwswrap.h"
#include "hvml-token.h"
#include "hvml-sbst.h"
#include "hvml-attr.h"
#include "hvml-tag.h"

#include <math.h>

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#define PCHVML_NEXT_TOKEN_BEGIN                                         \
struct pchvml_token* pchvml_next_token(struct pchvml_parser* parser,    \
                                          purc_rwstream_t rws)          \
{                                                                       \
    struct pchvml_uc* hvml_uc = NULL;                                   \
    uint32_t character = 0;                                             \
    if (parser->token) {                                                \
        struct pchvml_token* token = parser->token;                     \
        parser->token = NULL;                                           \
        return token;                                                   \
    }                                                                   \
                                                                        \
    pchvml_rwswrap_set_rwstream (parser->rwswrap, rws);                 \
                                                                        \
next_input:                                                             \
    hvml_uc = pchvml_rwswrap_next_char (parser->rwswrap);               \
    if (!hvml_uc) {                                                     \
        return NULL;                                                    \
    }                                                                   \
                                                                        \
    character = hvml_uc->character;                                     \
    if (character == PCHVML_INVALID_CHARACTER) {                        \
        SET_ERR(PCHVML_ERROR_INVALID_UTF8_CHARACTER);                   \
        return NULL;                                                    \
    }                                                                   \
                                                                        \
    if (is_separator(character)) {                                      \
        if (parser->prev_separator == ',' && character == ',') {        \
            SET_ERR(PCHVML_ERROR_UNEXPECTED_COMMA);                     \
            return NULL;                                                \
        }                                                               \
        parser->prev_separator = character;                             \
    }                                                                   \
    else if (!is_whitespace(character)) {                               \
        parser->prev_separator = 0;                                     \
    }                                                                   \
                                                                        \
next_state:                                                             \
    switch (parser->state) {


#define PCHVML_NEXT_TOKEN_END                                           \
    default:                                                            \
        break;                                                          \
    }                                                                   \
    return NULL;                                                        \
}

#define TEMP_BUFFER_TO_VCM_NODE()                                       \
        pchvml_buffer_to_vcm_node(parser->temp_buffer)

#ifdef HVML_DEBUG_PRINT
#define PRINT_STATE(state_name)                                             \
    fprintf(stderr, \
            "in %s|uc=%c|hex=0x%X|stack_is_empty=%d|stack_top=%c|vcm_node->type=%d\n",                              \
            pchvml_get_state_name(state_name), character, character,     \
            ejson_stack_is_empty(), (char)ejson_stack_top(),                \
            (parser->vcm_node != NULL ? (int)parser->vcm_node->type : -1));
#define SET_ERR(err)    do {                                                \
    fprintf(stderr, "error %s:%d %s\n", __FILE__, __LINE__,                 \
            pchvml_get_error_name(err));                                    \
    pcinst_set_error (err);                                                 \
} while (0)
#else
#define PRINT_STATE(state_name)
#define SET_ERR(err)    pcinst_set_error(err)
#endif

#define BEGIN_STATE(state_name)                                             \
    case state_name:                                                        \
    {                                                                       \
        enum pchvml_state curr_state = state_name;                          \
        UNUSED_PARAM(curr_state);                                           \
        PRINT_STATE(curr_state);

#define END_STATE()                                                         \
        break;                                                              \
    }

#define ADVANCE_TO(new_state)                                               \
    do {                                                                    \
        parser->state = new_state;                                          \
        goto next_input;                                                    \
    } while (false)

#define RECONSUME_IN(new_state)                                             \
    do {                                                                    \
        parser->state = new_state;                                          \
        goto next_state;                                                    \
    } while (false)

#define SET_RETURN_STATE(new_state)                                         \
    do {                                                                    \
        parser->return_state = new_state;                                   \
    } while (false)

#define CHECK_TEMPLATE_TAG_AND_SWITCH_STATE(token)                          \
    do {                                                                    \
        const char* name = pchvml_token_get_name(token);                    \
        if (pchvml_token_is_type(token, PCHVML_TOKEN_START_TAG) &&          \
                pchvml_parser_is_template_tag(name)) {                      \
            parser->state = PCHVML_EJSON_DATA_STATE;                        \
        }                                                                   \
    } while (false)

#define RETURN_AND_SWITCH_TO(next_state)                                    \
    do {                                                                    \
        parser->state = next_state;                                         \
        pchvml_parser_save_tag_name(parser);                                \
        pchvml_token_done(parser->token);                                   \
        struct pchvml_token* token = parser->token;                         \
        parser->token = NULL;                                               \
        CHECK_TEMPLATE_TAG_AND_SWITCH_STATE(token);                         \
        return token;                                                       \
    } while (false)

#define RETURN_CURRENT_TOKEN()                                              \
    do {                                                                    \
        pchvml_token_done(parser->token);                                   \
        struct pchvml_token* token = parser->token;                         \
        parser->token = NULL;                                               \
        return token;                                                       \
    } while (false)

#define RETURN_NEW_EOF_TOKEN()                                              \
    do {                                                                    \
        if (parser->token) {                                                \
            struct pchvml_token* token = parser->token;                     \
            parser->token = pchvml_token_new_eof();                         \
            return token;                                                   \
        }                                                                   \
        return pchvml_token_new_eof();                                      \
    } while (false)

#define RETURN_AND_STOP_PARSE()                                             \
    do {                                                                    \
        return NULL;                                                        \
    } while (false)

#define RESET_TEMP_BUFFER()                                                 \
    do {                                                                    \
        pchvml_buffer_reset(parser->temp_buffer);                           \
    } while (false)

#define APPEND_TO_TEMP_BUFFER(c)                                            \
    do {                                                                    \
        pchvml_buffer_append(parser->temp_buffer, c);                       \
    } while (false)

#define IS_TEMP_BUFFER_EMPTY()                                              \
        pchvml_buffer_is_empty(parser->temp_buffer)

#define APPEND_TO_TOKEN_NAME(uc)                                            \
    do {                                                                    \
        pchvml_token_append_to_name(parser->token, uc);                     \
    } while (false)

#define BEGIN_TOKEN_ATTR()                                                  \
    do {                                                                    \
        pchvml_token_begin_attr(parser->token);                             \
    } while (false)

#define END_TOKEN_ATTR()                                                    \
    do {                                                                    \
        pchvml_token_end_attr(parser->token);                               \
    } while (false)

#define APPEND_TO_TOKEN_ATTR_NAME(c)                                        \
    do {                                                                    \
        pchvml_token_append_to_attr_name(parser->token, c);                 \
    } while (false)

static UNUSED_FUNCTION
bool pchvml_parser_is_json_content_tag(const char* name)
{
    if (!name) {
        return false;
    }
    return (strcmp(name, "init") == 0 || strcmp(name, "archedata") == 0);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_operation_tag(const char* name)
{
    if (!name) {
        return false;
    }
    const struct pchvml_tag_entry* entry = pchvml_tag_static_search(name,
            strlen(name));
    return (entry &&
            (entry->cats & (PCHVML_TAGCAT_TEMPLATE | PCHVML_TAGCAT_VERB)));
}

static UNUSED_FUNCTION
void pchvml_parser_save_tag_name(struct pchvml_parser* parser)
{
    if (pchvml_token_is_type (parser->token, PCHVML_TOKEN_START_TAG)) {
        const char* name = pchvml_token_get_name(parser->token);
        parser->tag_is_operation = pchvml_parser_is_operation_tag(name);
        pchvml_buffer_reset(parser->tag_name);
        pchvml_buffer_append_bytes(parser->tag_name,
                name, strlen(name));
    }
    else {
        pchvml_buffer_reset(parser->tag_name);
        parser->tag_is_operation = false;
    }
}

static UNUSED_FUNCTION
bool pchvml_parser_is_appropriate_end_tag(struct pchvml_parser* parser)
{
    const char* name = pchvml_token_get_name(parser->token);
    return pchvml_buffer_equal_to (parser->tag_name, name,
            strlen(name));
}

static UNUSED_FUNCTION
bool pchvml_parser_is_appropriate_tag_name(struct pchvml_parser* parser,
        const char* name)
{
    return pchvml_buffer_equal_to (parser->tag_name, name,
            strlen(name));
}

static UNUSED_FUNCTION
bool pchvml_parser_is_operation_tag_token (struct pchvml_token* token)
{
    const char* name = pchvml_token_get_name(token);
    return pchvml_parser_is_operation_tag(name);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_json_content_tag_token (struct pchvml_token* token)
{
    const char* name = pchvml_token_get_name(token);
    return pchvml_parser_is_json_content_tag(name);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_ordinary_attribute (struct pchvml_token_attr* attr)
{
    const char* name = pchvml_token_attr_get_name(attr);
    const struct pchvml_attr_entry* entry =pchvml_attr_static_search(name,
            strlen(name));
    return (entry && entry->type == PCHVML_ATTR_TYPE_ORDINARY);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_preposition_attribute (
        struct pchvml_token_attr* attr)
{
    const char* name = pchvml_token_attr_get_name(attr);
    const struct pchvml_attr_entry* entry =pchvml_attr_static_search(name,
            strlen(name));
    return (entry && entry->type == PCHVML_ATTR_TYPE_PREP);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_template_tag (const char* name)
{
    const struct pchvml_tag_entry* entry = pchvml_tag_static_search(name,
            strlen(name));
    bool ret = (entry && (entry->id == PCHVML_TAG_ARCHETYPE
                || entry->id == PCHVML_TAG_ERROR
                || entry->id == PCHVML_TAG_EXCEPT));
    return ret;
}

static UNUSED_FUNCTION
bool pchvml_parser_is_in_template (struct pchvml_parser* parser)
{
    const char* name = pchvml_buffer_get_buffer(parser->tag_name);
    return pchvml_parser_is_template_tag(name);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_in_json_content_tag (struct pchvml_parser* parser)
{
    const char* name = pchvml_buffer_get_buffer(parser->tag_name);
    return pchvml_parser_is_json_content_tag(name);
}

static UNUSED_FUNCTION
bool pchvml_parser_is_handle_as_jsonee(struct pchvml_token* token, uint32_t uc)
{
    if (!(uc == '[' || uc == '{' || uc == '$')) {
        return false;
    }

    struct pchvml_token_attr* attr = pchvml_token_get_curr_attr(token);
    const char* name = pchvml_token_attr_get_name(attr);
    if (pchvml_parser_is_operation_tag_token(token)
            && (strcmp(name, "on") == 0 || strcmp(name, "with") == 0)) {
        return true;
    }
    const char* token_name = pchvml_token_get_name(token);
    if (strcmp(name, "via") == 0 && (
                strcmp(token_name, "choose") == 0 ||
                strcmp(token_name, "iterate") == 0 ||
                strcmp(token_name, "reduce") == 0 ||
                strcmp(token_name, "update") == 0)) {
        return true;
    }
    return false;
}

struct pcvcm_node* pchvml_buffer_to_vcm_node(struct pchvml_buffer* buffer)
{
    return buffer ? pcvcm_node_new_string(
                    pchvml_buffer_get_buffer(buffer)) : NULL;
}

#ifdef USE_NEW_TOKENIZER
PCHVML_NEXT_TOKEN_BEGIN


BEGIN_STATE(TOKENIZER_DATA_STATE)
    if (character == '&') {
        SET_RETURN_STATE(TOKENIZER_DATA_STATE);
        ADVANCE_TO(TOKENIZER_CHARACTER_REFERENCE_STATE);
    }
    if (character == '<') {
        if (parser->token) {
            RETURN_AND_SWITCH_TO(TOKENIZER_TAG_OPEN_STATE);
        }
        ADVANCE_TO(TOKENIZER_TAG_OPEN_STATE);
    }
    if (is_eof(character)) {
        RETURN_NEW_EOF_TOKEN();
    }
    RESET_TEMP_BUFFER();
    RECONSUME_IN(TOKENIZER_TAG_CONTENT_STATE);
END_STATE()

BEGIN_STATE(TOKENIZER_TAG_OPEN_STATE)
    if (character == '!') {
        ADVANCE_TO(TOKENIZER_MARKUP_DECLARATION_OPEN_STATE);
    }
    if (character == '/') {
        ADVANCE_TO(TOKENIZER_END_TAG_OPEN_STATE);
    }
    if (is_ascii_alpha(character)) {
        parser->token = pchvml_token_new_start_tag ();
        RECONSUME_IN(TOKENIZER_TAG_NAME_STATE);
    }
    if (character == '?') {
        SET_ERR(PCHVML_ERROR_UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_BEFORE_TAG_NAME);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TOKENIZER_END_TAG_OPEN_STATE)
    if (is_ascii_alpha(character)) {
        parser->token = pchvml_token_new_end_tag();
        RECONSUME_IN(TOKENIZER_TAG_NAME_STATE);
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_MISSING_END_TAG_NAME);
        RETURN_AND_STOP_PARSE();
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_BEFORE_TAG_NAME);
        RETURN_AND_STOP_PARSE();
    }
    SET_ERR(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TOKENIZER_TAG_CONTENT_STATE)
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_BEFORE_TAG_NAME);
        RETURN_AND_STOP_PARSE();
    }
    if (is_whitespace(character)) {
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(TOKENIZER_TAG_CONTENT_STATE);
    }
    if (!IS_TEMP_BUFFER_EMPTY()) {
        struct pcvcm_node* node = TEMP_BUFFER_TO_VCM_NODE();
        if (!node) {
            RETURN_AND_STOP_PARSE();
        }
        RESET_TEMP_BUFFER();
        parser->token = pchvml_token_new_vcm(parser->vcm_node);
        RETURN_CURRENT_TOKEN();
    }
    if (pchvml_parser_is_in_json_content_tag(parser)) {
        RECONSUME_IN(TOKENIZER_JSONTEXT_CONTENT_STATE);
    }
    RECONSUME_IN(TOKENIZER_TEXT_CONTENT_STATE);
END_STATE()

BEGIN_STATE(TOKENIZER_TAG_NAME_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(TOKENIZER_BEFORE_ATTRIBUTE_NAME_STATE);
    }
    if (character == '/') {
        ADVANCE_TO(TOKENIZER_SELF_CLOSING_START_TAG_STATE);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(TOKENIZER_DATA_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    APPEND_TO_TOKEN_NAME(character);
    ADVANCE_TO(TOKENIZER_TAG_NAME_STATE);
END_STATE()

BEGIN_STATE(TOKENIZER_BEFORE_ATTRIBUTE_NAME_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(TOKENIZER_BEFORE_ATTRIBUTE_NAME_STATE);
    }
    if (character == '/' || character == '>') {
        RECONSUME_IN(TOKENIZER_AFTER_ATTRIBUTE_NAME_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '=') {
        SET_ERR(PCHVML_ERROR_UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME);
        RETURN_AND_STOP_PARSE();
    }
    BEGIN_TOKEN_ATTR();
    RECONSUME_IN(TOKENIZER_ATTRIBUTE_NAME_STATE);
END_STATE()

BEGIN_STATE(TOKENIZER_ATTRIBUTE_NAME_STATE)
    if (is_whitespace(character) || character == '>') {
        RECONSUME_IN(TOKENIZER_AFTER_ATTRIBUTE_NAME_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '=') {
        ADVANCE_TO(TOKENIZER_BEFORE_ATTRIBUTE_VALUE_STATE);
    }
    if (character == '"' || character == '\'' || character == '<') {
        SET_ERR(PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME);
        RETURN_AND_STOP_PARSE();
    }
    if (is_attribute_value_operator(character)
            && pchvml_parser_is_operation_tag_token(parser->token)) {
        RESET_TEMP_BUFFER();
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(
        TOKENIZER_SPECIAL_ATTRIBUTE_OPERATOR_IN_ATTRIBUTE_NAME_STATE);
    }
    if (character == '/') {
        RECONSUME_IN(TOKENIZER_AFTER_ATTRIBUTE_NAME_STATE);
    }
    APPEND_TO_TOKEN_ATTR_NAME(character);
    ADVANCE_TO(TOKENIZER_ATTRIBUTE_NAME_STATE);
END_STATE()

BEGIN_STATE(TOKENIZER_AFTER_ATTRIBUTE_NAME_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(TOKENIZER_AFTER_ATTRIBUTE_NAME_STATE);
    }
    if (character == '=') {
        ADVANCE_TO(TOKENIZER_BEFORE_ATTRIBUTE_VALUE_STATE);
    }
    if (character == '>') {
        END_TOKEN_ATTR();
        RETURN_AND_SWITCH_TO(TOKENIZER_DATA_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_NEW_EOF_TOKEN();
    }
    if (is_attribute_value_operator(character)
            && pchvml_parser_is_operation_tag_token(parser->token)) {
        RESET_TEMP_BUFFER();
        APPEND_TO_TEMP_BUFFER(character);
        ADVANCE_TO(
        TOKENIZER_SPECIAL_ATTRIBUTE_OPERATOR_AFTER_ATTRIBUTE_NAME_STATE
        );
    }
    if (pchvml_parser_is_operation_tag_token(parser->token)
        && pchvml_parser_is_preposition_attribute(
                pchvml_token_get_curr_attr(parser->token))) {
        RECONSUME_IN(TOKENIZER_BEFORE_ATTRIBUTE_VALUE_STATE);
    }
    if (character == '/') {
        END_TOKEN_ATTR();
        ADVANCE_TO(TOKENIZER_SELF_CLOSING_START_TAG_STATE);
    }
    END_TOKEN_ATTR();
    BEGIN_TOKEN_ATTR();
    RECONSUME_IN(TOKENIZER_ATTRIBUTE_NAME_STATE);
END_STATE()

BEGIN_STATE(TOKENIZER_BEFORE_ATTRIBUTE_VALUE_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(TOKENIZER_BEFORE_ATTRIBUTE_VALUE_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_NEW_EOF_TOKEN();
    }
    if (character == '>') {
        SET_ERR(PCHVML_ERROR_MISSING_MISSING_ATTRIBUTE_VALUE);
        RETURN_AND_STOP_PARSE();
    }
    if (character == '"') {
        ADVANCE_TO(TOKENIZER_JSONEE_ATTRIBUTE_VALUE_DOUBLE_QUOTED_STATE);
    }
    if (character == '\'') {
        ADVANCE_TO(TOKENIZER_JSONEE_ATTRIBUTE_VALUE_SINGLE_QUOTED_STATE);
    }
    RECONSUME_IN(TOKENIZER_JSONEE_ATTRIBUTE_VALUE_UNQUOTED_STATE);
END_STATE()

BEGIN_STATE(TOKENIZER_AFTER_ATTRIBUTE_VALUE_STATE)
    if (is_whitespace(character)) {
        ADVANCE_TO(TOKENIZER_BEFORE_ATTRIBUTE_NAME_STATE);
    }
    if (character == '/') {
        ADVANCE_TO(TOKENIZER_SELF_CLOSING_START_TAG_STATE);
    }
    if (character == '>') {
        RETURN_AND_SWITCH_TO(TOKENIZER_DATA_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_NEW_EOF_TOKEN();
    }
    SET_ERR(PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_ATTRIBUTES);
    RETURN_AND_STOP_PARSE();
END_STATE()

BEGIN_STATE(TOKENIZER_SELF_CLOSING_START_TAG_STATE)
    if (character == '>') {
        pchvml_token_set_self_closing(parser->token, true);
        RETURN_AND_SWITCH_TO(TOKENIZER_DATA_STATE);
    }
    if (is_eof(character)) {
        SET_ERR(PCHVML_ERROR_EOF_IN_TAG);
        RETURN_NEW_EOF_TOKEN();
    }
    SET_ERR(PCHVML_ERROR_UNEXPECTED_SOLIDUS_IN_TAG);
    RETURN_AND_STOP_PARSE();
END_STATE()

PCHVML_NEXT_TOKEN_END

#endif


