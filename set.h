#ifndef SET_H
#define SET_H

#include "common.h"

/*
 * The type of sets.
 */
struct set;
typedef struct set set_t;

/*
 * Creates a new set using the given comparison function
 * to compare elements of the set.
 */
set_t *set_create(cmpfunc_t cmpfunc);

/*
 * Destroys the given set.  Subsequently accessing the set
 * will lead to undefined behavior.
 */
void set_destroy(set_t *set);

/*
 * Returns the size (cardinality) of the given set.
 */
int set_size(set_t *set);

/*
 * Adds the given element to the given set.
 */
void set_add(set_t *set, void *elem);

/*
 * Returns 1 if the given element is contained in
 * the given set, 0 otherwise.
 */
int set_contains(set_t *set, void *elem);

/*
 * Returns the union of the two given sets; the returned
 * set contains all elements that are contained in either
 * a or b.
 */
set_t *set_union(set_t *a, set_t *b);

/*
 * Returns the intersection of the two given sets; the
 * returned set contains all elements that are contained
 * in both a and b.
 */
set_t *set_intersection(set_t *a, set_t *b);

/*
 * Returns the set difference of the two given sets; the
 * returned set contains all elements that are contained
 * in a and not in b.
 */
set_t *set_difference(set_t *a, set_t *b);

/*
 * Returns a copy of the given set.
 */
set_t *set_copy(set_t *set);

/*
 * The type of set iterators.
 */
struct set_iter;
typedef struct set_iter set_iter_t;

/*
 * Creates a new set iterator for iterating over the given set.
 */
set_iter_t *set_createiter(set_t *set);

/*
 * Destroys the given set iterator.
 */
void set_destroyiter(set_iter_t *iter);

/*
 * Returns 0 if the given set iterator has reached the end of the
 * set, or 1 otherwise.
 */
int set_hasnext(set_iter_t *iter);

/*
 * Returns the next element in the sequence represented by the given
 * set iterator.
 */
void *set_next(set_iter_t *iter);

#endif
