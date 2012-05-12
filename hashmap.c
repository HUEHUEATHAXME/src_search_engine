#include "map.h"

#include <stdlib.h>

struct mapentry
{
    void *key;
    void *value;
    struct mapentry *next;
};

typedef struct mapentry mapentry_t;

struct map
{
    cmpfunc_t cmpfunc;
    hashfunc_t hashfunc;
    int size;
    mapentry_t **buckets;
    int numbuckets;
};

static mapentry_t *newentry(void *key, void *value, mapentry_t *next)
{
    mapentry_t *e = malloc(sizeof(mapentry_t));
    if (e == NULL) {
        fatal_error("out of memory");
		return NULL;
	}
    e->key = key;
    e->value = value;
    e->next = next;
    return e;
}

map_t *map_create(cmpfunc_t cmpfunc, hashfunc_t hashfunc)
{
    map_t *map = malloc(sizeof(map_t));

    if (map == NULL) {
        fatal_error("out of memory");
		return NULL;
	}
    map->cmpfunc = cmpfunc;
    map->hashfunc = hashfunc;
    map->size = 0;
    map->numbuckets = 8;
    map->buckets = calloc(map->numbuckets, sizeof(mapentry_t *));
    if (map->buckets == NULL) {
        fatal_error("out of memory");
		return NULL;
	}
    return map;
}

static void freebuckets(int numbuckets, mapentry_t **buckets)
{
    int b;
    mapentry_t *e, *tmp;

    for (b = 0; b < numbuckets; b++) {
        e = buckets[b];
        while (e != NULL) {
            tmp = e;
            e = e->next;
            free(tmp);
        }
    }
    free(buckets);
}

void map_destroy(map_t *map)
{
    freebuckets(map->numbuckets, map->buckets);
    free(map);
}

static void growmap(map_t *map)
{
    int b;
    int oldnumbuckets = map->numbuckets;
    mapentry_t **oldbuckets = map->buckets;

    map->size = 0;
    map->numbuckets = oldnumbuckets * 2;
    map->buckets = calloc(map->numbuckets, sizeof(mapentry_t *));
    if (map->buckets == NULL) {
        fatal_error("out of memory");
		return;
	}

    for (b = 0; b < oldnumbuckets; b++) {
        mapentry_t *e = oldbuckets[b];
        while (e != NULL) {
            map_put(map, e->key, e->value);
            e = e->next;
        }
    }
    freebuckets(oldnumbuckets, oldbuckets);
}

void map_put(map_t *map, void *key, void *value)
{
    unsigned long hash = map->hashfunc(key);
    int b = hash % map->numbuckets;
    mapentry_t *e = map->buckets[b];

    while (e != NULL && map->cmpfunc(key, e->key) != 0) {
        e = e->next;
    }
    if (e == NULL) {
        map->buckets[b] = newentry(key, value, map->buckets[b]);
        map->size++;
        if (map->size >= map->numbuckets)
            growmap(map);
    }
    else {
        e->value = value;
    }
}

int map_haskey(map_t *map, void *key)
{
    unsigned long hash = map->hashfunc(key);
    int b = hash % map->numbuckets;
    mapentry_t *e = map->buckets[b];

    while (e != NULL && map->cmpfunc(key, e->key) != 0) {
        e = e->next;
    }
    if (e == NULL) {
        return 0;
    }
    else {
        return 1;
    }
}

void *map_get(map_t *map, void *key)
{
    unsigned long hash = map->hashfunc(key);
    int b = hash % map->numbuckets;
    mapentry_t *e = map->buckets[b];

    while (e != NULL && map->cmpfunc(key, e->key) != 0) {
        e = e->next;
    }
    if (e == NULL) {
		return NULL;
    }
    else {
        return e->value;
    }
}
