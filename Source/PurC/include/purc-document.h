/**
 * @file purc-document.h
 * @author Vincent Wei
 * @date 2022/07/11
 * @brief The API of target document.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#ifndef PURC_PURC_DOCUMENT_H
#define PURC_PURC_DOCUMENT_H

#include "purc-macros.h"
#include "purc-variant.h"

/** document types */
typedef enum {
    PCDOC_K_TYPE_FIRST = 0,
    PCDOC_K_TYPE_VOID = PCDOC_K_TYPE_FIRST,
#define PCDOC_TYPE_VOID                 "void"
    PCDOC_K_TYPE_PLAIN,
#define PCDOC_TYPE_PLAIN                "plain"
    PCDOC_K_TYPE_HTML,
#define PCDOC_TYPE_HTML                 "html"
    PCDOC_K_TYPE_XML,
#define PCDOC_TYPE_XML                  "xml"
    PCDOC_K_TYPE_XGML,
#define PCDOC_TYPE_XGML                 "xgml"

    /* XXX: change this when you append a new operation */
    PCDOC_K_TYPE_LAST = PCDOC_K_TYPE_XGML,
} purc_document_type;

#define PCDOC_NR_TYPES (PCDOC_K_TYPE_LAST - PCDOC_K_TYPE_FIRST + 1)

/* namespace types */
typedef enum {
    PCDOC_K_NAMESPACE_FIRST = 0,
    PCDOC_K_NAMESPACE__UNDEF = PCDOC_K_NAMESPACE_FIRST,
#define PCDOC_NSNAME__UNDEF   ""
    PCDOC_K_NAMESPACE_HTML,
#define PCDOC_NSNAME_HTML     "html"
    PCDOC_K_NAMESPACE_MATHML,
#define PCDOC_NSNAME_MATHML   "mathml"
    PCDOC_K_NAMESPACE_SVG,
#define PCDOC_NSNAME_SVG      "svg"
    PCDOC_K_NAMESPACE_XGML,
#define PCDOC_NSNAME_XGML     "xgml"
    PCDOC_K_NAMESPACE_XLINK,
#define PCDOC_NSNAME_XLINK    "xlink"
    PCDOC_K_NAMESPACE_XML,
#define PCDOC_NSNAME_XML      "xml"
    PCDOC_K_NAMESPACE_XMLNS,
#define PCDOC_NSNAME_XMLNS    "xmlns"

    /* XXX: change this when you append a new operation */
    PCDOC_K_NAMESPACE_LAST = PCDOC_K_NAMESPACE_XMLNS,
} purc_namespace_type;

#define PCDOC_NR_NAMESPACES     (PCDOC_K_NS_LAST - PCDOC_K_NS_FIRST + 1)

/* Special document type */
#define PCDOC_K_STYPE_INHERIT           "_inherit"

struct purc_document;
typedef struct purc_document purc_document;
typedef struct purc_document *purc_document_t;

struct pcdoc_element;
typedef struct pcdoc_element pcdoc_element;
typedef struct pcdoc_element *pcdoc_element_t;

struct pcdoc_text_node;
typedef struct pcdoc_text_node pcdoc_text_node;
typedef struct pcdoc_text_node *pcdoc_text_node_t;

struct pcdoc_data_node;
typedef struct pcdoc_data_node pcdoc_data_node;
typedef struct pcdoc_data_node *pcdoc_data_node_t;

struct pcdoc_node_others;
typedef struct pcdoc_node_others pcdoc_node_others;
typedef struct pcdoc_node_others *pcdoc_node_others_t;

struct pcdoc_attr;
typedef struct pcdoc_attr pcdoc_attr;
typedef struct pcdoc_attr *pcdoc_attr_t;

typedef enum {
    PCDOC_NODE_ELEMENT = 0,
    PCDOC_NODE_TEXT,
    PCDOC_NODE_DATA,
    PCDOC_NODE_CDATA_SECTION,
    PCDOC_NODE_OTHERS,  // DOCUMENT, DOCTYPE, COMMENT, ...
    PCDOC_NODE_VOID,    // NOTHING
} pcdoc_node_type;

typedef struct {
    pcdoc_node_type         type;
    union {
        void               *data;
        pcdoc_element_t     elem;
        pcdoc_text_node_t   text_node;
        pcdoc_data_node_t   data_node;
        pcdoc_node_others_t others;
    };
} pcdoc_node;

