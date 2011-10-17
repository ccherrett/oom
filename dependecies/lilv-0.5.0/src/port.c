/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lilv_internal.h"

LilvPort*
lilv_port_new(LilvWorld*      world,
              const SordNode* node,
              uint32_t        index,
              const char*     symbol)
{
	LilvPort* port = malloc(sizeof(struct LilvPortImpl));
	port->node    = sord_node_copy(node);
	port->index   = index;
	port->symbol  = lilv_node_new(world, LILV_VALUE_STRING, symbol);
	port->classes = lilv_nodes_new();
	return port;
}

void
lilv_port_free(const LilvPlugin* plugin, LilvPort* port)
{
	sord_node_free(plugin->world->world, port->node);
	lilv_nodes_free(port->classes);
	lilv_node_free(port->symbol);
	free(port);
}

LILV_API
bool
lilv_port_is_a(const LilvPlugin* plugin,
               const LilvPort*   port,
               const LilvNode*   port_class)
{
	LILV_FOREACH(nodes, i, port->classes)
		if (lilv_node_equals(lilv_nodes_get(port->classes, i), port_class))
			return true;

	return false;
}

LILV_API
bool
lilv_port_has_property(const LilvPlugin* p,
                       const LilvPort*   port,
                       const LilvNode*   property)
{
	assert(property);
	SordIter* results = lilv_world_query_internal(
		p->world,
		port->node,
		p->world->lv2_portproperty_node,
		lilv_node_as_node(property));

	const bool ret = !lilv_matches_end(results);
	lilv_match_end(results);
	return ret;
}

LILV_API
bool
lilv_port_supports_event(const LilvPlugin* p,
                         const LilvPort*   port,
                         const LilvNode*   event)
{
#define NS_EV (const uint8_t*)"http://lv2plug.in/ns/ext/event#"

	assert(event);
	SordIter* results = lilv_world_query_internal(
		p->world,
		port->node,
		sord_new_uri(p->world->world, NS_EV "supportsEvent"),
		lilv_node_as_node(event));

	const bool ret = !lilv_matches_end(results);
	lilv_match_end(results);
	return ret;
}

static LilvNodes*
lilv_port_get_value_by_node(const LilvPlugin* p,
                            const LilvPort*   port,
                            const SordNode*   predicate)
{
	assert(sord_node_get_type(predicate) == SORD_URI);

	SordIter* results = lilv_world_query_internal(
		p->world,
		port->node,
		predicate,
		NULL);

	return lilv_nodes_from_stream_objects(p->world, results);
}

LILV_API
LilvNodes*
lilv_port_get_value(const LilvPlugin* p,
                    const LilvPort*   port,
                    const LilvNode*   predicate)
{
	if (!lilv_node_is_uri(predicate)) {
		LILV_ERRORF("Predicate `%s' is not a URI\n", predicate->str_val);
		return NULL;
	}

	return lilv_port_get_value_by_node(
		p, port,
		lilv_node_as_node(predicate));
}

LILV_API
const LilvNode*
lilv_port_get_symbol(const LilvPlugin* p,
                     const LilvPort*   port)
{
	return port->symbol;
}

LILV_API
LilvNode*
lilv_port_get_name(const LilvPlugin* p,
                   const LilvPort*   port)
{
	LilvNodes* results = lilv_port_get_value(p, port,
	                                         p->world->lv2_name_val);

	LilvNode* ret = NULL;
	if (results) {
		LilvNode* val = lilv_nodes_get_first(results);
		if (lilv_node_is_string(val))
			ret = lilv_node_duplicate(val);
		lilv_nodes_free(results);
	}

	if (!ret)
		LILV_WARNF("Plugin <%s> port has no (mandatory) doap:name\n",
		           lilv_node_as_string(lilv_plugin_get_uri(p)));

	return ret;
}

LILV_API
const LilvNodes*
lilv_port_get_classes(const LilvPlugin* p,
                      const LilvPort*   port)
{
	return port->classes;
}

LILV_API
void
lilv_port_get_range(const LilvPlugin* p,
                    const LilvPort*   port,
                    LilvNode**        def,
                    LilvNode**        min,
                    LilvNode**        max)
{
	if (def) {
		LilvNodes* defaults = lilv_port_get_value_by_node(
			p, port, p->world->lv2_default_node);
		*def = defaults
			? lilv_node_duplicate(lilv_nodes_get_first(defaults))
			: NULL;
		lilv_nodes_free(defaults);
	}
	if (min) {
		LilvNodes* minimums = lilv_port_get_value_by_node(
			p, port, p->world->lv2_minimum_node);
		*min = minimums
			? lilv_node_duplicate(lilv_nodes_get_first(minimums))
			: NULL;
		lilv_nodes_free(minimums);
	}
	if (max) {
		LilvNodes* maximums = lilv_port_get_value_by_node(
			p, port, p->world->lv2_maximum_node);
		*max = maximums
			? lilv_node_duplicate(lilv_nodes_get_first(maximums))
			: NULL;
		lilv_nodes_free(maximums);
	}
}

LILV_API
LilvScalePoints*
lilv_port_get_scale_points(const LilvPlugin* p,
                           const LilvPort*   port)
{
	SordIter* points = lilv_world_query_internal(
		p->world,
		port->node,
		sord_new_uri(p->world->world, (const uint8_t*)LILV_NS_LV2 "scalePoint"),
		NULL);

	LilvScalePoints* ret = NULL;
	if (!lilv_matches_end(points))
		ret = lilv_scale_points_new();

	FOREACH_MATCH(points) {
		const SordNode* point = lilv_match_object(points);

		LilvNode* value = lilv_plugin_get_unique(
			p,
			point,
			p->world->rdf_value_node);

		LilvNode* label = lilv_plugin_get_unique(
			p,
			point,
			p->world->rdfs_label_node);

		if (value && label) {
			zix_tree_insert(ret, lilv_scale_point_new(value, label), NULL);
		}
	}
	lilv_match_end(points);

	assert(!ret || lilv_nodes_size(ret) > 0);
	return ret;
}

LILV_API
LilvNodes*
lilv_port_get_properties(const LilvPlugin* p,
                         const LilvPort*   port)
{
	LilvNode* pred = lilv_node_new_from_node(
		p->world, p->world->lv2_portproperty_node);
	LilvNodes* ret = lilv_port_get_value(p, port, pred);
	lilv_node_free(pred);
	return ret;
}
