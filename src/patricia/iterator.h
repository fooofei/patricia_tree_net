
#ifndef PATRICIA_ITERATOR_H
#define PATRICIA_ITERATOR_H

#include "tree.h"

struct patree_iterator {
    struct patnode* stack[PATRICIA_MAXBITS + 1];
    struct patnode** stack_pointer;
    struct patnode* next;
};

void patree_iter_set(struct patree_iterator* it, const struct patree* tree);
// write next node to *out and return it
// return NULL for no next
struct patnode* patree_iter_next(struct patree_iterator* it, struct patnode** out);

#endif // !PATRICIA_ITERATOR_H
