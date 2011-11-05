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
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lilv-config.h"
#include "lilv_internal.h"

#define NS_UI   (const uint8_t*)"http://lv2plug.in/ns/extensions/ui#"
#define NS_DOAP (const uint8_t*)"http://usefulinc.com/ns/doap#"
#define NS_FOAF (const uint8_t*)"http://xmlns.com/foaf/0.1/"

/** Ownership of @a uri is taken */
LilvPlugin*
lilv_plugin_new(LilvWorld* world, LilvNode* uri, LilvNode* bundle_uri)
{
	assert(bundle_uri);
	LilvPlugin* plugin = malloc(sizeof(struct LilvPluginImpl));
	plugin->world        = world;
	plugin->plugin_uri   = uri;
	plugin->bundle_uri   = bundle_uri;
	plugin->binary_uri   = NULL;
#ifdef LILV_DYN_MANIFEST
	plugin->dynman_uri   = NULL;
#endif
	plugin->plugin_class = NULL;
	plugin->data_uris    = lilv_nodes_new();
	plugin->ports        = NULL;
	plugin->num_ports    = 0;
	plugin->loaded       = false;
	plugin->replaced     = false;

	return plugin;
}

void
lilv_plugin_free(LilvPlugin* p)
{
	lilv_node_free(p->plugin_uri);
	p->plugin_uri = NULL;

	lilv_node_free(p->bundle_uri);
	p->bundle_uri = NULL;

	lilv_node_free(p->binary_uri);
	p->binary_uri = NULL;

#ifdef LILV_DYN_MANIFEST
	lilv_node_free(p->dynman_uri);
	p->dynman_uri = NULL;
#endif

	if (p->ports) {
		for (uint32_t i = 0; i < p->num_ports; ++i) {
			lilv_port_free(p, p->ports[i]);
		}
		free(p->ports);
		p->ports = NULL;
	}

	lilv_nodes_free(p->data_uris);
	p->data_uris = NULL;

	free(p);
}

static LilvNode*
lilv_plugin_get_one(const LilvPlugin* p,
                    const SordNode*   subject,
                    const SordNode*   predicate)
{
	LilvNode* ret    = NULL;
	SordIter* stream = lilv_world_query_internal(
		p->world, subject, predicate, NULL);
	if (!lilv_matches_end(stream)) {
		ret = lilv_node_new_from_node(p->world, lilv_match_object(stream));
	}
	lilv_match_end(stream);
	return ret;
}

LilvNode*
lilv_plugin_get_unique(const LilvPlugin* p,
                       const SordNode*   subject,
                       const SordNode*   predicate)
{
	LilvNode* ret = lilv_plugin_get_one(p, subject, predicate);
	if (!ret) {
		LILV_ERRORF("Multiple values found for (%s %s ...) property\n",
		            sord_node_get_string(subject),
		            sord_node_get_string(predicate));
	}
	return ret;
}

