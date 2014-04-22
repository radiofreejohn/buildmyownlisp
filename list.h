#ifndef LVAL_LIST_H
#define LVAL_LIST_H

struct lval;

typedef struct list_node {
    struct lval* val;
    struct list_node* next;
    struct list_node* prev;
} list_node;

typedef struct list_t {
    list_node* head;
    int count;
} list_t;

list_t*      list_init(void);
void         list_push(list_t* head, struct lval* val);
struct lval* list_pop(list_t* head);
struct lval* list_index(list_t* head, int index);
void         list_remove(list_t* head, int index);
void         list_destroy(list_t* head);
void         list_replace(list_t* head, int index, struct lval* val);

#endif
