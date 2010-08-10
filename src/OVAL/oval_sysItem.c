/**
 * @file oval_sysItem.c
 * \brief Open Vulnerability and Assessment Language
 *
 * See more details at http://oval.mitre.org/
 */

/*
 * Copyright 2009 Red Hat Inc., Durham, North Carolina.
 * All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors:
 *      "David Niemoller" <David.Niemoller@g2-inc.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "oval_agent_api_impl.h"
#include "oval_system_characteristics_impl.h"
#include "oval_collection_impl.h"
#include "common/util.h"
#include "common/debug_priv.h"

typedef struct oval_sysitem {
	//oval_family_enum family;
	struct oval_syschar_model *model;
	oval_subtype_t subtype;
	oval_message_level_t message_level;
	char *id;
	char *message;
	struct oval_collection *items;
	oval_syschar_status_t status;
} oval_sysitem_t;

struct oval_sysitem *oval_sysitem_new(struct oval_syschar_model *model, const char *id)
{
	oval_sysitem_t *sysitem;

        if (model && oval_syschar_model_is_locked(model)) {
                oscap_dlprintf(DBG_W, "Attempt to update locked content.\n");
                return NULL;
        }

	sysitem = (oval_sysitem_t *) oscap_alloc(sizeof(oval_sysitem_t));
	if (sysitem == NULL)
		return NULL;

	sysitem->id = oscap_strdup(id);
	sysitem->message_level = OVAL_MESSAGE_LEVEL_NONE;
	sysitem->subtype = OVAL_SUBTYPE_UNKNOWN;
	sysitem->status = SYSCHAR_STATUS_UNKNOWN;
	sysitem->message = NULL;
	sysitem->items = oval_collection_new();
	sysitem->model = model;

	oval_syschar_model_add_sysitem(model, sysitem);

	return sysitem;
}

bool oval_sysitem_is_valid(struct oval_sysitem *sysitem)
{
	bool is_valid = true;
	struct oval_sysent_iterator *sysents_itr;

	if (sysitem == NULL) {
                oscap_dlprintf(DBG_W, "Argument is not valid: NULL.\n");
		return false;
        }

        if (oval_sysitem_get_subtype(sysitem) == OVAL_SUBTYPE_UNKNOWN) {
                oscap_dlprintf(DBG_W, "Argument is not valid: subtype == OVAL_SUBTYPE_UNKNOWN.\n");
                return false;
        }

	/* validate sysents */
	sysents_itr = oval_sysitem_get_items(sysitem);
	while (oval_sysent_iterator_has_more(sysents_itr)) {
		struct oval_sysent *sysent;

		sysent = oval_sysent_iterator_next(sysents_itr);
		if (oval_sysent_is_valid(sysent) != true) {
			is_valid = false;
			break;
		}
	}
	oval_sysent_iterator_free(sysents_itr);
	if (is_valid != true)
		return false;

	return true;
}

bool oval_sysitem_is_locked(struct oval_sysitem *sysitem)
{
	__attribute__nonnull__(sysitem);

	return oval_syschar_model_is_locked(sysitem->model);
}

struct oval_sysitem *oval_sysitem_clone(struct oval_syschar_model *new_model, struct oval_sysitem *old_data)
{
	struct oval_sysitem *new_data = oval_sysitem_new(new_model, oval_sysitem_get_id(old_data));
	char *old_message = oval_sysitem_get_message(old_data);
	if (old_message) {
		oval_sysitem_set_message(new_data, oscap_strdup(old_message));
		oval_sysitem_set_message_level(new_data, oval_sysitem_get_message_level(old_data));
	}

	oval_sysitem_set_status(new_data, oval_sysitem_get_status(old_data));
	oval_sysitem_set_subtype(new_data, oval_sysitem_get_subtype(old_data));

	struct oval_sysent_iterator *old_items = oval_sysitem_get_items(old_data);
	while (oval_sysent_iterator_has_more(old_items)) {
		struct oval_sysent *old_item = oval_sysent_iterator_next(old_items);
		struct oval_sysent *new_item = oval_sysent_clone(new_model, old_item);
		oval_sysitem_add_item(new_data, new_item);
	}
	oval_sysent_iterator_free(old_items);