static void
lilv_plugin_load(LilvPlugin* p)
{
	SordNode*       bundle_uri_node  = p->bundle_uri->val.uri_val;
	const SerdNode* bundle_uri_snode = sord_node_to_serd_node(bundle_uri_node);

	SerdEnv*    env    = serd_env_new(bundle_uri_snode);
	SerdReader* reader = sord_new_reader(p->world->model, env, SERD_TURTLE,
	                                     bundle_uri_node);

	// Parse all the plugin's data files into RDF model
	LILV_FOREACH(nodes, i, p->data_uris) {
		const LilvNode* data_uri_val  = lilv_nodes_get(p->data_uris, i);
		const SordNode* data_uri_node = data_uri_val->val.uri_val;
		const char*     data_uri_str  = lilv_node_as_uri(data_uri_val);

		serd_env_set_base_uri(env, sord_node_to_serd_node(data_uri_node));
		serd_reader_add_blank_prefix(reader,
		                             lilv_world_blank_node_prefix(p->world));
		serd_reader_read_file(reader, (const uint8_t*)data_uri_str);
	}

#ifdef LILV_DYN_MANIFEST
	typedef void* LV2_Dyn_Manifest_Handle;
	// Load and parse dynamic manifest data, if this is a library
	if (p->dynman_uri) {
		const char* lib_path = lilv_uri_to_path(lilv_node_as_string(p->dynman_uri));
		void* lib = dlopen(lib_path, RTLD_LAZY);
		if (!lib) {
			LILV_WARNF("Failed to open dynamic manifest %s\n",
			           lilv_node_as_string(p->dynman_uri));
			return;
		}

		typedef int (*OpenFunc)(LV2_Dyn_Manifest_Handle*, const LV2_Feature *const *);
		OpenFunc open_func = (OpenFunc)lilv_dlfunc(lib, "lv2_dyn_manifest_open");
		LV2_Dyn_Manifest_Handle handle = NULL;
		if (open_func)
			open_func(&handle, &dman_features);

		typedef int (*GetDataFunc)(LV2_Dyn_Manifest_Handle handle,
		                           FILE*                   fp,
		                           const char*             uri);
		GetDataFunc get_data_func = (GetDataFunc)lilv_dlfunc(
			lib, "lv2_dyn_manifest_get_data");
		if (get_data_func) {
			serd_env_set_base_uri(
				env, sord_node_to_serd_node(p->dynman_uri->val.uri_val));
			FILE* fd = tmpfile();
			get_data_func(handle, fd, lilv_node_as_string(p->plugin_uri));
			rewind(fd);
			serd_reader_read_file_handle(reader, fd,
			                             (const uint8_t*)"(dyn-manifest)");
			fclose(fd);
		}

		typedef int (*CloseFunc)(LV2_Dyn_Manifest_Handle);
		CloseFunc close_func = (CloseFunc)lilv_dlfunc(lib, "lv2_dyn_manifest_close");
		if (close_func)
			close_func(handle);
	}
#endif
	serd_reader_free(reader);
	serd_env_free(env);

	p->loaded = true;
}

static void
lilv_plugin_load_ports_if_necessary(const LilvPlugin* const_p)
{
	LilvPlugin* p = (LilvPlugin*)const_p;
	if (!p->loaded)
		lilv_plugin_load(p);

	if (!p->ports) {
		p->ports = malloc(sizeof(LilvPort*));
		p->ports[0] = NULL;

		SordIter* ports = lilv_world_query_internal(
			p->world,
			p->plugin_uri->val.uri_val,
			p->world->lv2_port_node,
			NULL);

		FOREACH_MATCH(ports) {
			LilvNode*       index  = NULL;
			const SordNode* port   = lilv_match_object(ports);
			LilvNode*       symbol = lilv_plugin_get_unique(
				p, port, p->world->lv2_symbol_node);

			if (!lilv_node_is_string(symbol)) {
				LILV_ERRORF("Plugin <%s> port symbol is not a string\n",
				            lilv_node_as_uri(p->plugin_uri));
				p->num_ports = 0;
				goto error;
			}

			index = lilv_plugin_get_unique(p, port, p->world->lv2_index_node);

			if (!lilv_node_is_int(index)) {
				LILV_ERRORF("Plugin <%s> port index is not an integer\n",
				            lilv_node_as_uri(p->plugin_uri));
				p->num_ports = 0;
				goto error;
			}

			uint32_t  this_index = lilv_node_as_int(index);
			LilvPort* this_port  = NULL;
			if (p->num_ports > this_index) {
				this_port = p->ports[this_index];
			} else {
				p->ports = realloc(p->ports, (this_index + 1) * sizeof(LilvPort*));
				memset(p->ports + p->num_ports, '\0',
				       (this_index - p->num_ports) * sizeof(LilvPort*));
				p->num_ports = this_index + 1;
			}

			// Havn't seen this port yet, add it to array
			if (!this_port) {
				this_port = lilv_port_new(p->world,
				                          port,
				                          this_index,
				                          lilv_node_as_string(symbol));
				p->ports[this_index] = this_port;
			}

			SordIter* types = lilv_world_query_internal(
				p->world, port, p->world->rdf_a_node, NULL);
			FOREACH_MATCH(types) {
				const SordNode* type = lilv_match_object(types);
				if (sord_node_get_type(type) == SORD_URI) {
					zix_tree_insert(
						this_port->classes,
						lilv_node_new_from_node(p->world, type), NULL);
				} else {
					LILV_WARNF("Plugin <%s> port type is not a URI\n",
					           lilv_node_as_uri(p->plugin_uri));
				}
			}
			lilv_match_end(types);

		error:
			lilv_node_free(symbol);
			lilv_node_free(index);
			if (p->num_ports == 0) {
				if (p->ports) {
					for (uint32_t i = 0; i < p->num_ports; ++i) {
						lilv_port_free(p, p->ports[i]);
					}
					free(p->ports);
					p->ports = NULL;
				}
				break;  // Invalid plugin
			}
		}
		lilv_match_end(ports);
	}
}

