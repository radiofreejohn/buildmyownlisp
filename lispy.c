#include <stdio.h>
#include <stdlib.h>
// #include <stdbool.h>
#include "mpc.h"
#include "list.h"

#include <editline/readline.h>

#define LASSERT(args, cond, fmt, ...) \
   if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
    LASSERT(args, list_index(args->cell,index)->type == expect, \
            "Function '%s' passed incorrect type for argument %d. Got %s, expected %s.", \
            func, index, ltype_name(list_index(args->cell,index)->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
    LASSERT(args, args->count == num, \
            "Function '%s' passed incorrect number of arguments. Got %d, expected %d.", \
            func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
    LASSERT(args, list_index(args->cell, index)->count != 0, \
            "Function '%s' passed {} for argument %d.", func, index)



/* typedef for function pointers has a special form.
 * the typical form is typedef oldname newname, but
 * for function pointers it's typedef returnval(name)(args);
 */
struct lval;
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
    char** syms;
    lval** vals;
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

/* enum -> name */
char* ltype_name(int t);

/* lambda stuff */
lval* lval_lambda(lval*, lval*);

/* Possible lval types */
enum {
    LVAL_ERR,
    LVAL_LONG,
    LVAL_FLOAT,
    LVAL_BOOL,
    LVAL_STR,
    LVAL_SYM,
    LVAL_FUN,
    LVAL_SEXPR,
    LVAL_QEXPR
};

/* Possible error types */
enum {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM
};

int main(int argc, char **argv) {
	/* Create Some Parsers */
	Number  = mpc_new("number");
    Bool    = mpc_new("bool");
    String  = mpc_new("string");
    Symbol  = mpc_new("symbol");
    Comment = mpc_new("comment");
	Sexpr   = mpc_new("sexpr");
    Qexpr   = mpc_new("qexpr");
	Expr    = mpc_new("expr");
	Lispy   = mpc_new("lispy");

	/* Define them with the following Language */
	mpca_lang(MPCA_LANG_DEFAULT,
		  "                                                      \
            number   : /-?[0-9]+(\\.[0-9]+)?/ ;                \
            bool     : /true|false/ ;                            \
            string   : /\"(\\\\.|[^\"])*\"/ ;                  \
            symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;        \
            comment  : /;[^\\r\\n]*/ ;                           \
            sexpr    : '(' <expr>* ')' ;                         \
            qexpr    : '{' <expr>* '}' ;                         \
            expr     : <number> | <bool> | <comment> | <string> | <symbol> | <sexpr> | <qexpr>; \
            lispy    : /^/ <expr>* /$/ ;                         \
          ",
		  Number, Bool, String, Symbol, Comment, Sexpr, Qexpr, Expr, Lispy);

    /* Set up the environment */
    lenv* e = lenv_new();
    lenv_add_builtins(e);
    
    if (argc == 1) {
        /* Print Version and Exit Information */
        puts("Lispy Version 0.0.0.0.1");
        puts("Press Ctrl+c to Exit\n");

        /* In a never ending loop */
        while (1) {

            /* Output our prompt and get input */
            char* input = readline("lispy> ");

            /* Add input to history */
            add_history(input);

            /* Attempt to Parse the user Input */
            mpc_result_t r;

            if (mpc_parse("<lispy>", input, Lispy, &r)) {
                lval* z = lval_read(r.output);
                lval* x = lval_eval(e, z);
                if (!(x->type == LVAL_SEXPR && x->count == 0)) {
                    lval_println(x);
                }
                lval_del(x);

            } else {
                /* Otherwise Print the Error */
                mpc_err_print(r.error);
                mpc_err_delete(r.error);
            }
            /* Free retrived input */
            free(input);

        }
    }

    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {
            // make an expr with a string containing the file name to load
            lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));

            // load it with builtin load
            lval* x = builtin_load(e, args);
            // duplicative?
            if (x->type == LVAL_ERR) { lval_println(x); }
            lval_del(x);

        }
    }

    lenv_del(e);
    mpc_cleanup(9, Number, Symbol, Bool, String, Comment, Sexpr, Qexpr, Expr, Lispy);

	return 0;
}