	return new_data;
}

void oval_sysitem_free(struct oval_sysitem *sysitem)
{
	if (sysitem == NULL)
		return;

	if (sysitem->message != NULL)
		oscap_free(sysitem->message);

	oval_collection_free_items(sysitem->items, (oscap_destruct_func) oval_sysent_free);
	oscap_free(sysitem->id);

	sysitem->id = NULL;
	sysitem->items = NULL;
	sysitem->message = NULL;
	oscap_free(sysitem);
}

bool oval_sysitem_iterator_has_more(struct oval_sysitem_iterator *oc_sysitem)
{
	return oval_collection_iterator_has_more((struct oval_iterator *)
						 oc_sysitem);
}

struct oval_sysitem *oval_sysitem_iterator_next(struct oval_sysitem_iterator
						*oc_sysitem)
{
	return (struct oval_sysitem *)
	    oval_collection_iterator_next((struct oval_iterator *)oc_sysitem);
}

void oval_sysitem_iterator_free(struct oval_sysitem_iterator
				*oc_sysitem)
{
	oval_collection_iterator_free((struct oval_iterator *)oc_sysitem);
}

oval_subtype_t oval_sysitem_get_subtype(struct oval_sysitem *sysitem)
{
	__attribute__nonnull__(sysitem);

	return sysitem->subtype;
}

void oval_sysitem_set_subtype(struct oval_sysitem *sysitem, oval_subtype_t subtype)
{
	if (sysitem && !oval_sysitem_is_locked(sysitem)) {
		sysitem->subtype = subtype;
	} else
		oscap_dlprintf(DBG_W, "Attempt to update locked content.\n");
}

char *oval_sysitem_get_id(struct oval_sysitem *data)
{
	return data->id;
}

char *oval_sysitem_get_message(struct oval_sysitem *data)
{
	return data->message;
}

void oval_sysitem_set_message(struct oval_sysitem *data, char *message)
{
	if (data && !oval_sysitem_is_locked(data)) {
		if (data->message != NULL)
			oscap_free(data->message);
		data->message = (message == NULL) ? NULL : oscap_strdup(message);
	} else
		oscap_dlprintf(DBG_W, "Attempt to update locked content.\n");
}

oval_message_level_t oval_sysitem_get_message_level(struct oval_sysitem *data)
{
	__attribute__nonnull__(data);

	return data->message_level;
}

void oval_sysitem_set_message_level(struct oval_sysitem *data, oval_message_level_t level)
{
	if (data && !oval_sysitem_is_locked(data)) {
		data->message_level = level;
	} else
		oscap_dlprintf(DBG_W, "Attempt to update locked content.\n");
}

struct oval_sysent_iterator *oval_sysitem_get_items(struct oval_sysitem *data)
{
	return (struct oval_sysent_iterator *)oval_collection_iterator(data->items);
}

void oval_sysitem_add_item(struct oval_sysitem *data, struct oval_sysent *item)
{
	if (data && !oval_sysitem_is_locked(data)) {
		oval_collection_add(data->items, item);
	} else
		oscap_dlprintf(DBG_W, "Attempt to update locked content.\n");
}

oval_syschar_status_t oval_sysitem_get_status(struct oval_sysitem *data)
{
	__attribute__nonnull__(data);

	return data->status;
}

void oval_sysitem_set_status(struct oval_sysitem *data, oval_syschar_status_t status)
{
	if (data && !oval_sysitem_is_locked(data)) {
		data->status = status;
	} else
		oscap_dlprintf(DBG_W, "Attempt to update locked content.\n");
}

static void _oval_sysitem_parse_subtag_consume(char *message, void *sysitem)
{
	oval_sysitem_set_message(sysitem, message);
}

static void _oval_sysitem_parse_subtag_item_consumer(struct oval_sysent *item, void *sysitem)
{
	oval_sysitem_add_item(sysitem, item);
}