void
lilv_plugin_load_if_necessary(const LilvPlugin* p)
{
	if (!p->loaded)
		lilv_plugin_load((LilvPlugin*)p);
}

LILV_API
const LilvNode*
lilv_plugin_get_uri(const LilvPlugin* p)
{
	assert(p);
	assert(p->plugin_uri);
	return p->plugin_uri;
}

LILV_API
const LilvNode*
lilv_plugin_get_bundle_uri(const LilvPlugin* p)
{
	assert(p);
	assert(p->bundle_uri);
	return p->bundle_uri;
}

LILV_API
const LilvNode*
lilv_plugin_get_library_uri(const LilvPlugin* const_p)
{
	LilvPlugin* p = (LilvPlugin*)const_p;
	lilv_plugin_load_if_necessary(p);
	if (!p->binary_uri) {
		// <plugin> lv2:binary ?binary
		SordIter* results = lilv_world_query_internal(
			p->world,
			p->plugin_uri->val.uri_val,
			p->world->lv2_binary_node,
			NULL);
		FOREACH_MATCH(results) {
			const SordNode* binary_node = lilv_match_object(results);
			if (sord_node_get_type(binary_node) == SORD_URI) {
				p->binary_uri = lilv_node_new_from_node(p->world, binary_node);
				break;
			}
		}
		lilv_match_end(results);
	}
	if (!p->binary_uri) {
		LILV_WARNF("Plugin <%s> has no lv2:binary\n",
		           lilv_node_as_uri(lilv_plugin_get_uri(p)));
	}
	return p->binary_uri;
}

LILV_API
const LilvNodes*
lilv_plugin_get_data_uris(const LilvPlugin* p)
{
	return p->data_uris;
}

LILV_API
const LilvPluginClass*
lilv_plugin_get_class(const LilvPlugin* const_p)
{
	LilvPlugin* p = (LilvPlugin*)const_p;
	lilv_plugin_load_if_necessary(p);
	if (!p->plugin_class) {
		// <plugin> a ?class
		SordIter* results = lilv_world_query_internal(
			p->world,
			p->plugin_uri->val.uri_val,
			p->world->rdf_a_node,
			NULL);
		FOREACH_MATCH(results) {
			const SordNode* class_node = lilv_match_object(results);
			if (sord_node_get_type(class_node) != SORD_URI) {
				continue;
			}

			LilvNode* class = lilv_node_new_from_node(p->world, class_node);
			if (!lilv_node_equals(class, p->world->lv2_plugin_class->uri)) {
				const LilvPluginClass* plugin_class = lilv_plugin_classes_get_by_uri(
					p->world->plugin_classes, class);

				if (plugin_class) {
					((LilvPlugin*)p)->plugin_class = plugin_class;
					lilv_node_free(class);
					break;
				}
			}

			lilv_node_free(class);
		}
		lilv_match_end(results);

		if (p->plugin_class == NULL)
			p->plugin_class = p->world->lv2_plugin_class;
	}
	return p->plugin_class;
}