lval* lval_eq(lval* x, lval* y) {
    if (x->type != y->type) { return lval_bool(0); }

    switch (x->type) {
        case LVAL_BOOL:
        case LVAL_LONG: 
        case LVAL_FLOAT:
            return lval_bool((x->num == y->num)); break;
        /* LVAL_STR, LVAL_ERR, LVAL_SYM all contain a string */
        case LVAL_STR:
        case LVAL_ERR:
        case LVAL_SYM: return lval_bool((strcmp(x->str, y->str) == 0)); break;
        case LVAL_FUN:
           if (x->builtin) {
               return lval_bool((x->builtin == y->builtin));
           } else {
               return lval_eq(lval_eq(x->formals, y->formals),
                              lval_eq(x->body, y->body));
           }
        case LVAL_QEXPR:
        case LVAL_SEXPR:
           if (x->count != y->count) { return lval_bool(0); }
           for (int i = 0; i < x->count; i++) {
               if (lval_eq(list_index(x->cell, i), list_index(y->cell, i))->num == 0) { return lval_bool(0); }
           }
           return lval_bool(1);
           break;
    }
    return lval_bool(0);
}

lval* builtin_cmp(lenv* e, lval* a, char* op) {
    LASSERT_NUM(op, a, 2);
    lval* r;
    if (strcmp(op, "eq") == 0) { r = lval_eq(list_index(a->cell, 0), list_index(a->cell, 1)); }
    if (strcmp(op, "ne") == 0) { r = bool_negate_val(lval_eq(list_index(a->cell, 0), list_index(a->cell, 1))); }
    lval_del(a);
    return r;
}


/* Create a new error type lval */
lval* lval_err(char* fmt, ...) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;

    /* create a va list and initialize it */
    va_list va;
    va_start(va, fmt);

    /* allocate 512 bytes of space (why 512?) */
    v->str = malloc(512);

    /* printf into the error string with max of 511 characters */
    vsnprintf(v->str, 511, fmt, va);

    /* reallocate to number of bytes actually used */
    v->str = realloc(v->str, strlen(v->str)+1);

    /* clean up va list */
    va_end(va);

    return v;
}

lval* lval_builtin(lbuiltin func) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = func;
    return v;
}

/* construct a pointer to a new symbol lval */
lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->str = strdup(s);
    return v;
}

/* pointer to a new empty sexpr lval */
lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = list_init();// NULL;
    return v;
}

/* pointer to a new empty qexpr lval */
lval* lval_qexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = list_init(); //NULL;
    return v;
}

/* delete lval pointer */
void lval_del(lval* v) {
    switch (v->type) {
        case LVAL_FLOAT: break;
        case LVAL_BOOL: break;
        case LVAL_LONG: break;
        case LVAL_FUN:
            if (!v->builtin) {
                   lenv_del(v->env);
                   lval_del(v->formals);
                   lval_del(v->body);
            }
            break;
    
        /* free string data */
        case LVAL_STR:
        case LVAL_ERR:
        case LVAL_SYM: free(v->str); break;

        /* for sexpr delete all elements inside */
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for (int i = 0; i < v->cell->count; i++) {
                // TODO: should pop all the values off to free
                // them from the list
                lval* z = list_index(v->cell, i);
                lval_del(z);
            }
            /* also free the memory allocated to contain the pointers */
            // TODO: should instead just create some way to delete the list
            list_destroy(v->cell);
            break;
    }
    free(v);
}

// ok, I think I get it, these will pass an expression (+ 3 3) -> lval (3 3)
lval* builtin_add(lenv* e, lval* a) { return builtin_op(e, a, "+"); }
lval* builtin_sub(lenv* e, lval* a) { return builtin_op(e, a, "-"); }
lval* builtin_mul(lenv* e, lval* a) { return builtin_op(e, a, "*"); }
lval* builtin_div(lenv* e, lval* a) { return builtin_op(e, a, "/"); }
lval* builtin_eq(lenv* e, lval* a)  { return builtin_cmp(e, a, "eq"); }
lval* builtin_ne(lenv* e, lval* a)  { return builtin_cmp(e, a, "ne"); }
lval* builtin_lt(lenv* e, lval* a)  { return builtin_op(e, a, "lt"); }
lval* builtin_gt(lenv* e, lval* a)  { return builtin_op(e, a, "gt"); }
lval* builtin_le(lenv* e, lval* a)  { return builtin_op(e, a, "le"); }
lval* builtin_ge(lenv* e, lval* a)  { return builtin_op(e, a, "ge"); }

// string operations
lval* builtin_str(lenv* e, lval* a) { return builtin_str_op(e, a, "str"); }

