#include "common.h"
#include "index.h"
#include "list.h"
#include "unittest.h"

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


struct query {
	char *q;
	char *ans[5];
};


list_t *list_from_array(char **array, int count)
{
	if (array == NULL || count <= 0) {
		return NULL;
	}

	list_t *list = list_create(compare_strings);
	if (list == NULL) {
		return NULL;
	}

	int i;
	for (i = 0; i < count; i++) {
		list_addlast(list, strdup(array[i]));
	}

	UNITTEST(list_size(list) == count);

	return list;
}


void check_query(index_t *idx, struct query *q)
{
	char *errmsg, *query = q->q, **ans=q->ans;

	list_t *res = index_query(idx, strdup(query), &errmsg);
	if (res == NULL) {
		if (ans[0] != NULL) {
			fprintf(stderr, "query: \"%s\" should have been %s, but failed with error \"%s\"\n", query, (*ans[0] == '\0' ? "empty" : "non empty"), errmsg);
		}
		return;
	}

	if (ans[0] == NULL) {
		if (res != NULL) {
			fprintf(stderr, "query: \"%s\" should have failed, but is %s\n", query, (list_size(res) > 0 ? "non empty" : "empty"));
		}
		return;
	}

	int i;
	for(i = 0; *ans[i] != '\0'; i++) {
		if (!list_contains(res, ans[i])) {
			fprintf(stderr, "query: \"%s\" missing \"%s\"\n", query, ans[i]);
		}
	}

	while(list_size(res) > 0) {
		char *item = list_popfirst(res);
		int in_res = 0;
		for (i = 0; *ans[i] != '\0'; i++) {
			if (strcmp(item, ans[i]) == 0) {
				in_res = 1;
			}
		}

		if (in_res == 0) {
			fprintf(stderr, "query: \"%s\" includes \"%s\" which is not in answer\n", query, item);
		}
	}
}


void index_test(void)
{
	enum {
		NALPHA = 26,
		NDEC = 10,
		NALNUM = NDEC + NALPHA,
		NHEX = NDEC + 6,
	};

	void *something = (void *)ULONG_MAX;
	UNITTEST(index_query(NULL, something, something) == NULL);

	index_t *idx = index_create();
	if (idx == NULL) {
		fprintf(stderr, "could not create index, ABORTING\n");
		return;
	}

	char *alpha[NALPHA] = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
							"k", "l", "m", "n", "o", "p", "q", "r", "s", "t",
							"u", "v", "w", "x", "y", "z"};
	char *dec[NDEC] =     { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
	char *alnum[NALNUM];
	char *hex[NHEX];

	int i;
	for (i = 0; i < NDEC; i++) {
		alnum[i] = dec[i];
		hex[i] = dec[i];
	}

	for (i=0 ; i<6; i++) {
		alnum[NDEC+i] = alpha[i]; 
		hex[NDEC+i] = alpha[i];
	}

	for ( ; i<NALPHA; i++) {
		alnum[NDEC+i] = alpha[i];
	}

	list_t *alpha_list = list_from_array(alpha, NALPHA);
	list_t *dec_list = list_from_array(dec, NDEC);
	list_t *alnum_list = list_from_array(alnum, NALNUM);
	list_t *hex_list = list_from_array(hex, NHEX);
	if (alpha_list == NULL || dec_list == NULL || alnum_list == NULL || hex_list == NULL) {
		fprintf(stderr, "failed to create some of the input lists, ABORTING\n");
		return;
	}

	index_addpath(idx, strdup("alpha"), alpha_list);
	index_addpath(idx, strdup("dec"), dec_list);
	index_addpath(idx, strdup("alnum"), alnum_list);
	index_addpath(idx, strdup("hex"), hex_list);

	struct query queries[100] = {	
									{"a", {"alpha", "alnum", "hex", ""}}, {"z", {"alpha","alnum", ""}}, {"0", {"dec", "alnum", "hex", ""}}, {"something", {""}}, // single word queries

									// single operator queries
									{"a OR 1", {"alpha", "alnum", "dec", "hex", ""}}, {"z OR a", {"alpha", "alnum", "hex", ""}}, {"a OR something", {"alpha", "alnum", "hex", ""}}, {"something OR a", {"alpha", "alnum", "hex", ""}},
									{"a AND 1", {"alnum", "hex", ""}}, {"a AND z", {"alpha", "alnum", ""}}, {"a AND something", {""}}, {"something AND a", {""}},
									{"a ANDNOT 1", {"alpha", ""}}, {"a ANDNOT z", {"hex", ""}}, {"a ANDNOT something", {"alpha", "alnum", "hex", ""}}, {"something AND a", {""}},

									// multi operator queries and check precedence (the queries on the same line should give the same result)
									{"a ANDNOT (1 AND (3 OR a))", {"alpha", ""}}, {"a ANDNOT 1 AND 3 OR a", {"alpha", ""}}, 
									{"((a OR 3) AND 4) ANDNOT 2", {""}}, {"a OR 3 AND 4 ANDNOT 2", {""}},

									{"    a    ", {"alpha", "alnum", "hex", ""}}, {"    a    ANDNOT    1    ", {"alpha", ""}}, // whitspace trimming
									{"((((((((((a))))))))))", {"alpha", "alnum", "hex", ""}}, {"((((((((((a AND 1))))))))))", {"alnum", "hex", ""}}, // multilevel parenthesis

									// errors
									{"(",{NULL}}, {"(a",{NULL}}, {"a)", {NULL}}, {")", {NULL}}, // unmatched parenthesis
									{"", {NULL}} ,{" ", {NULL}}, {"     ", {NULL}}, {"()", {NULL}}, // empty queries
									{"a a", {NULL}}, {"a (a)", {NULL}}, {"a OR 1 a", {NULL}}, {"(a) a", {NULL}}, {"a a OR 1", {NULL}}, // missing operator
									{"a OR", {NULL}}, {"a AND", {NULL}}, {"a ANDNOT", {NULL}}, // missing right parameter
									{"OR a", {NULL}}, {"AND a", {NULL}}, {"ANDNOT a", {NULL}}, // missing left parameter

									{NULL, {NULL}}
								};


	for (i = 0; queries[i].q != NULL; i++) {
		check_query(idx, &queries[i]);
	}

}

int main(int argc, char **argv)
{
	index_test();

	return 0;
}