struct pcdoc_elem_coll;
typedef struct pcdoc_elem_coll pcdoc_elem_coll;
typedef struct pcdoc_elem_coll *pcdoc_elem_coll_t;

PCA_EXTERN_C_BEGIN

/**
 * Retrieve the document type for a specific target name.
 *
 * @param target_name: the target name.
 *
 * Returns: A supported document type.
 *
 * Since: 0.2.0
 */
PCA_EXPORT purc_document_type
purc_document_retrieve_type(const char *target_name);

/**
 * Create a new empty document.
 *
 * @param type: the type of the document.
 *
 * This function creates a new empty document in specific type.
 *
 * Returns: a pointer to the document.
 *
 * Since: 0.2.0
 */
PCA_EXPORT purc_document_t
purc_document_new(purc_document_type type);

/**
 * Get the reference count of an existing document.
 *
 * @param doc: the pointer to the document.
 *
 * This function returns the current reference count of the given document.
 *
 * Returns: The current reference count.
 *
 * Since: 0.2.0
 */
PCA_EXPORT unsigned int
purc_document_get_refc(purc_document_t doc);

/**
 * Reference an existing document.
 *
 * @param doc: the pointer to the document.
 *
 * This function increases the reference count of an existing document.
 *
 * Returns: the document.
 *
 * Since: 0.2.0
 */
PCA_EXPORT purc_document_t
purc_document_ref(purc_document_t doc);

/**
 * Unreference an existing document.
 *
 * @param doc: the pointer to the document.
 *
 * This function decreases the reference count of an existing document,
 * when the reference count reaches zero, the function will delete
 * the document by calling `purc_document_delete()`.
 *
 * Returns: The new reference count. If it is zero, that means the document
 *  has been deleted.
 *
 * Since: 0.2.0
 */
PCA_EXPORT unsigned int
purc_document_unref(purc_document_t doc);

/**
 * Create a new document by loading a content.
 *
 * @param type: a string contains the type of the document.
 * @param content: a string contains the content to load.
 * @param len: the len of the content, 0 for null-terminated string.
 *
 * This function creates a new empty document in specific type.
 *
 * Returns: a pointer to the document.
 *
 * Since: 0.2.0
 */
PCA_EXPORT purc_document_t
purc_document_load(purc_document_type type, const char *content, size_t len);

/**
 * Get the underlying implementation entity of a document.
 *
 * @param doc: The pointer to the document.
 * @param type: A location to return the document type.
 *
 * Returns: a pointer to the underlying implementation.
 *
 * Since: 0.9.0
 */
PCA_EXPORT void *
purc_document_impl_entity(purc_document_t doc, purc_document_type *type);

/**
 * Delete a document.
 *
 * @param doc: The pointer to the document.
 *
 * This function deletes a document.
 *
 * Returns: The reference count when deleting the document.
 *
 * Since: 0.2.0
 */
PCA_EXPORT unsigned int
purc_document_delete(purc_document_t doc);

typedef enum {
    PCDOC_SPECIAL_ELEM_ROOT = 0,
    PCDOC_SPECIAL_ELEM_HEAD,
    PCDOC_SPECIAL_ELEM_BODY,
} pcdoc_special_elem;

PCA_EXPORT pcdoc_element_t
purc_document_special_elem(purc_document_t doc, pcdoc_special_elem elem);

static inline pcdoc_element_t purc_document_root(purc_document_t doc)
{
    return purc_document_special_elem(doc, PCDOC_SPECIAL_ELEM_ROOT);
}

static inline pcdoc_element_t purc_document_head(purc_document_t doc)
{
    return purc_document_special_elem(doc, PCDOC_SPECIAL_ELEM_HEAD);
}

static inline pcdoc_element_t purc_document_body(purc_document_t doc)
{
    return purc_document_special_elem(doc, PCDOC_SPECIAL_ELEM_BODY);
}

typedef enum {
    PCDOC_OP_APPEND = 0,
    PCDOC_OP_PREPEND,
    PCDOC_OP_INSERTBEFORE,
    PCDOC_OP_INSERTAFTER,
    PCDOC_OP_DISPLACE,
    PCDOC_OP_UPDATE,
    PCDOC_OP_ERASE,
    PCDOC_OP_CLEAR,
    PCDOC_OP_UNKNOWN,
} pcdoc_operation;