// unary operators
lval* builtin_not(lenv* e, lval* a) { return bool_negate_expr(a);}

lval* builtin_load(lenv* e, lval* a) {
    LASSERT_NUM("load", a, 1);
    LASSERT_TYPE("load", a, 0, LVAL_STR);

    // parse file given by string name
    mpc_result_t r;
    if (mpc_parse_contents(list_index(a->cell, 0)->str, Lispy, &r)) {

        // read contents
        lval* expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        // evaluate each expression
        while (expr->count) {
            lval* x = lval_eval(e, lval_pop(expr, 0));
            // if eval leads to an error, print it
            if (x->type == LVAL_ERR) { lval_println(x); }
            lval_del(x);
        }

        // delete expressions and arguments
        lval_del(expr);
        lval_del(a);

        return lval_sexpr();
    } else {
        // get parse error as string
        char* err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        // create new error message
        lval* err = lval_err("Could not load file %s", err_msg);
        free(err_msg);
        lval_del(a);

        return err;
    }
}

lval* builtin_print(lenv* e, lval* a) {
    for (int i = 0; i < a->count; i++) {
        if (list_index(a->cell, i)->type == LVAL_STR) {
            printf("%s", list_index(a->cell, i)->str);
        } else {
            lval_print(list_index(a->cell, i)); 
            if (i != a->count-1) { putchar(' '); }
        }
    }

    // putchar('\n');
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_error(lenv* e, lval* a) {
    LASSERT_NUM("error", a, 1);
    LASSERT_TYPE("error", a, 0, LVAL_STR);

    lval* err = lval_err(list_index(a->cell, 0)->str);

    lval_del(a);
    return err;
}

lval* lval_call(lenv* e, lval* f, lval* a) {
    // if builtin, call it
    if (f->builtin) { return f->builtin(e, a); }

    // record argument counts
    int given = a->count;
    int total = f->formals->count;

    // while args still remain to be processed
    while (a->count) {
        if (f->formals->count == 0) {
            lval_del(a);
            return lval_err("Function passed too many arguments. Got %d, expected %d", given, total);
        }

        // pop the first symbol from formals
        lval* sym = lval_pop(f->formals, 0);
        if (strcmp(sym->str, "&") == 0) {
            // ensure & is followed by another symbol
            if (f->formals->count != 1) {
                lval_del(a);
                return lval_err("Function format invalid. Symbol '&' not followed by a single symbol.");
            }

            // next formal should be bound to remaining arguments
            lval* nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a));
            lval_del(sym);
            lval_del(nsym);
            break;
        }

        lval* val = lval_pop(a, 0);

        // bind a copy into the function's env
        lenv_put(f->env, sym, val);

        //delete sym and val
        lval_del(sym);
        lval_del(val);
    }

    lval_del(a);

    if (f->formals->count > 0 && (strcmp(list_index(f->formals->cell, 0)->str, "&") == 0)) {
        // check to ensure that & is not passed invalidly
        if (f->formals->count != 2) {
            return lval_err("Function format invalid. Symbol '&' not followed by a single symbol.");
        }
        lval_del(lval_pop(f->formals, 0));

        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_qexpr();

        // bind to environment and delete
        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }

    if (f->formals->count == 0) {
        f->env->par = e;

        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    } else {
        return lval_copy(f);
    }
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    list_push(v->cell, x);
    // v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    // v->cell[v->count-1] = x;
    return v;
}

lval* lval_read(mpc_ast_t* t) {
    
    /* if symbol or number, return conversion to that type */
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "float")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
    if (strstr(t->tag, "bool"))   { return lval_read_bool(t); }
    if (strstr(t->tag, "string")) { return lval_read_str(t); }

    /* if root (>) or sexpr then create an empty list */ 
    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
    if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

    /* Fill this list with any valid expression contained within */
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        if (strstr(t->children[i]->tag, "comment")) { continue; }
        lval* z = lval_read(t->children[i]);
        x = lval_add(x, z);
    }
    return x;
}

