#ifndef MAP_H
#define MAP_H

#include "common.h"

/*
 * The type of maps.
 */
struct map;
typedef struct map map_t;

/*
 * Creates a new, empty map whose keys are compared using the given
 * comparison function, and hashed using the given hash function.
 */
map_t *map_create(cmpfunc_t cmpfunc, hashfunc_t hashfunc);

/*
 * Destroys the given map.  Subsequently accessing the map will lead
 * to undefined behavior.
 */
void map_destroy(map_t *map);

/*
 * Maps the given key to the given value.  This will overwrite any
 * value that the key was previously mapped to.
 */
void map_put(map_t *map, void *key, void *value);

/*
 * Returns 1 if the given map contains the given key, 0 otherwise.
 */
int map_haskey(map_t *map, void *key);

/*
 * Returns the value that the given key maps to.
 */
void *map_get(map_t *map, void *key);

#endif