/**
 * Create a new element with specific tag and insert it to
 * the specified position related to the specific element.
 *
 * @param elem: the pointer to an element.
 * @param op: The operation.
 * @param content: a string contains the content in the target markup language.
 * @param len: the len of the content, 0 for null-terminated string.
 *
 * Returns: The pointer to the new element.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_element_t
pcdoc_element_new_element(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *tag, bool self_close);

/**
 * Clear the content of an element.
 *
 * @param elem: the pointer to an element.
 *
 * Since: 0.2.0
 */
PCA_EXPORT void
pcdoc_element_clear(purc_document_t doc, pcdoc_element_t elem);

/**
 * Remove an element.
 *
 * @param elem: the pointer to an element.
 *
 * Since: 0.2.0
 */
PCA_EXPORT void
pcdoc_element_erase(purc_document_t doc, pcdoc_element_t elem);

/**
 * Create a new text content and insert it to the specified position related
 * to the specific element.
 *
 * @param elem: the pointer to an element.
 * @param op: The operation.
 * @param content: a string contains the content in the target markup language.
 * @param len: the len of the content, 0 for null-terminated string.
 *
 * Returns: The pointer to the new text content.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_text_node_t
pcdoc_element_new_text_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *text, size_t len);

/**
 * Set the data content of an element.
 *
 * @param elem: the pointer to an element.
 * @param vrt: the data in PurC variant.
 *
 * Returns: The pointer to the new data content.
 *
 * Note that only XGML supports the data content.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_data_node_t
pcdoc_element_set_data_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        purc_variant_t data);

/**
 * Insert or replace the content in target markup language of the specific
 * element.
 *
 * @param doc: the pointer to the document.
 * @param elem: the pointer to an element.
 * @param op: The operation.
 * @param content: a string contains the content in the target markup language.
 * @param len: the len of the content, 0 for null-terminated string.
 *
 * Returns: the root node of the content.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_node
pcdoc_element_new_content(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *content, size_t len);

/**
 * Get the tag name of a specific element.
 *
 * @param doc: the pointer to the document.
 * @param elem: the pointer to the element.
 * @param local_name: the location to return the local name of the element.
 * @param local_len: the location to return the length of the local name.
 * @param prefix: the location to return the prefix of the tag name of the element.
 * @param prefix_len: the location to return the length of the prefix.
 * @param ns_name: the location to return the namespace name of the element.
 * @param ns_len: the location to return the length of the namespace name.
 *
 * Returns: 0 for success, -1 for failure.
 *
 * Since: 0.9.0
 */
PCA_EXPORT int
pcdoc_element_get_tag_name(purc_document_t doc, pcdoc_element_t elem,
        const char **local_name, size_t *local_len,
        const char **prefix, size_t *prefix_len,
        const char **ns_name, size_t *ns_len);

/**
 * Set an attribute of the specified element.
 *
 * @param doc: the pointer to the document.
 * @param elem: the pointer to the element.
 * @param op: The operation, can be one of the following values:
 *  - PCDOC_OP_UPDATE: update the attribute value.
 *  - PCDOC_OP_ERASE: remove the attribute.
 *  - PCDOC_OP_CLEAR: clear the attribute value.
 * @param name: the name of the attribute.
 * @param value: the value of the attribute (nullable).
 *
 * Returns: 0 for success, -1 for failure.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_element_set_attribute(purc_document_t doc,
        pcdoc_element_t elem, pcdoc_operation op,
        const char *name, const char *val, size_t len);

static inline int pcdoc_element_remove_attribute(purc_document_t doc,
        pcdoc_element_t elem, const char *name)
{
    return pcdoc_element_set_attribute(doc, elem, PCDOC_OP_ERASE,
        name, NULL, 0);
}

/**
 * Get the attribute value of the specific attribute of the specified element.
 *
 * @param doc: the pointer to the document.
 * @param elem: the pointer to the element.
 * @param name: the name of the attribute.
 * @param val: the buffer to return the value of the attribute.
 * @param len (nullable): the buffer to return the length of the value.
 *
 * Returns: 0 for success, -1 for failure.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_element_get_attribute(purc_document_t doc, pcdoc_element_t elem,
        const char *name, const char **val, size_t *len);

typedef enum {
    PCDOC_ATTR_ID = 0,
    PCDOC_ATTR_CLASS,
} pcdoc_special_attr;

/**
 * Get the value of a special attribute of the speicified element.
 *
 * @param doc: the pointer to the doc.
 * @param elem: the pointer to the element.
 * @param which: which special attribute, can be one of the following values:
 *      - PCDOC_ATTR_ID
 *      - PCDOC_ATTR_CLASS
 * @param val: the buffer to return the value of the attribute.
 * @param len (nullable): the buffer to return the length of the value.
 *
 * Returns: 0 for success, -1 for no that attribute defined.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_element_get_special_attr(purc_document_t doc, pcdoc_element_t elem,
        pcdoc_special_attr which, const char **val, size_t *len);

/**
 * Get the value of `id` attribute of the specified element.
 *
 * @param doc: the pointer to the doc.
 * @param elem: the pointer to the element.
 * @param val: the buffer to return the value of the attribute.
 * @param len (nullable): the buffer to return the length of the value.
 *
 * Returns: 0 for success, -1 for no `id` attribute defined.
 *
 * Since: 0.2.0
 */
