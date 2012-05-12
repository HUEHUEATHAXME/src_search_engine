#include "common.h"
#include "list.h"
#include "map.h"
#include "query_parser.h"
#include "set.h"

#include <ctype.h>
#include <string.h>

//#define DEBUG
#ifdef DEBUG
  #define TOKEN_DEBUG 
#endif

enum token                  { ANDNOT ,  AND ,  OR ,  PARENTHESIS_LEFT,   PARENTHESIS_RIGHT,   WORD,   EOQ };
static char *token_text[] = {"ANDNOT", "AND", "OR", "PARENTHESIS_LEFT", "PARENTHESIS_RIGHT", "WORD", "EOQ"};

static enum token get_token(char **q)
{
#ifdef TOKEN_DEBUG
	printf("\"%s\" =", *q);
#endif
	while (isspace(**q)) {
		(*q)++;
	}
#ifdef TOKEN_DEBUG
	printf(" \"%s\" =>", *q);
#endif

	enum token t;
	switch(**q) {
		case '\0':
			t = EOQ;
			break;
		case '(':
			t = PARENTHESIS_LEFT;
			break;
		case ')':
			t = PARENTHESIS_RIGHT;
			break;
		case 'A':
			if (strncmp(token_text[AND], *q, 3) == 0) {
				char *post_AND = (*q)+3;
				if (strncmp(token_text[ANDNOT]+3, post_AND, 3) == 0) {
					t = ANDNOT;
				} else {
					t = AND;
				}
			} else {
				t = WORD;
			}
			break;
		case 'O':
			if (strncmp(token_text[OR], *q, 2) == 0) {
				t = OR;
			} else {
				t = WORD;
			}
			break;
		default:
			t = WORD;
			break;
	}

#ifdef TOKEN_DEBUG
	printf(" %s\n", token_text[t]); fflush(stdout);
#endif

	return t;
}


static set_t *_term(map_t *map, char **q, char **errmsg, int level) {
	// _term ::= "(" _query ")"
	//       | <word>

	set_t *result = NULL;

	enum token t = get_token(q);
	switch (t) {
		case PARENTHESIS_LEFT:
		{
			*q += 1; // skip left parenthesis
			result = _query(map, q, errmsg, level+1);
			if (get_token(q) == PARENTHESIS_RIGHT) {
				*q += 1; // skip right parenthesis
			} else {
				*errmsg = strdup("Missing right parenthesis");
				result = NULL;
			}
			break;
		}
		case WORD:
		{
			char *post_word = *q;
			while (!isspace(*post_word) && *post_word != '\0' && *post_word != ')') {
				post_word++;
			}
			char tmp = *post_word;
			*post_word = '\0';
			result = map_get(map, *q);
#ifdef DEBUG
			printf("\"%s\" => %i matches\n", *q, (result == NULL ? -1 : set_size(result))); fflush(stdout);
#endif
			*post_word = tmp;

			*q += post_word - *q; // skip the word

			if (result == NULL) {
				result = set_create(compare_strings); // query with no result is empty, not NULL
			} else {
				result = set_copy(result); // do not delete original data destroying result
			}
			break;
		}
		default:
			*errmsg = strdup("Expecting word or \"(\" query \")\"");
			break;
	}

	return result;
}


static set_t *_orterm(map_t *map, char **q, char **errmsg, int level) {
	// _orterm ::= _term
	//         | _term "OR" _orterm

	set_t *result, *left = _term(map, q, errmsg, level);

	switch (get_token(q)) {
		case OR:
		{
			*q += 2; // skip keyword
			set_t *right = _orterm(map, q, errmsg, level);
			if (right == NULL || left == NULL) {
				result = NULL;
			} else {
				result = set_union(left, right);
				set_destroy(left);
				set_destroy(right);
			}
			break;
		}
		case EOQ:
		case PARENTHESIS_RIGHT:
		case ANDNOT:
		case AND:
			result = left;
			break;
		default:
			*errmsg = strdup("Expecting term or term \"OR\" orterm");
			result = NULL;
	}

	return result;
}


static set_t *_andterm(map_t *map, char **q, char **errmsg, int level) {
	// _andterm ::= _orterm
	//          | _orterm "AND" _andterm

	set_t *result, *left = _orterm(map, q, errmsg, level);

	switch (get_token(q)) {
		case AND:
		{
			*q += 3; // skip keyword
			set_t *right = _andterm(map, q, errmsg, level);
			if (right == NULL || left == NULL) {
				result = NULL;
			} else {
				result = set_intersection(left, right);
				set_destroy(left);
				set_destroy(right);
			}
			break;
		}
		case EOQ:
		case PARENTHESIS_RIGHT:
		case ANDNOT:
			result = left;
			break;
		default:
			*errmsg = strdup("Expecting orterm or orterm \"AND\" andterm");
			result = NULL;
	}

	return result;
}


set_t *_query(map_t *map, char **q, char **errmsg, int level) {
	// _query ::= _andterm
	//        | _andterm "ANDNOT" query

	set_t *result, *left = _andterm(map, q, errmsg, level);

	switch (get_token(q)) {
		case ANDNOT:
		{
			*q += 6; // skip keyword
			set_t *right = _query(map, q, errmsg, level);
			if (right == NULL || left == NULL) {
				result = NULL;
			} else {
				result = set_difference(left, right);
				set_destroy(left);
				set_destroy(right);
			}
			break;
		}
			if (level > 0) {
				result = left;
			} else {
			}
		case PARENTHESIS_RIGHT:
			if (level == 0) {
				*errmsg = strdup("Missing left parenthesis");
				result = NULL;
			} else {
				result = left;
			}
			break;
		case EOQ:
			result = left;
			break;
		default:
			*errmsg = strdup("Expecting andterm or andterm \"ANDNOT\" query");
			result = NULL;
	}

	return result;
}
