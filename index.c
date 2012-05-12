#include "common.h"
#include "index.h"
#include "list.h"
#include "map.h"
#include "query_parser.h"
#include "set.h"

#include <stdlib.h>
#include <stdio.h>


struct index {
	map_t *words;
};


index_t *index_create() {
	index_t *idx = (index_t *)calloc(1,sizeof(*idx));
	if (idx == NULL) {
		goto error;
	}

	idx->words = map_create(compare_strings, hash_string);
	if (idx->words == NULL) {
		goto error;
	}

	return idx;
error:
	index_destroy(idx);
	return NULL;
}


void index_destroy(index_t *idx) {
	if (idx != NULL) {
		free(idx);
		if (idx->words != NULL) {
			map_destroy(idx->words);
		}
	}
}


void index_addpath(index_t *idx, char *path, list_t *words) {
	if (idx == NULL || path == NULL || words == NULL) {
		return;
	}

	while(list_size(words) > 0) {
		char *word = list_popfirst(words);

		set_t *files_with_word = map_get(idx->words, word);
		if (files_with_word == NULL) {
			files_with_word = set_create(compare_strings);
			if (files_with_word == NULL) {
				perror("index_addpath (malloc):");
				return;
			}
			map_put(idx->words, word, files_with_word);
		} else {
			free(word);
		}

		set_add(files_with_word, path);
	}
	return;
}


static list_t *list_from_set(set_t *set) {
	if (set == NULL) {
		return NULL;
	}

	list_t *list = list_create(compare_strings);
	if (list == NULL) {
		perror("list_from_set (could not create list):");
		return NULL;
	}

	set_iter_t *si = set_createiter(set);
	if (si == NULL) {
		perror("list_from_set (could not create set iterator):");
		return NULL;
	}

	while (set_hasnext(si)) {
		list_addlast(list, set_next(si));
	}

	return list;
}


list_t *index_query(index_t *idx, char *query, char **errmsg) {
	if (idx == NULL) {
		return NULL;
	}

	set_t *result = _query(idx->words, &query, errmsg, 0);
	if (result == NULL) {
		return NULL;
	}

	list_t *result_as_list = list_from_set(result);
	set_destroy(result);

	return result_as_list;
}