static inline const char *
pcdoc_element_id(purc_document_t doc, pcdoc_element_t elem, size_t *len)
{
    const char *val;

    if (pcdoc_element_get_special_attr(doc, elem, PCDOC_ATTR_ID,
                &val, len)) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    return val;
}

/**
 * Get the value of `class` attribute of the specified element.
 *
 * @param doc: the pointer to the doc.
 * @param elem: the pointer to the element.
 * @param val: the buffer to return the value of the attribute.
 * @param len (nullable): the buffer to return the length of the value.
 *
 * Returns: 0 for success, -1 for no `class` attribute defined.
 *
 * Since: 0.2.0
 */
static inline const char *
pcdoc_element_class(purc_document_t doc, pcdoc_element_t elem, size_t *len)
{
    const char *val;

    if (pcdoc_element_get_special_attr(doc, elem, PCDOC_ATTR_CLASS,
                &val, len)) {
        if (len != NULL) {
            *len = 0;
        }

        return NULL;
    }

    return val;
}

/**
 * Check whether a class name is defined for the specified element.
 *
 * @param doc: the pointer to the doc.
 * @param elem: the pointer to the element.
 * @param klass: the pointer to the class name.
 * @param found: the buffer to return the result.
 *
 * Returns: 0 for success (the reult is reliable), -1 for bad class name or
 *  other errors.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_element_has_class(purc_document_t doc, pcdoc_element_t elem,
        const char *klass, bool *found);

typedef int (*pcdoc_attribute_cb)(purc_document_t doc, pcdoc_attr_t attr,
        const char *name, size_t name_len,
        const char *value, size_t value_len, void *ctxt);

/**
 * Travel all attributes of the specified element.
 *
 * @param doc: the pointer to the doc.
 * @param element: the element.
 * @param cb: the callback for the attribute found.
 * @param ctxt: the context data will be passed to the callback.
 * @param n: the buffer to returned the number of attributes travelled.
 *
 * Returns: 0 for all attributes travelled, otherwise the traverse was broken
 * by the callback.
 *
 * Since: 0.9.0
 */
PCA_EXPORT int
pcdoc_element_travel_attributes(purc_document_t doc, pcdoc_element_t element,
        pcdoc_attribute_cb cb, void *ctxt, size_t *n);