LILV_API
bool
lilv_plugin_verify(const LilvPlugin* plugin)
{
	LilvNode*  rdf_type = lilv_new_uri(plugin->world, LILV_NS_RDF "type");
	LilvNodes* results  = lilv_plugin_get_value(plugin, rdf_type);
	lilv_node_free(rdf_type);
	if (!results) {
		return false;
	}

	lilv_nodes_free(results);
	results = lilv_plugin_get_value(plugin, plugin->world->doap_name_val);
	if (!results) {
		return false;
	}

	lilv_nodes_free(results);
	LilvNode*  lv2_port = lilv_new_uri(plugin->world, LILV_NS_LV2 "port");
	results = lilv_plugin_get_value(plugin, lv2_port);
	lilv_node_free(lv2_port);
	if (!results) {
		return false;
	}

	lilv_nodes_free(results);
	return true;
}

LILV_API
LilvNode*
lilv_plugin_get_name(const LilvPlugin* plugin)
{
	LilvNodes* results = lilv_plugin_get_value(plugin,
	                                           plugin->world->doap_name_val);

	LilvNode* ret = NULL;
	if (results) {
		LilvNode* val = lilv_nodes_get_first(results);
		if (lilv_node_is_string(val))
			ret = lilv_node_duplicate(val);
		lilv_nodes_free(results);
	}

	if (!ret)
		LILV_WARNF("Plugin <%s> has no (mandatory) doap:name\n",
		           lilv_node_as_string(lilv_plugin_get_uri(plugin)));

	return ret;
}

LILV_API
LilvNodes*
lilv_plugin_get_value(const LilvPlugin* p,
                      const LilvNode*   predicate)
{
	lilv_plugin_load_if_necessary(p);
	return lilv_world_find_nodes(p->world, p->plugin_uri, predicate, NULL);
}

LILV_API
uint32_t
lilv_plugin_get_num_ports(const LilvPlugin* p)
{
	lilv_plugin_load_ports_if_necessary(p);
	return p->num_ports;
}

LILV_API
void
lilv_plugin_get_port_ranges_float(const LilvPlugin* p,
                                  float*            min_values,
                                  float*            max_values,
                                  float*            def_values)
{
	lilv_plugin_load_ports_if_necessary(p);
	LilvNode*  min    = NULL;
	LilvNode*  max    = NULL;
	LilvNode*  def    = NULL;
	LilvNode** minptr = min_values ? &min : NULL;
	LilvNode** maxptr = max_values ? &max : NULL;
	LilvNode** defptr = def_values ? &def : NULL;

	for (uint32_t i = 0; i < p->num_ports; ++i) {
		lilv_port_get_range(p, p->ports[i], defptr, minptr, maxptr);

		if (min_values) {
			if (lilv_node_is_float(min)) {
				min_values[i] = lilv_node_as_float(min);
			} else if (lilv_node_is_int(min)) {
				min_values[i] = lilv_node_as_int(min);
			} else {
				min_values[i] = NAN;
			}
		}

		if (max_values) {
			if (lilv_node_is_float(max)) {
				max_values[i] = lilv_node_as_float(max);
			} else if (lilv_node_is_int(max)) {
				max_values[i] = lilv_node_as_int(max);
			} else {
				max_values[i] = NAN;
			}
		}

		if (def_values) {
			if (lilv_node_is_float(def)) {
				def_values[i] = lilv_node_as_float(def);
			} else if (lilv_node_is_int(def)) {
				def_values[i] = lilv_node_as_int(def);
			} else {
				def_values[i] = NAN;
			}
		}

		lilv_node_free(def);
		lilv_node_free(min);
		lilv_node_free(max);
	}
}

LILV_API
uint32_t
lilv_plugin_get_num_ports_of_class(const LilvPlugin* p,
                                   const LilvNode*   class_1, ...)
{
	lilv_plugin_load_ports_if_necessary(p);

	uint32_t ret = 0;
	va_list  args;

	for (unsigned i = 0; i < p->num_ports; ++i) {
		LilvPort* port = p->ports[i];
		if (!port || !lilv_port_is_a(p, port, class_1))
			continue;

		va_start(args, class_1);

		bool matches = true;
		for (LilvNode* class_i = NULL; (class_i = va_arg(args, LilvNode*)); ) {
			if (!lilv_port_is_a(p, port, class_i)) {
				va_end(args);
				matches = false;
				break;
			}
		}

		if (matches)
			++ret;

		va_end(args);
	}

	return ret;
}