static int _oval_sysitem_parse_subtag(xmlTextReaderPtr reader, struct oval_parser_context *context, void *client)
{
	struct oval_sysitem *sysitem = client;
	char *tagname = (char *)xmlTextReaderLocalName(reader);
	char *namespace = (char *)xmlTextReaderNamespaceUri(reader);
	int return_code;
	if (strcmp(NAMESPACE_OVALSYS, namespace) == 0) {
		/*This is a message */
		oval_sysitem_set_message_level(sysitem,
					       oval_message_level_parse(reader, "level", OVAL_MESSAGE_LEVEL_INFO));
		return_code = oval_parser_text_value(reader, context, _oval_sysitem_parse_subtag_consume, sysitem);
	} else {
		/*typedef *(oval_sysent_consumer)(struct oval_sysent *, void* client); */
		return_code =
		    oval_sysent_parse_tag(reader, context, _oval_sysitem_parse_subtag_item_consumer, sysitem);
	}
	oscap_free(tagname);
	oscap_free(namespace);
	return return_code;
}

int oval_sysitem_parse_tag(xmlTextReaderPtr reader, struct oval_parser_context *context)
{
	__attribute__nonnull__(context);

	char *tagname = (char *)xmlTextReaderLocalName(reader);
	oval_subtype_t subtype = oval_subtype_parse(reader);
	int return_code;
	if (subtype != OVAL_SUBTYPE_UNKNOWN) {
		char *item_id = (char *)xmlTextReaderGetAttribute(reader, BAD_CAST "id");
		struct oval_sysitem *sysitem = oval_sysitem_get_new(context->syschar_model, item_id);
		oscap_free(item_id);
		item_id = NULL;
		oval_subtype_t sub = oval_subtype_parse(reader);
		oval_sysitem_set_subtype(sysitem, sub);
		oval_syschar_status_t status_enum = oval_syschar_status_parse(reader, "status", SYSCHAR_STATUS_EXISTS);
		oval_sysitem_set_status(sysitem, status_enum);
		return_code = oval_parser_parse_tag(reader, context, &_oval_sysitem_parse_subtag, sysitem);
		/* TODO: -> oscap_dprintf */
		/*
		int numchars = 0;
		char message[2000];
		message[numchars] = '\0';
		numchars = numchars + sprintf(message + numchars, "oval_sysitem_parse_tag::");
		numchars =
		    numchars + sprintf(message + numchars, "\n    sysitem->id            = %s",
				       oval_sysitem_get_id(sysitem));
		numchars =
		    numchars + sprintf(message + numchars, "\n    sysitem->status        = %d",
				       oval_sysitem_get_status(sysitem));
		oval_message_level_t level = oval_sysitem_get_message_level(sysitem);
		if (level > OVAL_MESSAGE_LEVEL_NONE) {
			numchars = numchars + sprintf(message + numchars, "\n    sysitem->message_level = %d", level);
			numchars =
			    numchars + sprintf(message + numchars, "\n    sysitem->message       = %s",
					       oval_sysitem_get_message(sysitem));
		}
		struct oval_sysent_iterator *items = oval_sysitem_get_items(sysitem);
		int numItems;
		for (numItems = 0; oval_sysent_iterator_has_more(items); numItems++)
			oval_sysent_iterator_next(items);
		oval_sysent_iterator_free(items);
		numchars = numchars + sprintf(message + numchars, "\n    sysitem->items.length  = %d", numItems);
		oscap_dprintf(message);	// TODO: make this code in one string ^ */
	} else {
		char *tagnm = (char *)xmlTextReaderLocalName(reader);
		char *namespace = (char *)xmlTextReaderNamespaceUri(reader);
		oscap_dlprintf(DBG_W, "Expected <item>, got <%s:%s>.\n", namespace, tagnm);
		return_code = oval_parser_skip_tag(reader, context);
		oscap_free(tagnm);
		oscap_free(namespace);
	}
	if (return_code != 1) {
		oscap_dlprintf(DBG_W, "Return code is not 1: %d.\n", return_code);
	}
	oscap_free(tagname);

	return return_code;
}