/**
 * Get the first attribute of the specified element.
 *
 * @param doc: the pointer to the document.
 * @param elem: the element.
 *
 * Returns: The pointer to the desired attribute, NULL for no such attribute.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_attr_t
pcdoc_element_first_attr(purc_document_t doc, pcdoc_element_t elem);

/**
 * Get the last attribute of the specified element.
 *
 * @param doc: the pointer to the document.
 * @param elem: the element.
 *
 * Returns: The pointer to the desired attribute, NULL for no such attribute.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_attr_t
pcdoc_element_last_attr(purc_document_t doc, pcdoc_element_t elem);

/**
 * Get the next sibling of the current attribute.
 *
 * @param doc: the pointer to the document.
 * @param attr: the current attribute.
 *
 * Returns: The pointer to the desired attribute, NULL for no such attribute.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_attr_t
pcdoc_attr_next_sibling(purc_document_t doc, pcdoc_attr_t attr);

/**
 * Get the previous sibling of the current attribute.
 *
 * @param doc: the pointer to the document.
 * @param attr: the current attribute.
 *
 * Returns: The pointer to the desired attribute, NULL for no such attribute.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_attr_t
pcdoc_attr_prev_sibling(purc_document_t doc, pcdoc_attr_t attr);

/**
 * Get the name and value of a specific attribute.
 *
 * @param doc: the pointer to the document.
 * @param attr: the pointer to the attribute.
 * @param local_name: the pointer to the location to return
 *      the local name of the attribute.
 * @param local_len: the pointer to the location to return
 *      the length of the local name.
 * @param qualified_name (nullable): the pointer to the location to return
 *      the qualified name (with prefix) of the attribute.
 * @param qualified_len (nullable): the pointer to the location to return
 *      the length of the qualified name.
 * @param value (nullable): the pointer to the location to return
 *      the value of the attribute.
 * @param value_len (nullable): the pointer to the location to return
 *      the length of the value.
 *
 * Returns: 0 for success, -1 for failure.
 *
 * Since: 0.9.0
 */
PCA_EXPORT int
pcdoc_attr_get_info(purc_document_t doc, pcdoc_attr_t attr,
        const char **local_name, size_t *local_len,
        const char **qualified_name, size_t *qualified_len,
        const char **value, size_t *value_len);

/**
 * Get the text of a text node.
 *
 * @param doc: the pointer to the doc.
 * @param text_ndoe: the pointer to the text node.
 * @param text: the buffer to return the pointer to the text content.
 * @param len (nullable): the buffer to return the length of the text.
 *
 * Returns: 0 for success (the reult is reliable), -1 for errors.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_text_content_get_text(purc_document_t doc, pcdoc_text_node_t text_node,
        const char **text, size_t *len);

/**
 * Get the data of a data node.
 *
 * @param doc: the pointer to the doc.
 * @param data_node: the pointer to the data node.
 * @param data: the buffer to return the variant.
 *
 * Returns: 0 for success (the reult is reliable), -1 on failure.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_data_content_get_data(purc_document_t doc, pcdoc_data_node_t data_node,
        purc_variant_t *data);

/**
 * Get the first child node of the specified element.
 *
 * @param doc: the pointer to the doc.
 * @param elem: the element.
 *
 * Returns: The desired node, PCDOC_NODE_VOID type for no such node.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_node
pcdoc_element_first_child(purc_document_t doc, pcdoc_element_t elem);

/**
 * Get the last child node of the specified element.
 *
 * @param doc: the pointer to the doc.
 * @param elem: the element.
 *
 * Returns: The desired node, PCDOC_NODE_VOID type for no such node.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_node
pcdoc_element_last_child(purc_document_t doc, pcdoc_element_t elem);

/**
 * Get the next sibling node of the specified node.
 *
 * @param doc: the pointer to the doc.
 * @param node: the node.
 *
 * Returns: The desired node, PCDOC_NODE_VOID type for no such node.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_node
pcdoc_node_next_sibling(purc_document_t doc, pcdoc_node node);

/**
 * Get the previous sibling node of the specified node.
 *
 * @param doc: the pointer to the doc.
 * @param node: the node.
 *
 * Returns: The desired node, PCDOC_NODE_VOID type for no such node.
 *
 * Since: 0.9.0
 */
PCA_EXPORT pcdoc_node
pcdoc_node_prev_sibling(purc_document_t doc, pcdoc_node node);

/**
 * Get the user data of the specified node.
 *
 * @param doc: the pointer to the doc.
 * @param node: the node.
 * @param user_data The pointer to the location to receive the user data.
 *
 * Returns: 0 for success, -1 on failure.
 *
 * Since: 0.9.0
 */
PCA_EXPORT int
pcdoc_node_get_user_data(purc_document_t doc, pcdoc_node node,
        void **user_data);

/**
 * Set the user data of the specified node.
 *
 * @param doc: the pointer to the doc.
 * @param node: the node.
 * @param user_data The the user data to set.
 *
 * Returns: 0 for success, -1 on failure.
 *
 * Since: 0.9.0
 */
PCA_EXPORT int
pcdoc_node_set_user_data(purc_document_t doc, pcdoc_node node,
        void *user_data);

