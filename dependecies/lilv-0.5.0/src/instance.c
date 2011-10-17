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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lilv_internal.h"

LILV_API
LilvInstance*
lilv_plugin_instantiate(const LilvPlugin*        plugin,
                        double                   sample_rate,
                        const LV2_Feature*const* features)
{
	LilvInstance* result = NULL;

	const LV2_Feature** local_features = NULL;
	if (features == NULL) {
		local_features = malloc(sizeof(LV2_Feature));
		local_features[0] = NULL;
	}

	const LilvNode* const lib_uri_node = lilv_plugin_get_library_uri(plugin);
	const char* const     lib_uri      = lilv_node_as_uri(lib_uri_node);
	const char* const     lib_path     = lilv_uri_to_path(lib_uri);

	if (!lib_path)
		return NULL;

	dlerror();
	void* lib = dlopen(lib_path, RTLD_NOW);
	if (!lib) {
		LILV_ERRORF("Failed to open library %s (%s)\n", lib_path, dlerror());
		return NULL;
	}

	LV2_Descriptor_Function df = (LV2_Descriptor_Function)
		lilv_dlfunc(lib, "lv2_descriptor");

	if (!df) {
		LILV_ERRORF("Failed to find symbol 'lv2_descriptor' in %s\n", lib_path);
		dlclose(lib);
		return NULL;
	} else {
		// Search for plugin by URI

		const char* bundle_path = lilv_uri_to_path(
			lilv_node_as_uri(lilv_plugin_get_bundle_uri(plugin)));

		for (uint32_t i = 0; true; ++i) {
			const LV2_Descriptor* ld = df(i);
			if (!ld) {
				LILV_ERRORF("Failed to find plugin <%s> in %s\n",
				            lilv_node_as_uri(lilv_plugin_get_uri(plugin)),
				            lib_path);
				dlclose(lib);
				break;  // return NULL
			}

			// Parse bundle URI to use as base URI
			const LilvNode* bundle_uri     = lilv_plugin_get_bundle_uri(plugin);
			const char*     bundle_uri_str = lilv_node_as_uri(bundle_uri);
			SerdURI         base_uri;
			if (serd_uri_parse((const uint8_t*)bundle_uri_str, &base_uri)) {
				dlclose(lib);
				break;
			}

			// Resolve library plugin URI against base URI
			SerdURI  abs_uri;
			SerdNode abs_uri_node = serd_node_new_uri_from_string(
				(const uint8_t*)ld->URI, &base_uri, &abs_uri);
			if (!abs_uri_node.buf) {
				LILV_ERRORF("Failed to parse plugin URI `%s'\n", ld->URI);
				dlclose(lib);
				break;
			}

			if (!strcmp((const char*)abs_uri_node.buf,
			            lilv_node_as_uri(lilv_plugin_get_uri(plugin)))) {
				// Create LilvInstance to return
				result = malloc(sizeof(struct LilvInstanceImpl));
				result->lv2_descriptor = ld;
				result->lv2_handle = ld->instantiate(
					ld, sample_rate, (char*)bundle_path,
					(features) ? features : local_features);
				result->pimpl = lib;
				serd_node_free(&abs_uri_node);
				break;
			} else {
				serd_node_free(&abs_uri_node);
			}
		}
	}

	if (result) {
		assert(lilv_plugin_get_num_ports(plugin) > 0);

		// Failed to instantiate
		if (result->lv2_handle == NULL) {
			free(result);
			return NULL;
		}

		// "Connect" all ports to NULL (catches bugs)
		for (uint32_t i = 0; i < lilv_plugin_get_num_ports(plugin); ++i)
			result->lv2_descriptor->connect_port(result->lv2_handle, i, NULL);
	}

	free(local_features);

	return result;
}

LILV_API
void
lilv_instance_free(LilvInstance* instance)
{
	if (!instance)
		return;

	instance->lv2_descriptor->cleanup(instance->lv2_handle);
	instance->lv2_descriptor = NULL;
	dlclose(instance->pimpl);
	instance->pimpl = NULL;
	free(instance);
}