lval* lval_eval_sexpr(lenv* e, lval* v) {

    /* Evaluate the children */
    for (int i = 0; i < v->count; i++) {
        // TODO: is this OK?
        lval* l = list_index(v->cell, i);
        list_replace(v->cell, i, lval_eval(e, l));
        // v->cell[i] = lval_eval(e, v->cell[i]);
    }

    /* Error checking */
    for (int i = 0; i < v->count; i++) {
        if (list_index(v->cell, i)->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }

    /* Empty expression */
    if (v->count == 0) { return v; }

    /* Single expression */
    if (v->count == 1) { return lval_take(v, 0); }

    /* Ensure first element is symbol */
    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval* err = lval_err("S-Expression starts with incorrect type. Got %s, expected %s.", ltype_name(f->type), ltype_name(LVAL_FUN));
        lval_del(f); lval_del(v);
        return err;
    }

    /* Call builtin with operator */
    lval* result = lval_call(e, f, v);
    // should v be deleted too?
    lval_del(f);
    return result;
}

lval* lval_eval(lenv* e, lval* v) {
    if (v->type == LVAL_SYM)   { return lenv_get(e, v); }
    /* Evaluate S-expressions */
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
    /* all other lval types remain the same */
    return v;
}

// probably should change this to just be a list pop
lval* lval_pop(lval* v, int index) {
    /* find the item at i */
    lval* x = list_index(v->cell, index);
    list_remove(v->cell, index);

    /* shift the memory following the item at i over the top of it */
    // memmove(&v->cell[index], &v->cell[index+1], sizeof(lval*) * (v->count-index-1));

    v->count--;

    /* reallocate the memory used */
    // v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval* builtin_str_op(lenv* e, lval* a, char* op) {

    // convert all arguments to strings
    // this is wrong - but for now I'll keep it
    // may not always want to convert arguments, 
    // for example when slicing a single string
    // making an assumption that these operate
    // on an arbitrary number of variable
    // arguments that all must be strings
    for (int i = 0; i < a->count; i++) {
        if (list_index(a->cell, i)->type != LVAL_STR) {
            list_replace(a->cell, i, lval_to_str(list_index(a->cell, i)));
            // a->cell[i] = lval_to_str(a->cell[i]);
        }
    }
    lval* x = lval_pop(a, 0);

    while (a->count > 0 && x->type != LVAL_ERR) {
        lval* y = lval_pop(a, 0);
        
        // perform operation
        if (strcmp(op, "str") == 0) {
            x->str = realloc(x->str, strlen(x->str) + strlen(y->str) + 1);
            strncat(x->str, y->str, strlen(y->str));
        }
        lval_del(y);
    }
    lval_del(a);

    return x;
}

lval* builtin_op(lenv* e, lval* a, char* op) {
    /* Ensure all args are numbers */
    int hasfloat = 0;
    for (int i = 0; i < a->count; i++) {
        if (list_index(a->cell, i)->type == LVAL_FLOAT) {
            hasfloat = 1;
        }
        if ((list_index(a->cell, i)->type != LVAL_LONG) && (list_index(a->cell, i)->type != LVAL_FLOAT)) {
            int type = list_index(a->cell, i)->type;
            lval_del(a);
            return lval_err("builtin_op: Incorrect type. Got %s, expected a number.", ltype_name(type));
        }
    }

    /* Pop the first element */
    lval* x = lval_pop(a, 0);
    if (hasfloat == 1) {
        // x is the result in the end, it's the only one that needs to be a float
        x->type = LVAL_FLOAT;
    }

    /* if no args and sub then perform unary negation */
    if ((strcmp(op, "-") == 0) && a->count == 0) { x->num = -x->num; }

    /* while there are still elements remaining */

    while (a->count > 0) {

        /* pop the next element */
        lval* y = lval_pop(a, 0);

        /* perform operation */
        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) {
            x->type = LVAL_FLOAT;
            if (y->num == 0) {
                lval_del(x); lval_del(y);
                x = lval_err("Division by zero."); break;
            } else {
                x->num /= y->num;
            }
        }
        
        /* this isn't really necessary
        if (x->num == (long) x->num) {
            x->type = LVAL_LONG;
        }
        */ 

        if (strcmp(op, "lt") == 0) {
            x->num = (x->num < y->num);
            x->type = LVAL_BOOL;
        }
        if (strcmp(op, "gt") == 0) {
            x->num = (x->num > y->num);
            x->type = LVAL_BOOL;
        }
        if (strcmp(op, "le") == 0) {
            x->num = x->num <= y->num;
            x->type = LVAL_BOOL;
        }
        if (strcmp(op, "ge") == 0) {
            x->num = x->num >= y->num;
            x->type = LVAL_BOOL;
        }
        /* delete element now finished with */
        lval_del(y);
    }

    /* delete input expression and return result */
    lval_del(a);
    return x;
}