/**
 * Get the number of different children nodes of the speicified element.
 *
 * @param doc: the document.
 * @param elem: the element.
 * @param nr_elements (nullable):
 *      the buffer to return the number of children elements.
 * @param nr_text_nodes (nullable):
 *      the buffer to return the number of text nodes.
 * @param nr_data_nodes (nullable):
 *      the buffer to return the number of data nodes.
 *
 * Returns: 0 for success, -1 on failure.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_element_children_count(purc_document_t doc, pcdoc_element_t elem,
        size_t *nr_elements, size_t *nr_text_nodes, size_t *nr_data_nodes);

/**
 * Get the specified child element of an element.
 *
 * @param doc: the document.
 * @param elem: the element.
 * @param idx: the index of the child element.
 *
 * Returns: the pointer to the child element; @NULL for the bad index value.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_element_t
pcdoc_element_get_child_element(purc_document_t doc, pcdoc_element_t elem,
        size_t idx);

/**
 * Get the specified child text node of an element.
 *
 * @param doc: the document.
 * @param elem: the element.
 * @param idx: the index of the child text node.
 *
 * Returns: the pointer to the child text node; @NULL for the bad index value.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_text_node_t
pcdoc_element_get_child_text_node(purc_document_t doc, pcdoc_element_t elem,
        size_t idx);

/**
 * Get the specified child data node of an element.
 *
 * @param doc: the document.
 * @param elem: the element.
 * @param idx: the index of the child data node.
 *
 * Returns: the pointer to the child data node; @NULL for the bad index value.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_data_node_t
pcdoc_element_get_child_data_node(purc_document_t doc, pcdoc_element_t elem,
        size_t idx);

/**
 * Get the parent element of a document node.
 *
 * Returns: the pointer to the parent element or @NULL if the node is the root.
 *
 * Since: 0.2.0
 */
PCA_EXPORT pcdoc_element_t
pcdoc_node_get_parent(purc_document_t doc, pcdoc_node node);

typedef int (*pcdoc_element_cb)(purc_document_t doc,
        pcdoc_element_t element, void *ctxt);

/**
 * Travel all descendant elements in the subtree.
 *
 * @param doc: the pointer to the doc.
 * @param ancestor (nullable): the ancestor of the subtree.
 *      @NULL for the root element of the document.
 * @param cb: the callback for the element travelled.
 * @param ctxt: the context data will be passed to the callback.
 * @param n: the buffer to returned the number of elements travelled.
 *
 * Returns: 0 for all descendants travelled, otherwise the traverse was broken
 * by the callback.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_travel_descendant_elements(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_element_cb cb, void *ctxt, size_t *n);

typedef int (*pcdoc_text_node_cb)(purc_document_t doc,
        pcdoc_text_node_t text_node, void *ctxt);

/**
 * Travel all descendant text nodes in the subtree.
 *
 * @param doc: the pointer to the doc.
 * @param ancestor (nullable): the ancestor of the subtree.
 *      @NULL for the root element of the document.
 * @param cb: the callback for the text node travelled.
 * @param ctxt: the context data will be passed to the callback.
 * @param n: the buffer to returned the number of text nodes travelled.
 *
 * Returns: 0 for all descendant text nodes travelled, otherwise the traverse
 * was broken by the callback.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_travel_descendant_text_nodes(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_text_node_cb cb, void *ctxt, size_t *n);

typedef int (*pcdoc_data_node_cb)(purc_document_t doc,
        pcdoc_data_node_t data_node, void *ctxt);

/**
 * Travel all descendant data nodes in the subtree.
 *
 * @param doc: the pointer to the doc.
 * @param ancestor (nullable): the ancestor of the subtree.
 *      @NULL for the root element of the document.
 * @param cb: the callback for the data node travelled.
 * @param ctxt: the condata data will be passed to the callback.
 * @param n: the buffer to returned the number of data nodes travelled.
 *
 * Returns: 0 for all descendant data nodes travelled, otherwise the traverse
 * was broken by the callback.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_travel_descendant_data_nodes(purc_document_t doc,
        pcdoc_element_t ancestor, pcdoc_data_node_cb cb, void *ctxt, size_t *n);

/* XXX: keep sync with pchtml_html_serialize_opt in purc-html.h */
enum pcdoc_serialize_opt {
    PCDOC_SERIALIZE_OPT_UNDEF               = 0x00,
    PCDOC_SERIALIZE_OPT_SKIP_WS_NODES       = 0x01,
    PCDOC_SERIALIZE_OPT_SKIP_COMMENT        = 0x02,
    PCDOC_SERIALIZE_OPT_RAW                 = 0x04,
    PCDOC_SERIALIZE_OPT_WITHOUT_CLOSING     = 0x08,
    PCDOC_SERIALIZE_OPT_TAG_WITH_NS         = 0x10,
    PCDOC_SERIALIZE_OPT_WITHOUT_TEXT_INDENT = 0x20,
    PCDOC_SERIALIZE_OPT_FULL_DOCTYPE        = 0x40,
    PCDOC_SERIALIZE_OPT_WITH_HVML_HANDLE    = 0x80
};

