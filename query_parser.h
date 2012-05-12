#ifndef QUERY_PARSER_H
#define QUERY_PARSER_H

#include "map.h"
#include "set.h"

/* parse a query using this BNF grammar
 *
 * query   ::= andterm
 *         | andterm "ANDNOT" query
 * andterm ::= orterm
 *         | orterm "AND" andterm
 * orterm  ::= term
 *         | term "OR" orterm
 * term    ::= "(" query ")"
 *         | <word>
 *
 * returns: NULL on error along with errmsg and set with files otherwise */
set_t *_query(map_t *map, char **q, char **errmsg, int level);

#endif /* QUERY_PARSER_H */