LILV_API
bool
lilv_plugin_has_latency(const LilvPlugin* p)
{
	lilv_plugin_load_if_necessary(p);
	SordIter* ports = lilv_world_query_internal(
		p->world,
		p->plugin_uri->val.uri_val,
		p->world->lv2_port_node,
		NULL);

	bool ret = false;
	FOREACH_MATCH(ports) {
		const SordNode* port            = lilv_match_object(ports);
		SordIter*       reports_latency = lilv_world_query_internal(
			p->world,
			port,
			p->world->lv2_portproperty_node,
			p->world->lv2_reportslatency_node);
		const bool end = lilv_matches_end(reports_latency);
		lilv_match_end(reports_latency);
		if (!end) {
			ret = true;
			break;
		}
	}
	lilv_match_end(ports);

	return ret;
}

LILV_API
uint32_t
lilv_plugin_get_latency_port_index(const LilvPlugin* p)
{
	lilv_plugin_load_if_necessary(p);
	SordIter* ports = lilv_world_query_internal(
		p->world,
		p->plugin_uri->val.uri_val,
		p->world->lv2_port_node,
		NULL);

	uint32_t ret = 0;
	FOREACH_MATCH(ports) {
		const SordNode* port            = lilv_match_object(ports);
		SordIter*       reports_latency = lilv_world_query_internal(
			p->world,
			port,
			p->world->lv2_portproperty_node,
			p->world->lv2_reportslatency_node);
		if (!lilv_matches_end(reports_latency)) {
			LilvNode* index = lilv_plugin_get_unique(
				p, port, p->world->lv2_index_node);

			ret = lilv_node_as_int(index);
			lilv_node_free(index);
			lilv_match_end(reports_latency);
			break;
		}
		lilv_match_end(reports_latency);
	}
	lilv_match_end(ports);

	return ret;  // FIXME: error handling
}

LILV_API
bool
lilv_plugin_has_feature(const LilvPlugin* p,
                        const LilvNode*   feature)
{
	LilvNodes* features = lilv_plugin_get_supported_features(p);

	const bool ret = features && feature && lilv_nodes_contains(features, feature);

	lilv_nodes_free(features);
	return ret;
}

LILV_API
LilvNodes*
lilv_plugin_get_supported_features(const LilvPlugin* p)
{
	LilvNodes* optional = lilv_plugin_get_optional_features(p);
	LilvNodes* required = lilv_plugin_get_required_features(p);
	LilvNodes* result   = lilv_nodes_new();

	LILV_FOREACH(nodes, i, optional)
		zix_tree_insert(
			result, lilv_node_duplicate(lilv_nodes_get(optional, i)), NULL);
	LILV_FOREACH(nodes, i, required)
		zix_tree_insert(
			result, lilv_node_duplicate(lilv_nodes_get(required, i)), NULL);

	lilv_nodes_free(optional);
	lilv_nodes_free(required);

	return result;
}

LILV_API
LilvNodes*
lilv_plugin_get_optional_features(const LilvPlugin* p)
{
	return lilv_plugin_get_value(p, p->world->lv2_optionalFeature_val);
}

LILV_API
LilvNodes*
lilv_plugin_get_required_features(const LilvPlugin* p)
{
	return lilv_plugin_get_value(p, p->world->lv2_requiredFeature_val);
}

LILV_API
const LilvPort*
lilv_plugin_get_port_by_index(const LilvPlugin* p,
                              uint32_t          index)
{
	lilv_plugin_load_ports_if_necessary(p);
	if (index < p->num_ports)
		return p->ports[index];
	else
		return NULL;
}

LILV_API
const LilvPort*
lilv_plugin_get_port_by_symbol(const LilvPlugin* p,
                               const LilvNode*   symbol)
{
	lilv_plugin_load_ports_if_necessary(p);
	for (uint32_t i = 0; i < p->num_ports; ++i) {
		LilvPort* port = p->ports[i];
		if (lilv_node_equals(port->symbol, symbol))
			return port;
	}

	return NULL;
}