// it would be nice to be able to represent strings as lists to avoid
// a bunch of string specific operations
lval* builtin_strlen(lenv* e, lval* a) {
    LASSERT(a, (a->count == 1),                "Function 'strlen' passed too many arguments. Got %d, expected %d.", a->count, 1);
    LASSERT(a, (list_index(a->cell, 0)->type == LVAL_STR), "Function 'strlen' passed incorrect type. Got %s, expected %s", ltype_name(list_index(a->cell, 0)->type), ltype_name(LVAL_STR));
    lval* s = lval_pop(a, 0);
    int len = strlen(s->str);
    lval_del(s);
    lval_del(a);
    return lval_long(len);
}

lval* builtin_head(lenv* e, lval* a) {
    /* check error conditions */
    LASSERT(a, (a->count == 1),                  "Function 'head' passed too many arguments. Got %d, expected %d.", a->count, 1);
    LASSERT(a, (list_index(a->cell, 0)->type == LVAL_QEXPR), "Function 'head' passed incorrect type. Got %s, expected %s", ltype_name(list_index(a->cell, 0)->type), ltype_name(LVAL_QEXPR)); 
    LASSERT(a, (list_index(a->cell, 0)->count != 0),         "Function 'head' passed {}.");

    /* otherwise take first arg */
    lval* v = lval_take(a, 0);

    /* delete all elements that are not head and return */
    while (v->count > 1) { lval_del(lval_pop(v, 1)); }
    return v;
}

lval* builtin_tail(lenv* e, lval* a) {
    /* check error conditions */
    LASSERT(a, (a->count == 1),                  "Function 'tail' passed too many arguments. Got %d, expected %d.", a->count, 1);
    LASSERT(a, (list_index(a->cell, 0)->type == LVAL_QEXPR), "Function 'tail' passed incorrect type. Got %s, expected %s", ltype_name(list_index(a->cell, 0)->type), ltype_name(LVAL_QEXPR)); 
    LASSERT(a, (list_index(a->cell, 0)->count != 0),         "Function 'tail' passed {}.");

    /* Take first arg */
    lval* v = lval_take(a, 0);

    /* Delete the first element and return */
    lval_del(lval_pop(v, 0));
    return v;
}

lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;

    if (a->count == 1 && list_index(a->cell, 0)->type == LVAL_STR) {
        lval* z = lval_qexpr();
        for (int i = 0; i < strlen(list_index(a->cell, 0)->str); i++) {
            char* c = malloc(2);
            c[0] = list_index(a->cell, 0)->str[i];
            c[1] = '\0';
            lval* ss = lval_sym(c);
            z = lval_add(z, ss);
            free(c);
        }
        lval_del(a);
        return z;
    }

    return a;
}

lval* builtin_if(lenv* e, lval* a) {
    LASSERT(a, (a->count == 3), "Function 'if' passed incorrect number of arguments. Got %d, expected %d.", a->count, 3);
    for (int i = 1; i < a->count; i++) {
        LASSERT(a, (list_index(a->cell, i)->type == LVAL_QEXPR), "Expected Q-expression as an argument, but got %s", ltype_name(list_index(a->cell, i)->type));
    }
    // type check the arguments later... derp
    lval* condition = lval_pop(a, 0);
    lval* truecond  = lval_pop(a, 0);
    lval* falsecond = lval_pop(a, 0);
    lval* truth;
    if (condition->type == LVAL_QEXPR) {
        condition->type = LVAL_SEXPR;
        // lval_eval deletes condition
        truth = lval_eval(e, condition);
    } else {
        // an s-expression, already evaluated
        truth = condition;
    }

    LASSERT(truth, (truth->type == LVAL_BOOL), "First argument must be conditional. Got %s", ltype_name(truth->type));
    if ((int) truth->num == 1) {
        lval_del(falsecond);
        lval_del(truth);
        truecond->type = LVAL_SEXPR;
        return lval_eval(e, truecond);
    } else if ((int) truth->num == 0) {
        lval_del(truecond);
        lval_del(truth);
        falsecond->type = LVAL_SEXPR;
        return lval_eval(e, falsecond);
    } else {
        lval_del(truecond);
        lval_del(falsecond);
        int boolval = truth->num;
        lval_del(truth);
        return lval_err("Bool value somehow wasn't true or false. Got %d", boolval);
    }
}

