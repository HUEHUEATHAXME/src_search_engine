#include "list.h"

#include <stdlib.h>

struct listnode;

typedef struct listnode listnode_t;

struct listnode {
    listnode_t *next;
    listnode_t *prev;
    void *elem;
};

struct list {
    listnode_t *head;
    listnode_t *tail;
    int size;
    cmpfunc_t cmpfunc;
};

struct list_iter {
    listnode_t *node;
};

static listnode_t *newnode(void *elem)
{
    listnode_t *node = malloc(sizeof(listnode_t));
    if (node == NULL) {
	    fatal_error("out of memory");
		return NULL;
	}
    node->next = NULL;
    node->prev = NULL;
    node->elem = elem;
    return node;
}

list_t *list_create(cmpfunc_t cmpfunc)
{
    list_t *list = malloc(sizeof(list_t));
    if (list == NULL) {
	    fatal_error("out of memory");
		return NULL;
	}
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    list->cmpfunc = cmpfunc;
    return list;
}

void list_destroy(list_t *list)
{
    listnode_t *node = list->head;
    while (node != NULL) {
	    listnode_t *tmp = node;
	    node = node->next;
	    free(tmp);
    }
    free(list);
}

int list_size(list_t *list)
{
    return list->size;
}

void list_addfirst(list_t *list, void *elem)
{
    listnode_t *node = newnode(elem);
    if (list->head == NULL) {
	    list->head = list->tail = node;
    }
    else {
	    list->head->prev = node;
	    node->next = list->head;
	    list->head = node;
    }
    list->size++;
}

void list_addlast(list_t *list, void *elem)
{
    listnode_t *node = newnode(elem);
    if (list->head == NULL) {
	    list->head = list->tail = node;
    }
    else {
	    list->tail->next = node;
	    node->prev = list->tail;
	    list->tail = node;
    }
    list->size++;
}

void *list_popfirst(list_t *list)
{
    if (list->head == NULL) {
	    fatal_error("list_popfirst on empty list");
		return NULL;
    }
    else {
        void *elem = list->head->elem;
	    listnode_t *tmp = list->head;
	    list->head = list->head->next;
	    if (list->head == NULL) {
	        list->tail = NULL;
	    }
	    else {
	        list->head->prev = NULL;
	    }
	    list->size--;
	    free(tmp);
	    return elem;
    }
}

void *list_poplast(list_t *list)
{
    if (list->tail == NULL) {
        fatal_error("list_poplast on empty list");
		return NULL;
    }

	void *elem = list->tail->elem;
	listnode_t *tmp = list->tail;
	list->tail = list->tail->prev;
	if (list->tail == NULL) {
		list->head = NULL;
	} else {
		list->tail->next = NULL;
	}
	free(tmp);
	list->size--;
	return elem;
}

int list_contains(list_t *list, void *elem)
{
    listnode_t *node = list->head;
    while (node != NULL) {
	    if (list->cmpfunc(elem, node->elem) == 0)
	        return 1;
	    node = node->next;
    }
    return 0;
}

/*
 * Merges two sorted lists a and b using the given comparison function.
 * Only assigns the next pointers; the prev pointers will have to be
 * fixed by the caller.  Returns the head of the merged list.
 */
static listnode_t *merge(listnode_t *a, listnode_t *b, cmpfunc_t cmpfunc)
{
	listnode_t *head, *tail;
	
	/* Pick the smallest head node */
	if (cmpfunc(a->elem, b->elem) < 0) {
		head = tail = a;
		a = a->next;
	}
	else {
		head = tail = b;
		b = b->next;
	}
	/* Now repeatedly pick the smallest head node */
	while (a != NULL && b != NULL) {
		if (cmpfunc(a->elem, b->elem) < 0) {
			tail->next = a;
			tail = a;
			a = a->next;
		}
		else {
			tail->next = b;
			tail = b;
			b = b->next;
		}
	}
	/* Append the remaining non-empty list (if any) */
	if (a != NULL) {
		tail->next = a;
	}
	else {
		tail->next = b;
	}
	return head;
}

/*
 * Splits the given list in two halves, and returns the head of
 * the second half.
 */
static listnode_t *splitlist(listnode_t *head)
{
	listnode_t *slow, *fast, *half;
	
	/* Move two pointers, a 'slow' one and a 'fast' one which moves
	 * twice as fast.  When the fast one reaches the end of the list,
	 * the slow one will be at the middle.
	 */
	slow = head;
	fast = head->next;
	while (fast != NULL && fast->next != NULL) {
		slow = slow->next;
		fast = fast->next->next;
	}
	/* Now 'cut' the list and return the second half */
	half = slow->next;
	slow->next = NULL;
	return half;
}

/*
 * Recursive merge sort.  This function is named mergesort_ to avoid
 * collision with the mergesort function that is defined by the standard
 * library on some platforms.
 */
static listnode_t *mergesort_(listnode_t *head, cmpfunc_t cmpfunc)
{
    if (head->next == NULL) {
        return head;
    }
    else {
        listnode_t *half = splitlist(head);
        head = mergesort_(head, cmpfunc);
        half = mergesort_(half, cmpfunc);
        return merge(head, half, cmpfunc);
    }
}

void list_sort(list_t *list)
{
    if (list->head != NULL) {
        listnode_t *prev, *n;
    
        /* Recursively sort the list */
        list->head = mergesort_(list->head, list->cmpfunc);
    
        /* Fix the tail and prev links */
        prev = NULL;
        for (n = list->head; n != NULL; n = n->next) {
            n->prev = prev;
            prev = n;
        }
        list->tail = prev;
    }
}

#if 0
/*
 * Not actually used.
 */
static void list_selection_sort(list_t *list)
{
    listnode_t *min, *i, *j;

    if (list->size < 2)
	    return;

    /* Selection sort */
    for (i = list->head; i != NULL; i = i->next) {
	    min = i;
	    for (j = i->next; j != NULL; j = j->next) {
	        if (list->cmpfunc(j->elem, min->elem) < 0)
		        min = j;
	    }
	    if (min != i) {
	        void *tmp = min->elem;
	        min->elem = i->elem;
	        i->elem = tmp;
	    }
    }
}
#endif /* 0 */

list_iter_t *list_createiter(list_t *list)
{
    list_iter_t *iter = malloc(sizeof(list_iter_t));
    if (iter == NULL)
	    fatal_error("out of memory");
    iter->node = list->head;
    return iter;
}

void list_destroyiter(list_iter_t *iter)
{
    free(iter);
}

int list_hasnext(list_iter_t *iter)
{
    if (iter->node == NULL)
	    return 0;
    else
	    return 1;
}

void *list_next(list_iter_t *iter)
{
    if (iter->node == NULL) {
	    fatal_error("list iterator exhausted");
		return NULL;
    }

	void *elem = iter->node->elem;
	iter->node = iter->node->next;
	return elem;
}

