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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sord/sord.h"

static const int DIGITS  = 3;
static const int MAX_NUM = 999;

typedef struct {
	SordQuad query;
	int      expected_num_results;
} QueryTest;

#define USTR(s) ((const uint8_t*)(s))

static SordNode*
uri(SordWorld* world, int num)
{
	if (num == 0)
		return 0;

	char  uri[]   = "eg:000";
	char* uri_num = uri + 3;  // First `0'
	snprintf(uri_num, DIGITS + 1, "%0*d", DIGITS, num);
	return sord_new_uri(world, (const uint8_t*)uri);
}

int
test_fail(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	va_end(args);
	return 1;
}

int
generate(SordWorld* world, SordModel* sord, size_t n_quads, size_t n_objects_per)
{
	fprintf(stderr, "Generating %zu (S P *) quads with %zu objects each\n",
	        n_quads, n_objects_per);

	for (size_t i = 0; i < n_quads; ++i) {
		int num = (i * n_objects_per) + 1;

		SordNode* ids[2 + n_objects_per];
		for (size_t j = 0; j < 2 + n_objects_per; ++j) {
			ids[j] = uri(world, num++);
		}

		for (size_t j = 0; j < n_objects_per; ++j) {
			SordQuad tup = { ids[0], ids[1], ids[2 + j] };
			if (!sord_add(sord, tup)) {
				return test_fail("Fail: Failed to add quad\n");
			}
		}

		for (size_t j = 0; j < 2 + n_objects_per; ++j) {
			sord_node_free(world, ids[j]);
		}
	}

	// Add some literals
	SordQuad tup = { 0, 0, 0, 0};
	tup[0] = uri(world, 98);
	tup[1] = uri(world, 4);
	tup[2] = sord_new_literal(world, 0, USTR("hello"), NULL);
	tup[3] = 0;
	sord_add(sord, tup);
	sord_node_free(world, (SordNode*)tup[2]);
	tup[2] = sord_new_literal(world, 0, USTR("hi"), NULL);
	sord_add(sord, tup);

	sord_node_free(world, (SordNode*)tup[0]);
	sord_node_free(world, (SordNode*)tup[2]);
	tup[0] = uri(world, 14);
	tup[2] = sord_new_literal(world, 0, USTR("bonjour"), "fr");
	sord_add(sord, tup);
	sord_node_free(world, (SordNode*)tup[2]);
	tup[2] = sord_new_literal(world, 0, USTR("salut"), "fr");
	sord_add(sord, tup);

	// Attempt to add some duplicates
	if (sord_add(sord, tup)) {
		return test_fail("Fail: Successfully added duplicate quad\n");
	}
	if (sord_add(sord, tup)) {
		return test_fail("Fail: Successfully added duplicate quad\n");
	}

	// Add a blank node subject
	sord_node_free(world, (SordNode*)tup[0]);
	tup[0] = sord_new_blank(world, USTR("ablank"));
	sord_add(sord, tup);

	sord_node_free(world, (SordNode*)tup[1]);
	sord_node_free(world, (SordNode*)tup[2]);
	tup[1] = uri(world, 6);
	tup[2] = uri(world, 7);
	sord_add(sord, tup);
	sord_node_free(world, (SordNode*)tup[0]);
	sord_node_free(world, (SordNode*)tup[1]);
	sord_node_free(world, (SordNode*)tup[2]);

	return EXIT_SUCCESS;
}

#define TUP_FMT "(%6s %6s %6s)"
#define TUP_FMT_ARGS(t)                                                 \
	((t)[0] ? sord_node_get_string((t)[0]) : USTR("*")), \
		((t)[1] ? sord_node_get_string((t)[1]) : USTR("*")), \
		((t)[2] ? sord_node_get_string((t)[2]) : USTR("*"))