lval* builtin_eval(lenv* e, lval* a) {
    LASSERT(a, (a->count == 1),                  "Function 'eval' passed too many arguments. Got %d, expected %d.", a->count, 1);
    LASSERT(a, (list_index(a->cell, 0)->type == LVAL_QEXPR), "Function 'eval' passed incorrect type. Got %s, expected %s", ltype_name(list_index(a->cell, 0)->type), ltype_name(LVAL_QEXPR));

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT(a, (list_index(a->cell, i)->type == LVAL_QEXPR), "Function 'join' passed incorrect type. Got %s, expected %s", ltype_name(list_index(a->cell, i)->type), ltype_name(LVAL_QEXPR));
    }

    lval* x = lval_pop(a, 0);

    while (a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }

    lval_del(a);
    return x;
}

lval* builtin_lambda(lenv* e, lval* a) {
    // check two arguments, each of which are Q-expressions
    LASSERT_NUM("\\", a, 2);
    LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

    // check first Q expression contains only symbols
    for (int i = 0; i < list_index(a->cell, 0)->count; i++) {
        // whoa, this is not cool
        LASSERT(a, (list_index(list_index(a->cell, 0)->cell, i)->type == LVAL_SYM),
                "Cannot define non-symbol. Got %s, expected %s.",
                ltype_name(list_index(list_index(a->cell, 0)->cell, i)->type), ltype_name(LVAL_SYM));
    }

    // pop first two arguments and pass them to lval lambda
    lval* formals = lval_pop(a, 0);
    lval* body = lval_pop(a, 0);
    lval_del(a);

    return lval_lambda(formals, body);
}

lval* builtin_var(lenv* e, lval* a, char* func) {
    LASSERT_TYPE(func, a, 0, LVAL_QEXPR);
    lval* syms = list_index(a->cell, 0);
    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, (list_index(syms->cell, i)->type == LVAL_SYM),
                "Function '%s' cannot define non-symbol. Got %s, expected %s.",
                func, ltype_name(list_index(syms->cell, i)->type), ltype_name(LVAL_SYM));
    }

    LASSERT(a, (syms->count == a->count-1),
            "Function '%s' passed too many arguments for symbols. Got %d, expected %d.",
            func, syms->count, a->count-1);

    for (int i = 0; i < syms->count; i++) {
        /* if def define in global scope. if put define in local scope */
        if (strcmp(func, "def") == 0) { lenv_def(e, list_index(syms->cell, i), list_index(a->cell, i+1)); }
        if (strcmp(func, "="  ) == 0) { lenv_put(e, list_index(syms->cell, i), list_index(a->cell, i+1)); }
    }
    
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a) { return builtin_var(e, a, "def"); }
lval* builtin_put(lenv* e, lval* a) { return builtin_var(e, a, "=");   }

lval* lval_join(lval* x, lval* y) {
    /* for each cell in y, add it to x */
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }

    /* Delete the empty y and return x */
    lval_del(y);
    return x;
}

lval* lval_copy(lval* v) {
    lval* x = malloc(sizeof(lval));
    x->type = v->type;
    switch (v->type) {
        /* copy functions and numbers directly */
        case LVAL_FUN: 
           if (v->builtin) {
               x->builtin = v->builtin;
           } else {
               x->builtin = NULL;
               x->env = lenv_copy(v->env);
               x->formals = lval_copy(v->formals);
               x->body = lval_copy(v->body);
           }
           break;
        case LVAL_FLOAT:
        case LVAL_BOOL:
        case LVAL_LONG: x->num = v->num; break;

        /* copy strings using strdup */
        case LVAL_STR:
        case LVAL_ERR:
        case LVAL_SYM: x->str = strdup(v->str); break;

        /* copy lists by copying each sub expression */
        case LVAL_SEXPR:
        case LVAL_QEXPR:
          x->count = v->count;
          x->cell = list_init();//  malloc(sizeof(lval*) * x->count);
          for (int i = 0; i < x->count; i++) {
              // doesn't this potentially double allocate when copying sub expressions?
              // is this cleaned up later?
              list_push(x->cell, lval_copy(list_index(v->cell, i)));
              // x->cell[i] = lval_copy(v->cell[i]);
          }
          break;
    }

    return x;
}

/* print sexpr */
void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        /* print value contained within */
        lval_print(list_index(v->cell, i));

        /* don't print trailing space if last element */
        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

