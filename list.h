#ifndef LVAL_LIST_H
#define LVAL_LIST_H

typedef struct list_node {
    void* val;
    struct list_node* next;
    struct list_node* prev;
} list_node;

typedef struct list_t {
    list_node* head;
    int count;
    int end;
    list_node* curr;
} list_t;

list_t*      list_init(void);
void         list_push(list_t* head, void* val);
void* list_pop(list_t* head);
void* list_index(list_t* head, int index);
void         list_remove(list_t* head, int index);
void         list_destroy(list_t* head);
void         list_replace(list_t* head, int index, void* val);

// iteration
void* list_start(list_t* head);
int          list_end(list_t* head);
void* list_iter(list_t* head);
#endif