void oval_sysitem_to_print(struct oval_sysitem *sysitem, char *indent, int idx)
{
	char nxtindent[100];

	if (strlen(indent) > 80)
		indent = "....";

	if (idx == 0)
		snprintf(nxtindent, sizeof(nxtindent), "%sSYSDATA.", indent);
	else
		snprintf(nxtindent, sizeof(nxtindent), "%sSYSDATA[%d].", indent, idx);

	/*
	   char* id;
	   oval_subtype_enum subtype;
	   oval_syschar_status_enum status;
	   oval_message_level_enum message_level;
	   char* message;
	   struct oval_collection *items;
	 */
	{			//id
		oscap_dprintf("%sID            = %s\n", nxtindent, oval_sysitem_get_id(sysitem));
	}
	{			//subtype
		oscap_dprintf("%sSUBTYPE       = %d\n", nxtindent, oval_sysitem_get_subtype(sysitem));
	}
	{			//status
		oscap_dprintf("%sSTATUS        = %d\n", nxtindent, oval_sysitem_get_status(sysitem));
	}
	oval_message_level_t level = oval_sysitem_get_message_level(sysitem);
	{			//level
		oscap_dprintf("%sMESSAGE_LEVEL = %d\n", nxtindent, level);
	}
	if (level != OVAL_MESSAGE_LEVEL_NONE) {	//message
		oscap_dprintf("%sMESSAGE       = %s\n", nxtindent, oval_sysitem_get_message(sysitem));
	}
	{			//items
		struct oval_sysent_iterator *items = oval_sysitem_get_items(sysitem);
		int i;
		for (i = 1; oval_sysent_iterator_has_more(items); i++) {
			struct oval_sysent *item = oval_sysent_iterator_next(items);
			oval_sysent_to_print(item, nxtindent, i);
		}
		oval_sysent_iterator_free(items);
	}
}

void oval_sysitem_to_dom(struct oval_sysitem *sysitem, xmlDoc * doc, xmlNode * tag_parent)
{
	if (sysitem) {
		xmlNs *ns_syschar = xmlSearchNsByHref(doc, tag_parent, OVAL_SYSCHAR_NAMESPACE);

		char syschar_namespace[] = "http://oval.mitre.org/XMLSchema/oval-system-characteristics-5";
		oval_subtype_t subtype = oval_sysitem_get_subtype(sysitem);
		if (subtype) {
			const char *family = oval_family_get_text(oval_subtype_get_family(subtype));
			char family_namespace[sizeof(syschar_namespace) + strlen(family) + 2];
			*family_namespace = '\0';
			sprintf(family_namespace, "%s#%s", OVAL_SYSCHAR_NAMESPACE, family);
			const char *subtype_text = oval_subtype_get_text(subtype);
			char tagname[strlen(subtype_text) + 6];
			*tagname = '\0';
			sprintf(tagname, "%s_item", subtype_text);

			xmlNode *tag_sysitem = xmlNewChild(tag_parent, NULL, BAD_CAST tagname, NULL);
			xmlNs *ns_family = xmlNewNs(tag_sysitem, BAD_CAST family_namespace, NULL);
			xmlSetNs(tag_sysitem, ns_family);

			{	//attributes
				xmlNewProp(tag_sysitem, BAD_CAST "id", BAD_CAST oval_sysitem_get_id(sysitem));
				oval_syschar_status_t status_index = oval_sysitem_get_status(sysitem);
				const char *status = oval_syschar_status_get_text(status_index);
				xmlNewProp(tag_sysitem, BAD_CAST "status", BAD_CAST status);
			}
			{	//message
				char *message = oval_sysitem_get_message(sysitem);
				if (message != NULL) {
					xmlNode *tag_message = xmlNewChild
					    (tag_sysitem, ns_syschar, BAD_CAST "message", BAD_CAST message);
					oval_message_level_t idx = oval_sysitem_get_message_level(sysitem);
					const char *level = oval_message_level_text(idx);
					xmlNewProp(tag_message, BAD_CAST "level", BAD_CAST level);
				}
			}

			{	//items
				struct oval_sysent_iterator *items = oval_sysitem_get_items(sysitem);
				while (oval_sysent_iterator_has_more(items)) {
					struct oval_sysent *item = oval_sysent_iterator_next(items);
					oval_sysent_to_dom(item, doc, tag_sysitem);
				}
				oval_sysent_iterator_free(items);
			}
		} else {
			oscap_dprintf
			    ("WARNING: Skipping XML generation of oval_sysitem with subtype OVAL_SUBTYPE_UNKNOWN"
			     "%s(%d)", __FILE__, __LINE__);
		}
	}
}