/* Print an lval */
void lval_print(lval* v) {
    switch (v->type) {
        /* In the case the type is a number print it then break out */
        case LVAL_FUN:
            if (v->builtin) {
                printf("<builtin>");
            } else {
                printf("(\\ ");
                lval_print(v->formals); putchar(' '); lval_print(v->body); putchar(')');
            }
            break;
        case LVAL_STR:   lval_print_str(v); break;
        case LVAL_LONG:  printf("%li", (long) v->num); break;
        case LVAL_FLOAT: printf("%g", v->num); break;
        case LVAL_BOOL:  printf("%s", (int) v->num ? "true" : "false"); break;
        case LVAL_ERR:   printf("Error: %s", v->str); break;
        case LVAL_SYM:   printf("%s", v->str); break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
        case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    }
}

// convert an lval to a string
// QUESTION: should I be deleting the original value here or
// leave that to the caller?
lval* lval_to_str(lval* v) {
    char buffer[65];
    switch (v->type) {
        case LVAL_LONG:
        case LVAL_FLOAT:
            snprintf(buffer, 64, "%g", v->num);
            lval_del(v);
            return lval_str(buffer);
        case LVAL_BOOL:
            {
            int num = (int) v->num;
            lval_del(v);
            return lval_str((num ? "true" : "false"));
            } break;
        case LVAL_STR:
            return v;
        case LVAL_ERR:
        case LVAL_SYM:
        case LVAL_SEXPR:
        case LVAL_QEXPR:
        case LVAL_FUN:
        default:
            {
            int ltype = v->type;
            lval_del(v);
            return lval_err("Error: Cannot convert a %s to a string.", ltype_name(ltype));
            } break;
    }
}

void lval_println(lval* v) {
    lval_print(v);
    putchar('\n');
}

void lval_print_str(lval* v) {
    char* escaped = strdup(v->str);
    escaped = mpcf_escape(escaped);
    printf("\"%s\"", escaped);
    free(escaped);
}

lenv* lenv_new(void) {
    lenv* e = malloc(sizeof(lenv));
    e->par = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void lenv_del(lenv* e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

lval* lenv_get(lenv* e, lval* k) {

    /* Iterate over all items in environment */
    for (int i = 0; i < e->count; i++) {
        /* check if the stored string matches the symbol string
         * if it does, return a copy of the value 
         */
        if (strcmp(e->syms[i], k->str) == 0) {
            return lval_copy(e->vals[i]);
        }
    }
    // if no symbol found, check in the parent
    if (e->par) {
        return lenv_get(e->par, k);
    } else {
        /* if no symbol found, return error */
        return lval_err("Unbound symbol '%s'", k->str);
    }
}

void lenv_put(lenv* e, lval* k, lval* v) {
    /* iterate over items in environment to see if variable already exists */
    for (int i = 0; i < e->count; i++) {
        /* if variable is found, delete item at that position and
         * replace with variable supplied by user
         */
        if (strcmp(e->syms[i], k->str) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            // sizeof(char) should always be 1 
            e->syms[i] = realloc(e->syms[i], strlen(k->str)+1);
            strcpy(e->syms[i], k->str);
            return;
        }
    }

    /* if no existing entry found then allocate space */
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    /* copy contents of lval and symbol string into new loc */
    e->vals[e->count-1] = lval_copy(v);
    // should v be deleted?
    e->syms[e->count-1] = strdup(k->str);
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
    lval* k = lval_sym(name);
    lval* v = lval_builtin(func);
    lenv_put(e, k, v);
    lval_del(k); lval_del(v);
}

void lenv_add_builtins(lenv* e) {
    /* variable functions */
    lenv_add_builtin(e, "\\", builtin_lambda);
    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "=", builtin_put);

    /* list functions */
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);

    /* maths */
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);

    /* conditionals */
    lenv_add_builtin(e, "eq", builtin_eq);
    lenv_add_builtin(e, "ne", builtin_ne);
    lenv_add_builtin(e, "lt", builtin_lt);
    lenv_add_builtin(e, "gt", builtin_gt);
    lenv_add_builtin(e, "le", builtin_le);
    lenv_add_builtin(e, "ge", builtin_ge);
    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, "not", builtin_not);
    lenv_add_builtin(e, "and", builtin_and);
    lenv_add_builtin(e, "or", builtin_or);

    // strings
    lenv_add_builtin(e, "str", builtin_str);
    lenv_add_builtin(e, "strlen", builtin_strlen);

    // load and print
    lenv_add_builtin(e, "load", builtin_load);
    lenv_add_builtin(e, "print", builtin_print);
    lenv_add_builtin(e, "error", builtin_error);
}

