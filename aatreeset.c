#include "set.h"
#include "list.h"

#include <assert.h>
#include <stdlib.h>

#define DEBUG_CHECKSET 0

struct treenode;
typedef struct treenode treenode_t;

/*
 * AA tree node.
 */
struct treenode {
    treenode_t *left;
    treenode_t *right;
    treenode_t *next;
    unsigned int level;
    void *elem;
};

#define nullNode &theNullNode
static treenode_t theNullNode = { nullNode, nullNode, nullNode, 0, NULL };

struct set {
    treenode_t *root;   /* Root of the AA tree */
    treenode_t *first;  /* Head of the linked list */
    int size;
    cmpfunc_t cmpfunc;
};

struct set_iter {
    treenode_t *node;
};

#if DEBUG_CHECKSET
/*
 * Returns the maximum of two integers.
 */
static int max(int a, int b)
{
	if (a > b)
		return a;
	else
		return b;
}

/*
 * Returns the maximum depth of a tree.
 */
static int maxdepth(treenode_t *n)
{
	if (n == nullNode) {
		return 0;
	}
	else {
		return 1 + max(maxdepth(n->left), maxdepth(n->right));
	}
}
#endif /* DEBUG_CHECKSET */

/*
 * Asserts that a node is valid.
 */
static void checknode(treenode_t *n, cmpfunc_t cmp)
{
	/* Tree ordering properties */
	assert(n->left == nullNode || cmp(n->left->elem, n->elem) < 0);
	assert(n->right == nullNode || cmp(n->right->elem, n->elem) > 0);
	assert(n->next == nullNode || cmp(n->next->elem, n->elem) > 0);
	/* Level properties that ensure a balanced tree */
	assert(n->level - n->left->level == 1);
	assert(n->level - n->right->level <= 1);
	assert(n->level - n->right->right->level >= 1);
}

/*
 * Asserts that a tree is valid, and returns the size of the tree.
 */
static int checktree(treenode_t *n, cmpfunc_t cmp)
{
	if (n == nullNode) {
		return 0;
	}
	else {
		checknode(n, cmp);
		return 1 + checktree(n->left, cmp) + checktree(n->right, cmp);
	}
}

/*
 * Checks that a set is valid.
 */
static void checkset(set_t *set)
{
	int size = checktree(set->root, set->cmpfunc);
	assert(size == set->size);
}

static treenode_t *newnode(void *elem)
{
    treenode_t *node = malloc(sizeof(treenode_t));
    if (node == NULL)
	    fatal_error("out of memory");
    node->left = nullNode;
    node->right = nullNode;
    node->next = nullNode;
    node->level = 1;
    node->elem = elem;
    return node;
}

static treenode_t *addnode(set_t *set, treenode_t *prev, void *elem)
{
    treenode_t *node = newnode(elem);
    if (prev == nullNode) {
	    node->next = set->first;
	    set->first = node;
    }
    else {
	    node->next = prev->next;
	    prev->next = node;
    }
    set->size++;
    return node;
}

set_t *set_create(cmpfunc_t cmpfunc)
{
    set_t *set = malloc(sizeof(set_t));
    if (set == NULL)
	    fatal_error("out of memory");
    set->root = nullNode;
    set->first = nullNode;
    set->size = 0;
    set->cmpfunc = cmpfunc;
    return set;
}

void set_destroy(set_t *set)
{
    treenode_t *n = set->first;
    while (n != nullNode) {
	    treenode_t *tmp = n;
	    n = n->next;
	    free(tmp);
    }
    free(set);
}

int set_size(set_t *set)
{
    return set->size;
}

static treenode_t *skew(treenode_t *root)
{
	if (root->left->level == root->level) {
		treenode_t *newroot = root->left;
		root->left = newroot->right;
		newroot->right = root;
		return newroot;
	}
	return root;
}

static treenode_t *split(treenode_t *root)
{
	if (root->right->right->level == root->level) {
		treenode_t *newroot = root->right;
		root->right = newroot->left;
		newroot->left = root;
		newroot->level++;
		return newroot;
	}
	return root;
}

static treenode_t *insert(set_t *set, treenode_t *root,
						  treenode_t *prev, void *elem)
{
	if (root == nullNode) {
		return addnode(set, prev, elem);
	}
	else {
		int cmp = set->cmpfunc(elem, root->elem);
		if (cmp < 0) {
			root->left = insert(set, root->left, prev, elem);
		}
		else if (cmp > 0) {
			root->right = insert(set, root->right, root, elem);
		}
		else {
			/* Already contained */
			return root;
		}
		/* Rebalance the tree */
		root = skew(root);
		root = split(root);
		return root;
	}
}

void set_add(set_t *set, void *elem)
{
	set->root = insert(set, set->root, nullNode, elem);
    if (DEBUG_CHECKSET)
	   checkset(set);
}

int set_contains(set_t *set, void *elem)
{
    treenode_t *n = set->root;

    while (n != nullNode) {
	    int cmp = set->cmpfunc(elem, n->elem);
	    if (cmp < 0) {
	        n = n->left;
	    }
	    else if (cmp > 0) {
	        n = n->right;
	    }
	    else {
	        /* Found it */
	        return 1;
	    }
    }
    /* No dice */
    return 0;
}

/*
 * Builds a balanced tree from the N first elements of the
 * given sorted list.  Assigns the first, root and last node
 * pointers.
 */