int
test_read(SordWorld* world, SordModel* sord,
          const size_t n_quads, const int n_objects_per)
{
	int ret = EXIT_SUCCESS;

	SordQuad id;

	SordIter* iter = sord_begin(sord);
	if (sord_iter_get_model(iter) != sord) {
		return test_fail("Fail: Iterator has incorrect sord pointer\n");
	}

	for (; !sord_iter_end(iter); sord_iter_next(iter))
		sord_iter_get(iter, id);

	// Attempt to increment past end
	if (!sord_iter_next(iter)) {
		return test_fail("Fail: Successfully incremented past end\n");
	}

	sord_iter_free(iter);

#define NUM_PATTERNS 9

	QueryTest patterns[NUM_PATTERNS] = {
		{ { 0, 0, 0 }, (n_quads * n_objects_per) + 6 },
		{ { uri(world, 9), uri(world, 9), uri(world, 9) }, 0 },
		{ { uri(world, 1), uri(world, 2), uri(world, 4) }, 1 },
		{ { uri(world, 3), uri(world, 4), uri(world, 0) }, 2 },
		{ { uri(world, 0), uri(world, 2), uri(world, 4) }, 1 },
		{ { uri(world, 0), uri(world, 0), uri(world, 4) }, 1 },
		{ { uri(world, 1), uri(world, 0), uri(world, 0) }, 2 },
		{ { uri(world, 1), uri(world, 0), uri(world, 4) }, 1 },
		{ { uri(world, 0), uri(world, 2), uri(world, 0) }, 2 } };

	SordQuad match = { uri(world, 1), uri(world, 2), uri(world, 4) };
	if (!sord_contains(sord, match)) {
		return test_fail("Fail: No match for " TUP_FMT "\n",
		                 TUP_FMT_ARGS(match));
	}

	SordQuad nomatch = { uri(world, 1), uri(world, 2), uri(world, 9) };
	if (sord_contains(sord, nomatch)) {
		return test_fail("Fail: False match for " TUP_FMT "\n",
		                 TUP_FMT_ARGS(nomatch));
	}

	for (unsigned i = 0; i < NUM_PATTERNS; ++i) {
		QueryTest test = patterns[i];
		SordQuad  pat = { test.query[0], test.query[1], test.query[2], 0 };
		fprintf(stderr, "Query " TUP_FMT "... ", TUP_FMT_ARGS(pat));

		iter = sord_find(sord, pat);
		int num_results = 0;
		for (; !sord_iter_end(iter); sord_iter_next(iter)) {
			sord_iter_get(iter, id);
			++num_results;
			if (!sord_quad_match(pat, id)) {
				sord_iter_free(iter);
				return test_fail("Fail: Query result " TUP_FMT " does not match pattern\n",
				                 TUP_FMT_ARGS(id));
			}
		}
		sord_iter_free(iter);
		if (num_results != test.expected_num_results) {
			return test_fail("Fail: Expected %d results, got %d\n",
			                 test.expected_num_results, num_results);
		}
		fprintf(stderr, "OK (%u matches)\n", test.expected_num_results);
	}

	// Query blank node subject
	SordQuad pat = { sord_new_blank(world, USTR("ablank")), 0, 0 };
	if (!pat[0]) {
		return test_fail("Blank node subject lost\n");
	}
	fprintf(stderr, "Query " TUP_FMT "... ", TUP_FMT_ARGS(pat));
	iter = sord_find(sord, pat);
	int num_results = 0;
	for (; !sord_iter_end(iter); sord_iter_next(iter)) {
		sord_iter_get(iter, id);
		++num_results;
		if (!sord_quad_match(pat, id)) {
			sord_iter_free(iter);
			return test_fail("Fail: Query result " TUP_FMT " does not match pattern\n",
			                 TUP_FMT_ARGS(id));
		}
	}
	fprintf(stderr, "OK\n");
	sord_node_free(world, (SordNode*)pat[0]);
	sord_iter_free(iter);
	if (num_results != 2) {
		return test_fail("Blank node subject query failed\n");
	}

	// Test nested queries
	fprintf(stderr, "Nested Queries... ");
	pat[0] = pat[1] = pat[2] = 0;
	const SordNode* last_subject = 0;
	iter = sord_find(sord, pat);
	for (; !sord_iter_end(iter); sord_iter_next(iter)) {
		sord_iter_get(iter, id);
		if (id[0] == last_subject)
			continue;

		SordQuad subpat  = { id[0], 0, 0 };
		SordIter* subiter = sord_find(sord, subpat);
		int num_sub_results = 0;
		for (; !sord_iter_end(subiter); sord_iter_next(subiter)) {
			SordQuad subid;
			sord_iter_get(subiter, subid);
			if (!sord_quad_match(subpat, subid)) {
				sord_iter_free(iter);
				sord_iter_free(subiter);
				return test_fail("Fail: Nested query result does not match pattern\n");
			}
			++num_sub_results;
		}
		sord_iter_free(subiter);
		if (num_sub_results != n_objects_per) {
			return test_fail("Fail: Nested query failed (got %d results, expected %d)\n",
			                 num_sub_results, n_objects_per);
		}
		last_subject = id[0];
	}
	fprintf(stderr, "OK\n\n");
	sord_iter_free(iter);

	return ret;
}

