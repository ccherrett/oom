/*
  Copyright 2011 David Robillard <http://drobilla.net>

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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "serd/serd.h"

#include "sord-config.h"
#include "sord_internal.h"

typedef struct {
	SerdEnv*   env;
	SordNode*  graph_uri_node;
	SordWorld* world;
	SordModel* sord;
} ReadState;

static SerdStatus
event_base(void*           handle,
           const SerdNode* uri_node)
{
	ReadState* const state = (ReadState*)handle;

	return serd_env_set_base_uri(state->env, uri_node);
}

static SerdStatus
event_prefix(void*           handle,
             const SerdNode* name,
             const SerdNode* uri_node)
{
	ReadState* const state = (ReadState*)handle;

	return serd_env_set_prefix(state->env, name, uri_node);
}

static SerdStatus
event_statement(void*              handle,
                SerdStatementFlags flags,
                const SerdNode*    graph,
                const SerdNode*    subject,
                const SerdNode*    predicate,
                const SerdNode*    object,
                const SerdNode*    object_datatype,
                const SerdNode*    object_lang)
{
	ReadState* const state = (ReadState*)handle;

	SordNode* s = sord_node_from_serd_node(state->world, state->env,
	                                       subject, NULL, NULL);
	SordNode* p = sord_node_from_serd_node(state->world, state->env,
	                                       predicate, NULL, NULL);
	SordNode* o = sord_node_from_serd_node(
		state->world, state->env, object, object_datatype, object_lang);

	SordNode* g = NULL;
	if (state->graph_uri_node) {
		assert(graph->type == SERD_NOTHING);
		g = sord_node_copy(state->graph_uri_node);
	} else {
		g = (graph && graph->buf)
			? sord_node_from_serd_node(state->world, state->env,
			                           graph, NULL, NULL)
			: NULL;
	}

	const SordQuad tup = { s, p, o, g };
	sord_add(state->sord, tup);

	sord_node_free(state->world, s);
	sord_node_free(state->world, p);
	sord_node_free(state->world, o);
	sord_node_free(state->world, g);

	return SERD_SUCCESS;
}

SORD_API
SerdReader*
sord_new_reader(SordModel* model,
                SerdEnv*   env,
                SerdSyntax syntax,
                SordNode*  graph)
{
	ReadState* state = malloc(sizeof(ReadState));
	state->env            = env;
	state->graph_uri_node = graph;
	state->world          = sord_get_world(model);
	state->sord           = model;

	SerdReader* reader = serd_reader_new(
		syntax, state, free,
		event_base, event_prefix, event_statement, NULL);

	return reader;
}

static void
write_statement(SordModel* sord, SerdWriter* writer, SordQuad tup,
                SerdStatementFlags flags)
{
	const SordNode* s  = tup[SORD_SUBJECT];
	const SordNode* p  = tup[SORD_PREDICATE];
	const SordNode* o  = tup[SORD_OBJECT];
	const SordNode* d  = sord_node_get_datatype(o);
	const SerdNode* ss = sord_node_to_serd_node(s);
	const SerdNode* sp = sord_node_to_serd_node(p);
	const SerdNode* so = sord_node_to_serd_node(o);
	const SerdNode* sd = sord_node_to_serd_node(d);

	const char* lang_str = sord_node_get_language(o);
	size_t      lang_len = lang_str ? strlen(lang_str) : 0;
	SerdNode    language = SERD_NODE_NULL;
	if (lang_str) {
		language.type    = SERD_LITERAL;
		language.n_bytes = lang_len;
		language.n_chars = lang_len;
		language.buf     = (const uint8_t*)lang_str;
	};

	// TODO: Subject abbreviation

	if (sord_node_is_inline_object(s) && !(flags & SERD_ANON_CONT)) {
		return;
	}

	SerdStatus st = SERD_SUCCESS;
	if (sord_node_is_inline_object(o)) {
		SordQuad  sub_pat  = { o, 0, 0, 0 };
		SordIter* sub_iter = sord_find(sord, sub_pat);

		SerdStatementFlags start_flags = flags
			| ((sub_iter) ? SERD_ANON_O_BEGIN : SERD_EMPTY_O);

		st = serd_writer_write_statement(
			writer, start_flags, NULL, ss, sp, so, sd, &language);

		if (!st && sub_iter) {
			flags |= SERD_ANON_CONT;
			for (; !sord_iter_end(sub_iter); sord_iter_next(sub_iter)) {
				SordQuad sub_tup;
				sord_iter_get(sub_iter, sub_tup);
				write_statement(sord, writer, sub_tup, flags);
			}
			sord_iter_free(sub_iter);
			serd_writer_end_anon(writer, so);
		}
	} else {
		st = serd_writer_write_statement(
			writer, flags, NULL, ss, sp, so, sd, &language);
	}

	if (st) {
		fprintf(stderr, "Failed to write statement (%s)\n",
		        serd_strerror(st));
		return;
	}
}

bool
sord_write(SordModel*  model,
           SerdWriter* writer,
           SordNode*   graph)
{
	SordQuad  pat  = { 0, 0, 0, graph };
	SordIter* iter = sord_find(model, pat);
	return sord_write_iter(iter, writer);
}

bool
sord_write_iter(SordIter*   iter,
                SerdWriter* writer)
{
	if (!iter) {
		return false;
	}

	SordModel* model = (SordModel*)sord_iter_get_model(iter);
	for (; !sord_iter_end(iter); sord_iter_next(iter)) {
		SordQuad tup;
		sord_iter_get(iter, tup);
		write_statement(model, writer, tup, 0);
	}
	sord_iter_free(iter);
	return true;
}