static const SordNode*
lilv_plugin_get_author(const LilvPlugin* p)
{
	lilv_plugin_load_if_necessary(p);

	SordNode* doap_maintainer = sord_new_uri(
		p->world->world, NS_DOAP "maintainer");

	SordIter* maintainers = lilv_world_query_internal(
		p->world,
		p->plugin_uri->val.uri_val,
		doap_maintainer,
		NULL);

	sord_node_free(p->world->world, doap_maintainer);

	if (lilv_matches_end(maintainers)) {
		return NULL;
	}

	const SordNode* author = lilv_match_object(maintainers);

	lilv_match_end(maintainers);
	return author;
}

LILV_API
LilvNode*
lilv_plugin_get_author_name(const LilvPlugin* plugin)
{
	const SordNode* author = lilv_plugin_get_author(plugin);
	if (author) {
		return lilv_plugin_get_one(
			plugin, author, sord_new_uri(
				plugin->world->world, NS_FOAF "name"));
	}
	return NULL;
}

LILV_API
LilvNode*
lilv_plugin_get_author_email(const LilvPlugin* plugin)
{
	const SordNode* author = lilv_plugin_get_author(plugin);
	if (author) {
		return lilv_plugin_get_one(
			plugin, author, sord_new_uri(
				plugin->world->world, NS_FOAF "mbox"));
	}
	return NULL;
}

LILV_API
LilvNode*
lilv_plugin_get_author_homepage(const LilvPlugin* plugin)
{
	const SordNode* author = lilv_plugin_get_author(plugin);
	if (author) {
		return lilv_plugin_get_one(
			plugin, author, sord_new_uri(
				plugin->world->world, NS_FOAF "homepage"));
	}
	return NULL;
}

LILV_API
bool
lilv_plugin_is_replaced(const LilvPlugin* plugin)
{
	return plugin->replaced;
}

LILV_API
LilvUIs*
lilv_plugin_get_uis(const LilvPlugin* p)
{
	lilv_plugin_load_if_necessary(p);

	SordNode* ui_ui_node     = sord_new_uri(p->world->world, NS_UI "ui");
	SordNode* ui_binary_node = sord_new_uri(p->world->world, NS_UI "binary");

	LilvUIs*  result = lilv_uis_new();
	SordIter* uis    = lilv_world_query_internal(
		p->world,
		p->plugin_uri->val.uri_val,
		ui_ui_node,
		NULL);

	FOREACH_MATCH(uis) {
		const SordNode* ui     = lilv_match_object(uis);
		LilvNode*       type   = lilv_plugin_get_unique(p, ui, p->world->rdf_a_node);
		LilvNode*       binary = lilv_plugin_get_unique(p, ui, ui_binary_node);

		if (sord_node_get_type(ui) != SORD_URI
		    || !lilv_node_is_uri(type)
		    || !lilv_node_is_uri(binary)) {
			lilv_node_free(binary);
			lilv_node_free(type);
			LILV_ERRORF("Corrupt UI <%s>\n", sord_node_get_string(ui));
			continue;
		}

		LilvUI* lilv_ui = lilv_ui_new(
			p->world,
			lilv_node_new_from_node(p->world, ui),
			type,
			binary);

		zix_tree_insert(result, lilv_ui, NULL);
	}
	lilv_match_end(uis);

	sord_node_free(p->world->world, ui_binary_node);
	sord_node_free(p->world->world, ui_ui_node);

	if (lilv_uis_size(result) > 0) {
		return result;
	} else {
		lilv_uis_free(result);
		return NULL;
	}
}

static size_t
file_sink(const void* buf, size_t len, void* stream)
{
	FILE* file = (FILE*)stream;
	return fwrite(buf, 1, len, file);
}