int
main(int argc, char** argv)
{
	static const size_t n_quads       = 300;
	static const int    n_objects_per = 2;

	sord_free(NULL);  // Shouldn't crash

	SordWorld* world = sord_world_new();

	// Create with minimal indexing
	SordModel* sord = sord_new(world, SORD_SPO, false);
	generate(world, sord, n_quads, n_objects_per);

	if (test_read(world, sord, n_quads, n_objects_per)) {
		sord_free(sord);
		sord_world_free(world);
		return EXIT_FAILURE;
	}

	// Check interning merges equivalent values
	SordNode* uri_id   = sord_new_uri(world, USTR("http://example.org"));
	SordNode* blank_id = sord_new_uri(world, USTR("testblank"));
	SordNode* lit_id   = sord_new_literal(world, uri_id, USTR("hello"), NULL);
	//sord_clear_cache(write);
	SordNode* uri_id2   = sord_new_uri(world, USTR("http://example.org"));
	SordNode* blank_id2 = sord_new_uri(world, USTR("testblank"));
	SordNode* lit_id2   = sord_new_literal(world, uri_id, USTR("hello"), NULL);
	if (uri_id2 != uri_id) {
		fprintf(stderr, "Fail: URI interning failed (duplicates)\n");
		goto fail;
	} else if (blank_id2 != blank_id) {
		fprintf(stderr, "Fail: Blank node interning failed (duplicates)\n");
		goto fail;
	} else if (lit_id2 != lit_id) {
		fprintf(stderr, "Fail: Literal interning failed (duplicates)\n");
		goto fail;
	}

	// Check interning doesn't clash non-equivalent values
	SordNode* uri_id3   = sord_new_uri(world, USTR("http://example.orgX"));
	SordNode* blank_id3 = sord_new_uri(world, USTR("testblankX"));
	SordNode* lit_id3   = sord_new_literal(world, uri_id, USTR("helloX"), NULL);
	if (uri_id3 == uri_id) {
		fprintf(stderr, "Fail: URI interning failed (clash)\n");
		goto fail;
	} else if (blank_id3 == blank_id) {
		fprintf(stderr, "Fail: Blank node interning failed (clash)\n");
		goto fail;
	} else if (lit_id3 == lit_id) {
		fprintf(stderr, "Fail: Literal interning failed (clash)\n");
		goto fail;
	}

	sord_node_free(world, uri_id);
	sord_node_free(world, blank_id);
	sord_node_free(world, lit_id);
	sord_node_free(world, uri_id2);
	sord_node_free(world, blank_id2);
	sord_node_free(world, lit_id2);
	sord_node_free(world, uri_id3);
	sord_node_free(world, blank_id3);
	sord_node_free(world, lit_id3);
	sord_free(sord);

	static const char* const index_names[6] = {
		"spo", "sop", "ops", "osp", "pso", "pos"
	};

	for (int i = 0; i < 6; ++i) {
		sord = sord_new(world, (1 << i), false);
		printf("Testing Index `%s'\n", index_names[i]);
		generate(world, sord, n_quads, n_objects_per);
		if (test_read(world, sord, n_quads, n_objects_per))
			goto fail;
		sord_free(sord);
	}

	// Test removing
	sord = sord_new(world, SORD_SPO, false);
	SordQuad tup = { 0, 0, 0, 0};
	tup[0] = uri(world, 1);
	tup[1] = uri(world, 2);
	tup[2] = sord_new_literal(world, 0, USTR("hello"), NULL);
	tup[3] = 0;
	sord_add(sord, tup);
	sord_node_free(world, (SordNode*)tup[2]);
	tup[2] = sord_new_literal(world, 0, USTR("hi"), NULL);
	sord_add(sord, tup);
	sord_remove(sord, tup);
	if (sord_num_quads(sord) != 1) {
		fprintf(stderr, "Removed failed (%zu quads, expected 1)\n",
		        sord_num_quads(sord));
		goto fail;
	}

	sord_free(sord);

	sord_world_free(world);

	return EXIT_SUCCESS;

fail:
	sord_free(sord);
	sord_world_free(world);
	return EXIT_FAILURE;
}
