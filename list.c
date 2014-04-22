#include <stdlib.h>
#include "list.h"
#include "lispy.h"

list_t* list_init(void) {
    list_t* l = malloc(sizeof(list_t));
    l->count = 0;
    l->head = NULL;
    return l;
}

void list_push(list_t* head, struct lval* val) {
    if (head->head == NULL) {
        head->head = malloc(sizeof(list_node));
        head->head->val = val;
        head->head->next = NULL;
        head->head->prev = NULL;
        head->count = head->count + 1;
        return;
    }
    list_node* l = head->head;
    list_node* p = NULL;
    while (l != NULL) {
        p = l;
        l = l->next;
    }
    l = malloc(sizeof(list_node));
    l->prev = p;
    p->next = l;
    l->val = val;
    l->next = NULL;
    head->count = head->count + 1;
}

struct lval* list_pop(list_t* head) {
    list_node* l = head->head;
    if (l == NULL || head->count == 0) {
        return lval_err("Attempted to pop from empty list.");
    }
    while ((l != NULL) && (l->next != NULL)) {
        l = l->next;
    }
    struct lval* val = l->val;
    
    if (l->prev != NULL) {
        l->prev->next = NULL;
    }
    head->count = head->count - 1;
    // why do I need to do this?
    if (l == head->head) {
        free(head->head);
        head->head = NULL;
        return val;
    }
    free(l);
    l = NULL;
    return val;
}

struct lval* list_index(list_t* head, int index) {
    if (index > head->count) {
        return lval_err("Index out of bounds: %d", index);
    }
    list_node* l = head->head;
    while (index > 0) {
        l = l->next;
        index--;
    }
    return l->val;
}

void list_remove(list_t* head, int index) {
    list_node* l = head->head;
    while (index > 0) {
        l = l->next;
        index--;
    }
    if (l == head->head) {
        head->head = l->next;
    }
    if (l->prev != NULL) { 
        l->prev->next = l->next;
    }
    if (l->next != NULL) {
        l->next->prev = l->prev;
    }
    head->count = head->count - 1;
    free(l);
}

void list_destroy(list_t* head) {
    list_node* l = head->head;
    list_node* n = NULL;
    if (l != NULL) {
        n = l->next;
    }

    while (l != NULL) {
        free(l);
        l = NULL;
        if (n == NULL) {
            break;
        }
        l = n;
        n = l->next;
    }
    head->count = 0;
}

void list_replace(list_t* head, int index, struct lval* v) {
    list_node* l = head->head;
    while (index > 0) {
        l = l->next;
        index--;
    }
    l->val = v;
}

// iteration
struct lval* list_start(list_t* head) {
    head->end = 1;
    if (head->head == NULL || head->head->val->type == LVAL_ERR) {
        head->end = 0;
        return NULL;
    }
    head->curr = head->head;
    return head->curr->val;
}

int list_end(list_t* head) {
    return head->end;
}

struct lval* list_iter(list_t* head) {
    if (head->curr->next == NULL) {
        head->end = 0;
        // shouldn't actually be used.
        return NULL;
    }
    head->curr = head->curr->next;
    return head->curr->val;
}