/**
 * Serialize text contents (including contents of all descendants) in
 * the subtree of the specific document to a stream.
 *
 * @param doc: the document.
 * @param ancestor (nullable): the ancestor of the subtree.
 *      @NULL for the root element of the document.
 * @param opts: the serialization options.
 * @param out: the output stream.
 *
 * Returns: 0 for succes, -1 for errors.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_serialize_text_contents_to_stream(purc_document_t doc,
        pcdoc_element_t ancestor, unsigned opts, purc_rwstream_t out);

static inline int
purc_document_serialize_text_contents_to_stream(purc_document_t doc,
        unsigned opts, purc_rwstream_t out)
{
    return pcdoc_serialize_text_contents_to_stream(doc, NULL, opts, out);
}

/**
 * Serialize all descendant elements (as well as the contents) in the subtree
 * of the specific document to a stream.
 *
 * @param doc: the document.
 * @param ancestor (nullable): the ancestor of the subtree.
 *      @NULL for the root element of the document.
 * @param opts: the serialization options.
 * @param out: the output stream.
 *
 * Returns: 0 for succes, -1 for errors.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
pcdoc_serialize_descendants_to_stream(purc_document_t doc,
        pcdoc_element_t ancestor, unsigned opts, purc_rwstream_t out);

/**
 * Serialize whole document to a stream.
 *
 * @param doc: the document.
 * @param opts: the serialization options.
 * @param out: the output stream.
 *
 * Returns: 0 for succes, -1 for errors.
 *
 * Since: 0.2.0
 */
PCA_EXPORT int
purc_document_serialize_contents_to_stream(purc_document_t doc,
        unsigned opts, purc_rwstream_t out);

/**
 * Find the first element matching the CSS selector from the descendants.
 *
 * Returns: the pointer to the matching element or @NULL if no such one.
 *
 * Note: Unimplemented.
 */
PCA_EXPORT pcdoc_element_t
pcdoc_find_element_in_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, const char *selector);

/**
 * Find the first element matching the CSS selector in the document.
 *
 * Returns: the pointer to the matching element or @NULL if no such one.
 *
 * Note: Unimplemented.
 */
static inline pcdoc_element_t
pcdoc_find_element_in_document(purc_document_t doc, const char *selector)
{
    return pcdoc_find_element_in_descendants(doc, NULL, selector);
}

/**
 * Create an element collection by selecting the elements from the descendants
 * of the specified element according to the CSS selector.
 *
 * Returns: A pointer to the element collection; @NULL on failure.
 *
 * Note: Unimplemented.
 */
PCA_EXPORT pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, const char *selector);

/**
 * Create an element collection by selecting the elements from
 * the whole document according to the CSS selector.
 *
 * Returns: A pointer to the element collection; @NULL on failure.
 *
 * Note: Unimplemented.
 */
static inline pcdoc_elem_coll_t
pcdoc_elem_coll_new_from_document(purc_document_t doc,
        const char *selector)
{
    return pcdoc_elem_coll_new_from_descendants(doc, NULL, selector);
}

/**
 * Create a new element collection by selecting a part of elements
 * in the specific element collection.
 *
 * Returns: A pointer to the new element collection; @NULL on failure.
 *
 * Note: Unimplemented.
 */
PCA_EXPORT pcdoc_elem_coll_t
pcdoc_elem_coll_select(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll, const char *selector);

/**
 * Delete the speicified element collection.
 *
 * Note: Unimplemented.
 */
PCA_EXPORT void
pcdoc_elem_coll_delete(purc_document_t doc,
        pcdoc_elem_coll_t elem_coll);

PCA_EXTERN_C_END

#endif  /* PURC_PURC_DOCUMENT_H */

