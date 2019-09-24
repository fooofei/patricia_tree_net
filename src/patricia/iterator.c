

#include "iterator.h"

#include <string.h>

void patree_iter_set(struct patree_iterator* it, const struct patree* tree)
{
    memset(it, 0, sizeof(*it));
    it->stack_pointer = it->stack;
    it->next = tree->root;
}

struct patnode* patree_iter_next(struct patree_iterator* it,
    struct patnode** out)
{
    *out = NULL;

    if (it->next == NULL) {
        return NULL;
    }
    *out = it->next;

    // advance
    struct patnode* left;
    struct patnode* right;

    left = it->next->left;
    right = it->next->right;

    if (left) {
        if (right) {
            *(it->stack_pointer) = right;
            it->stack_pointer++;
        }
        it->next = left;
    } else if (right) {
        it->next = right;
    } else if (it->stack_pointer != it->stack) {
        // the stack not empty
        it->stack_pointer--;
        it->next = *(it->stack_pointer);
    } else {
        it->next = NULL;
    }
    return *out;
}
