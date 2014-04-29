#ifndef LISPY_H
#define LISPY_H
#include <strhash.h>
#include "mpc.h"
#include "list.h"

struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;
typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
    int type;
    float num;

    /* error and symbol have some string data */
    char* str;
    lbuiltin builtin;
    lenv* env;
    lval* formals;
    lval* body;

    /* count and pointer to a list of lval* */
    int count;
    list_t* cell;
};

struct lenv {
    lenv* par;
    int count;
    hash_table* syms;
};
// forward delcare parser names
mpc_parser_t*   Number;
mpc_parser_t*   Bool;
mpc_parser_t*   String;
mpc_parser_t*   Symbol;
mpc_parser_t*   Comment;
mpc_parser_t*   Sexpr;
mpc_parser_t*   Qexpr;
mpc_parser_t*   Expr;
mpc_parser_t*   Lispy;
mpc_parser_t*   Number;
mpc_parser_t*   Bool;
mpc_parser_t*   String;
mpc_parser_t*   Symbol;
mpc_parser_t*   Comment;
mpc_parser_t*   Sexpr;
mpc_parser_t*   Qexpr;
mpc_parser_t*   Expr;
mpc_parser_t*   Lispy;

lval* lval_eval_sexpr(lenv*, lval*);
lval* lval_eval(lenv*, lval*);
lval* lval_long(long);
lval* lval_str(char*);
lval* lval_bool(int);
lval* lval_err(char*, ...);
lval* lval_sym(char*);
lval* lval_sexpr(void);
lval* lval_pop(lval*, int);
lval* lval_take(lval*, int);
lval* lval_eq(lval*, lval*);
lval* lval_to_str(lval*);

lval* builtin_op(lenv*, lval*, char*);
lval* builtin_add(lenv*, lval*);
lval* builtin_sub(lenv*, lval*);
lval* builtin_mul(lenv*, lval*);
lval* builtin_div(lenv*, lval*);
lval* builtin_head(lenv*, lval*);
lval* builtin_tail(lenv*, lval*);
lval* builtin_list(lenv*, lval*);
lval* builtin_eval(lenv*, lval*);
lval* builtin_join(lenv*, lval*);
lval* builtin_def(lenv*, lval*);
lval* builtin_lambda(lenv*, lval*);
lval* builtin_var(lenv*, lval*, char*);
lval* builtin_put(lenv*, lval*);

//hacks
lval* bool_negate_expr(lval*);
lval* bool_negate_val(lval*);
lval* builtin_if(lenv*, lval*);
lval* builtin_not(lenv*, lval*);
lval* builtin_and(lenv*, lval*);
lval* builtin_or(lenv*, lval*);
lval* builtin_or(lenv*, lval*);
lval* builtin_eq(lenv*, lval*);
lval* builtin_ne(lenv*, lval*);
lval* builtin_lt(lenv*, lval*);
lval* builtin_gt(lenv*, lval*);
lval* builtin_ge(lenv*, lval*);
lval* builtin_le(lenv*, lval*);

//string ops
lval* builtin_str_op(lenv*, lval*, char*);
lval* builtin_str(lenv*, lval*);
lval* builtin_strlen(lenv*, lval*);

// file loading
lval* builtin_load(lenv* e, lval* a);

// printing
lval* builtin_print(lenv* e, lval* a);
lval* builtin_error(lenv* e, lval* a);

lval* lval_join(lval*, lval*);
lval* lval_copy(lval*);
void  lval_del(lval*);
lval* lval_call(lenv*, lval*, lval*);

lval* lval_add(lval*, lval*);
lval* lval_read_num(mpc_ast_t*);
lval* lval_float(float);
lval* lval_read(mpc_ast_t*);
lval* lval_read_bool(mpc_ast_t*);
lval* lval_read_str(mpc_ast_t*);
void lval_expr_print(lval*, char, char);
void lval_print(lval*);
void lval_print_str(lval*);
void lval_println(lval*);

/* lenv stuff */
lenv* lenv_new(void);
void  lenv_del(lenv*);
lval* lenv_get(lenv*, lval*);
void  lenv_put(lenv*, lval*, lval*);
void lenv_add_builtin(lenv*, char*, lbuiltin);
void lenv_add_builtins(lenv*);
lenv* lenv_copy(lenv* e);
void lenv_def(lenv*, lval*, lval*);
void lenv_hash_purge(char*, lval*);
int  lenv_hash_print_keys(char*, lval*, void*);
int  lenv_hash_copy_kv(char*, lval*, hash_table*);

/* enum -> name */
char* ltype_name(int t);

/* lambda stuff */
lval* lval_lambda(lval*, lval*);

/* Possible lval types */
enum {
    LVAL_ERR,   // 0
    LVAL_LONG,  // 1
    LVAL_FLOAT, // 2
    LVAL_BOOL,  // 3
    LVAL_STR,   // 4
    LVAL_SYM,   // 5
    LVAL_FUN,   // 6
    LVAL_SEXPR, // 7
    LVAL_QEXPR  // 8
};

/* Possible error types */
enum {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM
};
#endif
