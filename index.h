#ifndef INDEX_H
#define INDEX_H

#include "list.h"

struct index;
typedef struct index index_t;

/*
 * Creates a new, empty index.
 */
index_t *index_create();

/*
 * Destroys the given index.  Subsequently accessing the index will
 * lead to undefined behavior.
 */
void index_destroy(index_t *index);

/*
 * Adds the given path to the given index, and index the given
 * list of words under that path.
 */
void index_addpath(index_t *index, char *path, list_t *words);

/*
 * Performs the given query on the given index.  If the query
 * succeeds, the return value will be a list of paths.  If there
 * is an error (e.g. a syntax error in the query), an error message
 * is assigned to the given errmsg pointer and the return value
 * will be NULL.
 */
list_t *index_query(index_t *index, char *query, char **errmsg);

#endif