static void buildtree(list_t *list, int N,
                      treenode_t **first, treenode_t **root, treenode_t **last)
{
    if (N == 1) {
        *first = *root = *last = newnode(list_popfirst(list));
    }
    else if (N == 2) {
    	*first = *root = newnode(list_popfirst(list));
    	*last = (*root)->right = (*root)->next = newnode(list_popfirst(list));
    }
    else if (N > 2) {
        treenode_t *left;       /* root of left subtree */
        treenode_t *leftlast;   /* last node in left subtree */
        treenode_t *right;      /* root of right subtree */
        treenode_t *rightfirst; /* first node in right subtree */

        buildtree(list, N - N/2 - 1, first, &left, &leftlast);
        *root = *last = newnode(list_popfirst(list));
        (*root)->left = left;
        (*root)->level = left->level + 1;
        leftlast->next = *root;
		buildtree(list, N/2, &rightfirst, &right, last);
        (*root)->right = right;
        (*root)->next = rightfirst;
    }
}

/*
 * Builds a new set with a balanced tree, given a sorted list.
 * Destroys the list before returning the new set.
 */
static set_t *buildset(list_t *list, cmpfunc_t cmpfunc)
{
    set_t *set = set_create(cmpfunc);
    int size = list_size(list);

    if (size > 0) {
        treenode_t *last;
        buildtree(list, size, &(set->first), &(set->root), &last);
        set->size = size;
    }
    list_destroy(list);

	if (DEBUG_CHECKSET)
        checkset(set);
    return set;
}

set_t *set_union(set_t *a, set_t *b)
{
    if (a->cmpfunc != b->cmpfunc) {
	    fatal_error("union of incompatible sets");
		return NULL;
    }

	/* Merge the two sets into a sorted list */
	list_t *result = list_create(a->cmpfunc);
	treenode_t *na = a->first;
	treenode_t *nb = b->first;

	while (na != nullNode && nb != nullNode) {
		int cmp = a->cmpfunc(na->elem, nb->elem);
		if (cmp < 0) {
			/* Occurs in a only */
			list_addlast(result, na->elem);
			na = na->next;
		}
		else if (cmp > 0) {
			/* Occurs in b only */
			list_addlast(result, nb->elem);
			nb = nb->next;
		}
		else {
			/* Occurs in both a and b */
			list_addlast(result, na->elem);
			na = na->next;
			nb = nb->next;
		}
	}
	/* Plus what's left of the remaining set (either a or b) */
	for (; na != nullNode; na = na->next) {
		list_addlast(result, na->elem);
	}
	for (; nb != nullNode; nb = nb->next) {
		list_addlast(result, nb->elem);
	}
	/* Convert the sorted list into a balanced tree */
	return buildset(result, a->cmpfunc);
}

set_t *set_intersection(set_t *a, set_t *b)
{
    if (a->cmpfunc != b->cmpfunc) {
	    fatal_error("intersection of incompatible sets");
		return NULL;
    }

	/* Merge the two sets into a sorted list,
	   keeping common elements only */
	list_t *result = list_create(a->cmpfunc);
	treenode_t *na = a->first;
	treenode_t *nb = b->first;

	while (na != nullNode && nb != nullNode) {
		int cmp = a->cmpfunc(na->elem, nb->elem);
		if (cmp < 0) {
			/* Occurs in a only */
			na = na->next;
		}
		else if (cmp > 0) {
			/* Occurs in b only */
			nb = nb->next;
		}
		else {
			/* Occurs in both a and b, keep this one */
			list_addlast(result, na->elem);
			na = na->next;
			nb = nb->next;
		}
	}
	/* Convert the sorted list into a balanced tree */
	return buildset(result, a->cmpfunc);
}

set_t *set_difference(set_t *a, set_t *b)
{
    if (a->cmpfunc != b->cmpfunc) {
	    fatal_error("difference between incompatible sets");
		return NULL;
    }

	/* Merge the two sets into a sorted list,
	   keeping only elements that occur in a and not b */
	list_t *result = list_create(a->cmpfunc);
	treenode_t *na = a->first;
	treenode_t *nb = b->first;

	while (na != nullNode && nb != nullNode) {
		int cmp = a->cmpfunc(na->elem, nb->elem);
		if (cmp < 0) {
			/* Occurs in a only, keep this one */
			list_addlast(result, na->elem);
			na = na->next;
		}
		else if (cmp > 0) {
			/* Occurs in b only */
			nb = nb->next;
		}
		else {
			/* Occurs in both a and b */
			na = na->next;
			nb = nb->next;
		}
	}
	/* Plus what's left of a */
	for (; na != nullNode; na = na->next) {
		list_addlast(result, na->elem);
	}
	/* Convert the sorted list into a balanced tree */
	return buildset(result, a->cmpfunc);
}

set_t *set_copy(set_t *set)
{
    /* Insert all our elements into a list in sorted order */
    list_t *list = list_create(set->cmpfunc);
    treenode_t *n;

    for (n = set->first; n != nullNode; n = n->next) {
	    list_addlast(list, n->elem);
    }
    /* Convert the sorted list into a balanced tree */
    return buildset(list, set->cmpfunc);
}

set_iter_t *set_createiter(set_t *set)
{
    set_iter_t *iter = malloc(sizeof(set_iter_t));
    if (iter == NULL)
	    fatal_error("out of memory");
    iter->node = set->first;
    return iter;
}

void set_destroyiter(set_iter_t *iter)
{
    free(iter);
}

int set_hasnext(set_iter_t *iter)
{
    if (iter->node == nullNode)
	    return 0;
    else
	    return 1;
}

void *set_next(set_iter_t *iter)
{
    if (iter->node == nullNode) {
	    fatal_error("set iterator exhausted");
		return NULL;
    }

	void *elem = iter->node->elem;
	iter->node = iter->node->next;
	return elem;
}