static SerdEnv*
new_lv2_env(const SerdNode* base)
{
	SerdEnv* env = serd_env_new(base);

#define USTR(s) ((const uint8_t*)s)
	serd_env_set_prefix_from_strings(env, USTR("doap"), USTR(LILV_NS_DOAP));
	serd_env_set_prefix_from_strings(env, USTR("foaf"), USTR(LILV_NS_FOAF));
	serd_env_set_prefix_from_strings(env, USTR("lv2"),  USTR(LILV_NS_LV2));
	serd_env_set_prefix_from_strings(env, USTR("owl"),  USTR(LILV_NS_OWL));
	serd_env_set_prefix_from_strings(env, USTR("rdf"),  USTR(LILV_NS_RDF));
	serd_env_set_prefix_from_strings(env, USTR("rdfs"), USTR(LILV_NS_RDFS));
	serd_env_set_prefix_from_strings(env, USTR("xsd"),  USTR(LILV_NS_XSD));

	return env;
}

static void
maybe_write_prefixes(SerdWriter* writer, SerdEnv* env, FILE* file)
{
	fseek(file, 0, SEEK_END);
	if (ftell(file) == 0) {
		serd_env_foreach(
			env, (SerdPrefixSink)serd_writer_set_prefix, writer);
	} else {
		fprintf(file, "\n");
	}
}

LILV_API
void
lilv_plugin_write_description(LilvWorld*        world,
                              const LilvPlugin* plugin,
                              const LilvNode*   base_uri,
                              FILE*             plugin_file)
{
	const LilvNode* subject   = lilv_plugin_get_uri(plugin);
	const uint32_t  num_ports = lilv_plugin_get_num_ports(plugin);
	const SerdNode* base      = sord_node_to_serd_node(base_uri->val.uri_val);
	SerdEnv*        env       = new_lv2_env(base);

	SerdWriter* writer = serd_writer_new(
		SERD_TURTLE,
		SERD_STYLE_ABBREVIATED|SERD_STYLE_CURIED,
		env,
		NULL,
		file_sink,
		plugin_file);

	// Write prefixes if this is a new file
	maybe_write_prefixes(writer, env, plugin_file);

	// Write plugin description
	SordIter* iter = lilv_world_query_internal(
		world, subject->val.uri_val, NULL, NULL);
	sord_write_iter(iter, writer);

	// Write port descriptions
	for (uint32_t i = 0; i < num_ports; ++i) {
		const LilvPort* port = plugin->ports[i];
		SordIter* iter = lilv_world_query_internal(
			world, port->node, NULL, NULL);
		sord_write_iter(iter, writer);
	}

	serd_writer_free(writer);
	serd_env_free(env);
}

LILV_API
void
lilv_plugin_write_manifest_entry(LilvWorld*        world,
                                 const LilvPlugin* plugin,
                                 const LilvNode*   base_uri,
                                 FILE*             manifest_file,
                                 const char*       plugin_file_path)
{
	const LilvNode* subject = lilv_plugin_get_uri(plugin);
	const SerdNode* base    = sord_node_to_serd_node(base_uri->val.uri_val);
	SerdEnv*        env     = new_lv2_env(base);

	SerdWriter* writer = serd_writer_new(
		SERD_TURTLE,
		SERD_STYLE_ABBREVIATED|SERD_STYLE_CURIED,
		env,
		NULL,
		file_sink,
		manifest_file);

	// Write prefixes if this is a new file
	maybe_write_prefixes(writer, env, manifest_file);

	// Write manifest entry
	serd_writer_write_statement(
		writer, 0, NULL,
		sord_node_to_serd_node(subject->val.uri_val),
		sord_node_to_serd_node(plugin->world->rdf_a_node),
		sord_node_to_serd_node(plugin->world->lv2_plugin_node), 0, 0);

	const SerdNode file_node = serd_node_from_string(
		SERD_URI, (const uint8_t*)plugin_file_path);
	serd_writer_write_statement(
		writer, 0, NULL,
		sord_node_to_serd_node(subject->val.uri_val),
		sord_node_to_serd_node(plugin->world->rdfs_seealso_node),
		&file_node, 0, 0);

	serd_writer_free(writer);
	serd_env_free(env);
}