lenv* lenv_copy(lenv* e) {
    lenv* n = malloc(sizeof(lenv));
    n->par = e->par;
    n->count = e->count;
    n->syms = malloc(sizeof(char*) * n->count);
    n->vals = malloc(sizeof(lval*) * n->count);

    for (int i = 0; i < e->count; i++) {
        // could accomplish the same with strdup
        n->syms[i] = strdup(e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
}

void lenv_def(lenv* e, lval* k, lval* v) {
    /* iterate until e has no parent */
    while (e->par) { e = e->par; }
    /* put value in e */
    lenv_put(e, k, v);
}

char *ltype_name(int t) {
    switch(t) {
        case LVAL_FUN: return "Function";
        case LVAL_BOOL: return "Boolean";
        case LVAL_LONG: return "Integer";
        case LVAL_FLOAT: return "Float";
        case LVAL_STR: return "String";
        case LVAL_ERR: return "Error";
        case LVAL_SYM: return "Symbol";
        case LVAL_SEXPR: return "S-Expression";
        case LVAL_QEXPR: return "Q-Expression";
        default: return "Unknown";
    }
}

lval* lval_lambda(lval* formals, lval* body) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;

    v->builtin = NULL;

    // set up new environment for function (scope)
    v->env = lenv_new();

    v->formals = formals;
    v->body = body;
    // should we free formals and body?
    return v;
}

/* string related */
lval* lval_str(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str = strdup(s);
    return v;
}

lval* lval_read_str(mpc_ast_t* t) {
    t->contents[strlen(t->contents)-1] = '\0';
    char* unescaped = strdup(t->contents + 1);
    unescaped = mpcf_unescape(unescaped);
    lval* str = lval_str(unescaped);
    free(unescaped);
    return str;
}

/* number related */

/* Create a new number type lval */
lval* lval_long(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_LONG;
    v->num = (float) x;
    return v;
}

lval* lval_read_num(mpc_ast_t* t) {
    if (strstr(t->contents, ".")) {
        float f = strtof(t->contents, NULL);
        return errno != ERANGE ? lval_float(f) : lval_err("Invalid Float");
    } else {
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_long(x) : lval_err("invalid number");    
    }
}

lval* lval_float(float x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FLOAT;
    v->num = x;
    return v;
}

/* booleans */
lval* lval_bool(int truth) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_BOOL;
    v->num  = truth;
    return v;
}

lval* lval_read_bool(mpc_ast_t* t) {
    if (strcmp("true", t->contents) == 0) {
        return lval_bool(1);
    } else if (strcmp("false", t->contents) == 0) {
        return lval_bool(0);
    } else {
        return lval_err("Error converting boolean value %s.", t->contents);
    }
}

lval* bool_negate_val(lval* l) {
    LASSERT(l, (l->type == LVAL_BOOL), "Function 'bool_negate_val' passed wrong type. Got %s, expected %s.", ltype_name(l->type), ltype_name(LVAL_BOOL));
    if ((int)l->num == 0) {
        l->num = 1; 
    } else if ((int)l->num == 1) {
        l->num = 0;
    }
    return l;
}

lval* bool_negate_expr(lval* l) {
    LASSERT_NUM("not", l, 1);
    LASSERT_TYPE("not", l, 0, LVAL_BOOL);
    lval* v = lval_pop(l, 0);
    lval_del(l);

    if ((int)v->num == 0) {
        v->num = 1; 
    } else if ((int)v->num == 1) {
        v->num = 0;
    }
    return v;
}

lval* builtin_and(lenv* e, lval* l) {
    for (int i = 0; i < l->count; i++) {
        LASSERT_TYPE("and", l, i, LVAL_BOOL);
        if (list_index(l->cell, i)->num == 0) {
            lval_del(l);
            return lval_bool(0);
        }
    }
    lval_del(l);
    return lval_bool(1);
}

lval* builtin_or(lenv* e, lval* l) {
    for (int i = 0; i < l->count; i++) {
        LASSERT_TYPE("or", l, i, LVAL_BOOL);
        if (list_index(l->cell, i)->num == 1) {
            lval_del(l);
            return lval_bool(1);
        }
    }
    lval_del(l);
    return lval_bool(0);
}
