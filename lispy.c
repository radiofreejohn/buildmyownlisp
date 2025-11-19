#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* this is only included in *BSD/Mac OS X
   I prefer it over hsearch because it allows multiple
   hash tables */
#include <strhash.h>
// #include <stdbool.h>
#include "mpc.h"
#include "lispy.h"
#include "list.h"

#include <editline/readline.h>

/* Length-prefixed string implementation */

lstr* lstr_new(const char* s) {
    if (s == NULL) return NULL;
    int len = strlen(s);
    return lstr_newn(s, len);
}

lstr* lstr_newn(const char* s, int len) {
    lstr* str = malloc(sizeof(lstr));
    str->len = len;
    str->cap = len + 1;
    str->data = malloc(str->cap);
    memcpy(str->data, s, len);
    str->data[len] = '\0';
    return str;
}

void lstr_free(lstr* s) {
    if (s) {
        free(s->data);
        free(s);
    }
}

lstr* lstr_copy(lstr* s) {
    if (s == NULL) return NULL;
    return lstr_newn(s->data, s->len);
}

lstr* lstr_concat(lstr* a, lstr* b) {
    int new_len = a->len + b->len;
    lstr* result = malloc(sizeof(lstr));
    result->len = new_len;
    result->cap = new_len + 1;
    result->data = malloc(result->cap);
    memcpy(result->data, a->data, a->len);
    memcpy(result->data + a->len, b->data, b->len);
    result->data[new_len] = '\0';
    return result;
}

int lstr_cmp(lstr* a, lstr* b) {
    if (a->len != b->len) return a->len - b->len;
    return memcmp(a->data, b->data, a->len);
}

#define LASSERT(args, cond, fmt, ...) \
   if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
    LASSERT(args, ((lval*)list_index(args->cell,index))->type == expect, \
            "Function '%s' passed incorrect type for argument %d. Got %s, expected %s.", \
            func, index, ltype_name(((lval*)list_index(args->cell,index))->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
    LASSERT(args, args->count == num, \
            "Function '%s' passed incorrect number of arguments. Got %d, expected %d.", \
            func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
    LASSERT(args, ((lval*)list_index(args->cell, index))->count != 0, \
            "Function '%s' passed {} for argument %d.", func, index)

int counter;
int debug;
void count_inc(int ltype) {
    counter++;
    if (debug == 1) {
        printf("%s type: increment counter to %d\n", ltype_name(ltype), counter);
    }
}
void count_dec(int ltype) {
    counter--;
    if (debug == 1) {
        printf("%s type: decrement counter to %d\n", ltype_name(ltype), counter);
    }
}

/* Check if input has balanced brackets. Returns 0 if balanced, >0 if more opens */
static int bracket_balance(const char* input) {
    int balance = 0;
    int in_string = 0;

    for (int i = 0; input[i] != '\0'; i++) {
        char c = input[i];

        /* Handle string literals - don't count brackets inside strings */
        if (c == '"' && (i == 0 || input[i-1] != '\\')) {
            in_string = !in_string;
            continue;
        }

        if (!in_string) {
            if (c == '(' || c == '{') {
                balance++;
            } else if (c == ')' || c == '}') {
                balance--;
            }
        }
    }

    return balance;
}

#ifndef LISPY_TEST
int main(int argc, char **argv) {
    counter = 0;
    debug = 0;
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
            if (!input) break;  /* Handle EOF */

            /* Multi-line input: keep reading if brackets aren't balanced */
            int balance = bracket_balance(input);
            size_t input_len = strlen(input);
            size_t input_cap = input_len + 1;

            while (balance > 0) {
                char* continuation = readline("...... ");
                if (!continuation) break;

                /* Resize input buffer and append continuation */
                size_t cont_len = strlen(continuation);
                size_t new_len = input_len + 1 + cont_len;  /* +1 for newline */

                if (new_len + 1 > input_cap) {
                    input_cap = new_len + 256;  /* Grow with some slack */
                    input = realloc(input, input_cap);
                }

                input[input_len] = '\n';
                memcpy(input + input_len + 1, continuation, cont_len + 1);
                input_len = new_len;

                free(continuation);
                balance = bracket_balance(input);
            }

            /* Add input to history */
            add_history(input);

            // temporary hacks
            if (strcmp(input, "refs") == 0) {
                printf("refs: %d\n", counter);
                free(input);
                continue;
            }
            if (strcmp(input, "debug") == 0) {
                printf("debugging refs\n");
                debug = (debug + 1) % 2;
                free(input);
                continue;
            }
            if (strcmp(input, "builtins") == 0) {
                printf("%d builtins:\n", e->count);
                hash_traverse(e->syms, lenv_hash_print_keys, NULL);
                printf("\n");
                free(input);
                continue;
            }

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
#endif /* LISPY_TEST */

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
           // I am not confident I made this clearer :P
           for (lval* u = list_start(x->cell), * v = list_start(y->cell);
                list_end(x->cell) || list_end(y->cell);
                list_iter(x->cell), list_iter(y->cell)) {
               if (lval_eq(u, v)->num == 0) { return lval_bool(0); }
           }
           return lval_bool(1);
           break;
        case LVAL_UTYPE:
        case LVAL_UVAL:
           if (strcmp(x->type_name, y->type_name) != 0) { return lval_bool(0); }
           return lval_eq(x->fields, y->fields);
           break;
        case LVAL_FRAC:
           return lval_bool(x->numer == y->numer && x->denom == y->denom);
           break;
    }
    return lval_bool(0);
}

lval* builtin_cmp(lenv* e, lval* a, char* op) {
    LASSERT_NUM(op, a, 2);
    lval* r;
    if (strcmp(op, "eq") == 0) { r = lval_eq((lval*)list_index(a->cell, 0), (lval*)list_index(a->cell, 1)); }
    if (strcmp(op, "ne") == 0) { r = bool_negate_val(lval_eq((lval*)list_index(a->cell, 0), (lval*)list_index(a->cell, 1))); }
    lval_del(a);
    return r;
}


/* Create a new error type lval */
lval* lval_err(char* fmt, ...) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    count_inc(v->type);

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
    count_inc(v->type);
    return v;
}

/* construct a pointer to a new symbol lval */
lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->str = strdup(s);
    count_inc(v->type);
    return v;
}

/* pointer to a new empty sexpr lval */
lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = list_init();// NULL;
    count_inc(v->type);
    return v;
}

/* pointer to a new empty qexpr lval */
lval* lval_qexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = list_init(); //NULL;
    count_inc(v->type);
    return v;
}

/* delete lval pointer */
void lval_del(lval* v) {
    count_dec(v->type);
    switch (v->type) {
        case LVAL_FLOAT: break;
        case LVAL_BOOL: break;
        case LVAL_LONG: break;
        case LVAL_FRAC: break;
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
            for (lval* z = list_start(v->cell);
                 list_end(v->cell);
                 z = list_iter(v->cell)) {
                lval_del(z);
            }
            /* also free the memory allocated to contain the pointers */
            // TODO: should instead just create some way to delete the list
            list_destroy(v->cell);
            break;

        /* user-defined types */
        case LVAL_UTYPE:
        case LVAL_UVAL:
            free(v->type_name);
            lval_del(v->fields);
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
    if (mpc_parse_contents((((lval*)list_index(a->cell, 0)))->str, Lispy, &r)) {

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
    int first = 1;
    list_start(a->cell);
    while (list_end(a->cell)) {
        lval* v = list_curr(a->cell);
        if (v->type == LVAL_STR) {
            printf("%s", v->str);
        } else {
            if (!first) { putchar(' '); }
            lval_print(v);
        }
        first = 0;
        list_iter(a->cell);
    }

    // putchar('\n');
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_error(lenv* e, lval* a) {
    LASSERT_NUM("error", a, 1);
    LASSERT_TYPE("error", a, 0, LVAL_STR);

    lval* err = lval_err(((lval*)list_index(a->cell, 0))->str);

    lval_del(a);
    return err;
}

/* Helper to print indentation */
static void debug_indent(int depth) {
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
}

/* Debug version of lval_eval_sexpr */
static lval* lval_eval_sexpr_debug(lenv* e, lval* v, int depth) {
    debug_indent(depth);
    printf("EVAL S-EXPR: ");
    lval_print(v);
    printf("\n");

    /* Evaluate the children */
    int child_num = 0;
    list_start(v->cell);
    while (list_end(v->cell)) {
        lval* l = list_curr(v->cell);
        debug_indent(depth + 1);
        printf("ARG %d: ", child_num);
        lval_print(l);
        printf("\n");
        lval* evaluated = lval_eval_debug(e, l, depth + 1);
        list_replace_curr(v->cell, evaluated);
        debug_indent(depth + 1);
        printf("ARG %d => ", child_num);
        lval_print(evaluated);
        printf("\n");
        list_iter(v->cell);
        child_num++;
    }

    /* Error checking */
    int err_index = 0;
    list_start(v->cell);
    while (list_end(v->cell)) {
        lval* l = list_curr(v->cell);
        if (l->type == LVAL_ERR) {
            return lval_take(v, err_index);
        }
        err_index++;
        list_iter(v->cell);
    }

    /* Empty expression */
    if (v->count == 0) {
        debug_indent(depth);
        printf("=> (empty)\n");
        return v;
    }

    /* Single expression */
    if (v->count == 1) {
        lval* result = lval_take(v, 0);
        debug_indent(depth);
        printf("=> ");
        lval_print(result);
        printf("\n");
        return result;
    }

    /* Ensure first element is function */
    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval* err = lval_err("S-Expression starts with incorrect type. Got %s, expected %s.", ltype_name(f->type), ltype_name(LVAL_FUN));
        lval_del(f); lval_del(v);
        return err;
    }

    /* Call function */
    debug_indent(depth);
    printf("CALL: ");
    lval_print(f);
    printf(" with %d args\n", v->count);

    lval* result = lval_call(e, f, v);
    lval_del(f);

    debug_indent(depth);
    printf("=> ");
    lval_print(result);
    printf("\n");

    return result;
}

/* Debug version of lval_eval - verbose output */
lval* lval_eval_debug(lenv* e, lval* v, int depth) {
    if (v->type == LVAL_SYM) {
        debug_indent(depth);
        printf("LOOKUP: %s\n", v->str);
        lval* x = lenv_get(e, v);
        debug_indent(depth);
        printf("=> ");
        lval_print(x);
        printf("\n");
        lval_del(v);
        return x;
    }
    /* Evaluate S-expressions */
    if (v->type == LVAL_SEXPR) {
        return lval_eval_sexpr_debug(e, v, depth);
    }
    /* all other lval types remain the same */
    debug_indent(depth);
    printf("LITERAL: ");
    lval_print(v);
    printf("\n");
    return v;
}

/* Debug builtin - verbose evaluation */
lval* builtin_debug(lenv* e, lval* a) {
    LASSERT_NUM("debug", a, 1);
    LASSERT_TYPE("debug", a, 0, LVAL_QEXPR);

    lval* x = lval_pop(a, 0);
    lval_del(a);

    /* Convert Q-expression to S-expression for evaluation */
    x->type = LVAL_SEXPR;

    printf("\n=== DEBUG EVAL ===\n");
    lval* result = lval_eval_debug(e, x, 0);
    printf("=== END DEBUG ===\n\n");

    return result;
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

    if (f->formals->count > 0 && (strcmp(((lval*)list_index(f->formals->cell, 0))->str, "&") == 0)) {
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
    list_start(v->cell);
    while (list_end(v->cell)) {
        lval* l = list_curr(v->cell);
        list_replace_curr(v->cell, lval_eval(e, l));
        list_iter(v->cell);
    }

    /* Error checking */
    int err_index = 0;
    list_start(v->cell);
    while (list_end(v->cell)) {
        lval* l = list_curr(v->cell);
        if (l->type == LVAL_ERR) {
            return lval_take(v, err_index);
        }
        err_index++;
        list_iter(v->cell);
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
    if (v->type == LVAL_SYM)   { 
        lval* x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    /* Evaluate S-expressions */
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
    /* all other lval types remain the same */
    return v;
}

// probably should change this to just be a list pop
lval* lval_pop(lval* v, int index) {
    /* find the item at i */
    lval* x = (lval*)list_index(v->cell, index);
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
    list_start(a->cell);
    while (list_end(a->cell)) {
        lval* v = list_curr(a->cell);
        if (v->type != LVAL_STR) {
            lval* z = lval_to_str(v);
            if (z->type == LVAL_ERR) {
                lval_del(a);
                return z;
            }
            list_replace_curr(a->cell, z);
        }
        list_iter(a->cell);
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
    list_start(a->cell);
    while (list_end(a->cell)) {
        lval* v = list_curr(a->cell);
        if (v->type == LVAL_FLOAT) {
            hasfloat = 1;
        }
        if (v->type != LVAL_LONG && v->type != LVAL_FLOAT) {
            int type = v->type;
            lval_del(a);
            return lval_err("builtin_op: Incorrect type. Got %s, expected a number.", ltype_name(type));
        }
        list_iter(a->cell);
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
    LASSERT(a, (((lval*)list_index(a->cell, 0))->type == LVAL_STR), "Function 'strlen' passed incorrect type. Got %s, expected %s", ltype_name(((lval*)list_index(a->cell, 0))->type), ltype_name(LVAL_STR));
    lval* s = lval_pop(a, 0);
    int len = strlen(s->str);
    lval_del(s);
    lval_del(a);
    return lval_long(len);
}

/* Type casting builtins */

/* Convert to integer: (int 3.14) -> 3, (int "42") -> 42, (int true) -> 1 */
lval* builtin_int(lenv* e, lval* a) {
    LASSERT_NUM("int", a, 1);
    lval* v = lval_pop(a, 0);
    lval_del(a);

    switch (v->type) {
        case LVAL_LONG:
            return v;
        case LVAL_FLOAT: {
            long result = (long)v->num;
            lval_del(v);
            return lval_long(result);
        }
        case LVAL_BOOL: {
            long result = (int)v->num;
            lval_del(v);
            return lval_long(result);
        }
        case LVAL_STR: {
            char* endptr;
            long result = strtol(v->str, &endptr, 10);
            if (*endptr != '\0') {
                lval* err = lval_err("Cannot convert string '%s' to integer", v->str);
                lval_del(v);
                return err;
            }
            lval_del(v);
            return lval_long(result);
        }
        default: {
            lval* err = lval_err("Cannot convert %s to integer", ltype_name(v->type));
            lval_del(v);
            return err;
        }
    }
}

/* Convert to float: (float 3) -> 3.0, (float "3.14") -> 3.14 */
lval* builtin_float_cast(lenv* e, lval* a) {
    LASSERT_NUM("float", a, 1);
    lval* v = lval_pop(a, 0);
    lval_del(a);

    switch (v->type) {
        case LVAL_FLOAT:
            return v;
        case LVAL_LONG: {
            float result = v->num;
            lval_del(v);
            return lval_float(result);
        }
        case LVAL_BOOL: {
            float result = v->num;
            lval_del(v);
            return lval_float(result);
        }
        case LVAL_STR: {
            char* endptr;
            float result = strtof(v->str, &endptr);
            if (*endptr != '\0') {
                lval* err = lval_err("Cannot convert string '%s' to float", v->str);
                lval_del(v);
                return err;
            }
            lval_del(v);
            return lval_float(result);
        }
        default: {
            lval* err = lval_err("Cannot convert %s to float", ltype_name(v->type));
            lval_del(v);
            return err;
        }
    }
}

/* Convert to boolean: (bool 0) -> false, (bool 1) -> true, (bool "") -> false */
lval* builtin_bool_cast(lenv* e, lval* a) {
    LASSERT_NUM("bool", a, 1);
    lval* v = lval_pop(a, 0);
    lval_del(a);

    switch (v->type) {
        case LVAL_BOOL:
            return v;
        case LVAL_LONG:
        case LVAL_FLOAT: {
            int result = (v->num != 0);
            lval_del(v);
            return lval_bool(result);
        }
        case LVAL_STR: {
            int result = (strlen(v->str) > 0);
            lval_del(v);
            return lval_bool(result);
        }
        case LVAL_QEXPR:
        case LVAL_SEXPR: {
            int result = (v->count > 0);
            lval_del(v);
            return lval_bool(result);
        }
        default: {
            lval* err = lval_err("Cannot convert %s to boolean", ltype_name(v->type));
            lval_del(v);
            return err;
        }
    }
}

lval* builtin_head(lenv* e, lval* a) {
    /* check error conditions */
    LASSERT(a, (a->count == 1),                  "Function 'head' passed too many arguments. Got %d, expected %d.", a->count, 1);
    LASSERT(a, (((lval*)list_index(a->cell, 0))->type == LVAL_QEXPR), "Function 'head' passed incorrect type. Got %s, expected %s", ltype_name(((lval*)list_index(a->cell, 0))->type), ltype_name(LVAL_QEXPR)); 
    LASSERT(a, (((lval*)list_index(a->cell, 0))->count != 0),         "Function 'head' passed {}.");

    /* otherwise take first arg */
    lval* v = lval_take(a, 0);

    /* delete all elements that are not head and return */
    while (v->count > 1) { lval_del(lval_pop(v, 1)); }
    return v;
}

lval* builtin_tail(lenv* e, lval* a) {
    /* check error conditions */
    LASSERT(a, (a->count == 1),                  "Function 'tail' passed too many arguments. Got %d, expected %d.", a->count, 1);
    LASSERT(a, (((lval*)list_index(a->cell, 0))->type == LVAL_QEXPR), "Function 'tail' passed incorrect type. Got %s, expected %s", ltype_name(((lval*)list_index(a->cell, 0))->type), ltype_name(LVAL_QEXPR)); 
    LASSERT(a, (((lval*)list_index(a->cell, 0))->count != 0),         "Function 'tail' passed {}.");

    /* Take first arg */
    lval* v = lval_take(a, 0);

    /* Delete the first element and return */
    lval_del(lval_pop(v, 0));
    return v;
}

lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;

    lval* first = list_start(a->cell);
    if (a->count == 1 && first != NULL && first->type == LVAL_STR) {
        lval* z = lval_qexpr();
        char* str = first->str;
        int len = strlen(str);
        for (int i = 0; i < len; i++) {
            int end = 1;
            char* c = malloc(3);
            c[0] = str[i];
            switch (c[0]) {
                case '\a':
                    c[0] = '\\';
                    c[1] = 'a';
                    end++;
                    break;
                case '\b':
                    c[0] = '\\';
                    c[1] = 'b';
                    end++;
                    break;
                case '\f':
                    c[0] = '\\';
                    c[1] = 'f';
                    end++;
                    break;
                case '\n':
                    c[0] = '\\';
                    c[1] = 'n';
                    end++;
                    break;
                case '\r':
                    c[0] = '\\';
                    c[1] = 'r';
                    end++;
                    break;
                case '\t':
                    c[0] = '\\';
                    c[1] = 't';
                    end++;
                    break;
                case '\v':
                    c[0] = '\\';
                    c[1] = 'v';
                    end++;
                    break;
                case '\'':
                    c[0] = '\\';
                    c[1] = '\'';
                    end++;
                    break;
                case '\\':
                    c[0] = '\\';
                    c[1] = '\\';
                    end++;
                    break;
            }
            c[end] = '\0';
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
    int i = 0;
    list_start(a->cell);
    while (list_end(a->cell)) {
        lval* v = list_curr(a->cell);
        if (i >= 1) {
            LASSERT(a, (v->type == LVAL_QEXPR), "Expected Q-expression as an argument, but got %s", ltype_name(v->type));
        }
        i++;
        list_iter(a->cell);
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
    LASSERT(a, (((lval*)list_index(a->cell, 0))->type == LVAL_QEXPR), "Function 'eval' passed incorrect type. Got %s, expected %s", ltype_name(((lval*)list_index(a->cell, 0))->type), ltype_name(LVAL_QEXPR));

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a) {
    list_start(a->cell);
    while (list_end(a->cell)) {
        lval* v = list_curr(a->cell);
        LASSERT(a, (v->type == LVAL_QEXPR), "Function 'join' passed incorrect type. Got %s, expected %s", ltype_name(v->type), ltype_name(LVAL_QEXPR));
        list_iter(a->cell);
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
    lval* formals_check = (lval*)list_index(a->cell, 0);
    list_start(formals_check->cell);
    while (list_end(formals_check->cell)) {
        lval* v = list_curr(formals_check->cell);
        LASSERT(a, (v->type == LVAL_SYM),
                "Cannot define non-symbol. Got %s, expected %s.",
                ltype_name(v->type), ltype_name(LVAL_SYM));
        list_iter(formals_check->cell);
    }

    // pop first two arguments and pass them to lval lambda
    lval* formals = lval_pop(a, 0);
    lval* body = lval_pop(a, 0);
    lval_del(a);

    return lval_lambda(formals, body);
}

lval* builtin_var(lenv* e, lval* a, char* func) {
    LASSERT_TYPE(func, a, 0, LVAL_QEXPR);
    lval* syms = (lval*)list_index(a->cell, 0);
    list_start(syms->cell);
    while (list_end(syms->cell)) {
        lval* v = list_curr(syms->cell);
        LASSERT(a, (v->type == LVAL_SYM),
                "Function '%s' cannot define non-symbol. Got %s, expected %s.",
                func, ltype_name(v->type), ltype_name(LVAL_SYM));
        list_iter(syms->cell);
    }

    LASSERT(a, (syms->count == a->count-1),
            "Function '%s' passed too many arguments for symbols. Got %d, expected %d.",
            func, syms->count, a->count-1);

    /* Iterate over symbols and values together using index for values offset */
    list_start(syms->cell);
    int i = 0;
    while (list_end(syms->cell)) {
        lval* sym = list_curr(syms->cell);
        lval* val = (lval*)list_index(a->cell, i + 1);
        /* if def define in global scope. if put define in local scope */
        if (strcmp(func, "def") == 0) { lenv_def(e, sym, val); }
        if (strcmp(func, "="  ) == 0) { lenv_put(e, sym, val); }
        list_iter(syms->cell);
        i++;
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
    count_inc(v->type);
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
        case LVAL_FRAC:
           x->numer = v->numer;
           x->denom = v->denom;
           break;

        /* copy strings using strdup */
        case LVAL_STR:
        case LVAL_ERR:
        case LVAL_SYM: x->str = strdup(v->str); break;

        /* copy lists by copying each sub expression */
        case LVAL_SEXPR:
        case LVAL_QEXPR:
          x->count = v->count;
          x->cell = list_init();
          list_start(v->cell);
          while (list_end(v->cell)) {
              lval* elem = list_curr(v->cell);
              list_push(x->cell, lval_copy(elem));
              list_iter(v->cell);
          }
          break;

        /* copy user-defined types */
        case LVAL_UTYPE:
        case LVAL_UVAL:
          x->type_name = strdup(v->type_name);
          x->fields = lval_copy(v->fields);
          break;
    }

    return x;
}

/* print sexpr */
void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    int first = 1;
    list_start(v->cell);
    while (list_end(v->cell)) {
        lval* elem = list_curr(v->cell);
        /* print space before all but first element */
        if (!first) {
            putchar(' ');
        }
        /* print value contained within */
        lval_print(elem);
        first = 0;
        list_iter(v->cell);
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
        case LVAL_FRAC:  printf("%ld/%ld", v->numer, v->denom); break;
        case LVAL_BOOL:  printf("%s", (int) v->num ? "true" : "false"); break;
        case LVAL_ERR:   printf("Error: %s", v->str); break;
        case LVAL_SYM:   printf("%s", v->str); break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
        case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
        case LVAL_UTYPE:
            printf("<type %s ", v->type_name);
            lval_print(v->fields);
            printf(">");
            break;
        case LVAL_UVAL:
            printf("<%s ", v->type_name);
            lval_print(v->fields);
            printf(">");
            break;
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
          //  lval_del(v);
            return lval_str(buffer);
        case LVAL_BOOL:
            {
            int num = (int) v->num;
          //  lval_del(v);
            return lval_str((num ? "true" : "false"));
            } break;
        case LVAL_STR:
        case LVAL_ERR:
        case LVAL_SYM:
            return v;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
        case LVAL_FUN:
        default:
            // something is wrong here, I don't quite understand
            // why lval_del chokes here but not above
            return lval_err("Cannot convert values of this type.");
            break;
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
    e->syms = hash_create(51);
    return e;
}

void lenv_del(lenv* e) {
    hash_purge(e->syms, lenv_hash_purge);
    free(e->syms);
    free(e);
}

lval* lenv_get(lenv* e, lval* k) {
    lval* z = hash_search(e->syms, k->str, NULL, NULL);
    if (z != NULL) {
        return lval_copy(z);
    } else if (e->par) {
        // if no symbol found, check in the parent
        return lenv_get(e->par, k);
    } else {
        /* if no symbol found, return error */
        return lval_err("Unbound symbol '%s'", k->str);
    }
}

void lenv_put(lenv* e, lval* k, lval* v) {
    /* iterate over items in environment to see if variable already exists */
    lval* z = hash_search(e->syms, k->str, lval_copy(v), lval_del);

    // hash_search returns NULL if a key exists and value is replaced
    if (z != NULL) {
        e->count++;
    }
    // should v be deleted?
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

    // type casting
    lenv_add_builtin(e, "int", builtin_int);
    lenv_add_builtin(e, "float", builtin_float_cast);
    lenv_add_builtin(e, "bool", builtin_bool_cast);

    // load and print
    lenv_add_builtin(e, "load", builtin_load);
    lenv_add_builtin(e, "print", builtin_print);
    lenv_add_builtin(e, "error", builtin_error);

    // user-defined types
    lenv_add_builtin(e, "deftype", builtin_deftype);
    lenv_add_builtin(e, "new", builtin_new);
    lenv_add_builtin(e, "get", builtin_get);
    lenv_add_builtin(e, "set", builtin_set);

    // fractions
    lenv_add_builtin(e, "frac", builtin_frac);
    lenv_add_builtin(e, "numer", builtin_numer);
    lenv_add_builtin(e, "denom", builtin_denom);

    // debugging
    lenv_add_builtin(e, "debug", builtin_debug);
}

lenv* lenv_copy(lenv* e) {
    lenv* n = malloc(sizeof(lenv));
    n->par = e->par;
    n->count = e->count;
    // here, maybe could use e->count + some
    n->syms = hash_create(51);
    hash_traverse(e->syms, lenv_hash_copy_kv, n->syms);

    return n;
}

void lenv_def(lenv* e, lval* k, lval* v) {
    /* iterate until e has no parent */
    while (e->par) { e = e->par; }
    /* put value in e */
    lenv_put(e, k, v);
}

// used to clear hash table
void lenv_hash_purge(char* key, lval* v) {
    free(key);
    lval_del(v);
}

int lenv_hash_print_keys(char* key, lval* v, void* n) {
    printf("%s ", key);
    return 1;
}

int lenv_hash_copy_kv(char* key, lval* v, hash_table* h) {
    // assuming that the source table won't have dupe keys
    
    // do I need to create a fresh key with strdup?
    lval* z = lval_copy(v);
    hash_search(h, key, z, NULL);
    return 1;
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
        case LVAL_UTYPE: return "User-Type";
        case LVAL_UVAL: return "User-Value";
        case LVAL_FRAC: return "Fraction";
        default: return "Unknown";
    }
}

lval* lval_lambda(lval* formals, lval* body) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    count_inc(v->type);

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
    count_inc(v->type);
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
    count_inc(v->type);
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
    count_inc(v->type);
    return v;
}

/* booleans */
lval* lval_bool(int truth) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_BOOL;
    v->num  = truth;
    count_inc(v->type);
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
    int i = 0;
    list_start(l->cell);
    while (list_end(l->cell)) {
        lval* v = list_curr(l->cell);
        LASSERT(l, (v->type == LVAL_BOOL),
                "Function '%s' passed incorrect type for argument %d. Got %s, expected %s.",
                "and", i, ltype_name(v->type), ltype_name(LVAL_BOOL));
        if (v->num == 0) {
            lval_del(l);
            return lval_bool(0);
        }
        i++;
        list_iter(l->cell);
    }
    lval_del(l);
    return lval_bool(1);
}

lval* builtin_or(lenv* e, lval* l) {
    int i = 0;
    list_start(l->cell);
    while (list_end(l->cell)) {
        lval* v = list_curr(l->cell);
        LASSERT(l, (v->type == LVAL_BOOL),
                "Function '%s' passed incorrect type for argument %d. Got %s, expected %s.",
                "or", i, ltype_name(v->type), ltype_name(LVAL_BOOL));
        if (v->num == 1) {
            lval_del(l);
            return lval_bool(1);
        }
        i++;
        list_iter(l->cell);
    }
    lval_del(l);
    return lval_bool(0);
}

/* User-defined types */

/* Create a new user type definition */
lval* lval_utype(char* name, lval* fields) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_UTYPE;
    v->type_name = strdup(name);
    v->fields = fields;
    count_inc(v->type);
    return v;
}

/* Create a new user type instance */
lval* lval_uval(char* type_name, lval* values) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_UVAL;
    v->type_name = strdup(type_name);
    v->fields = values;
    count_inc(v->type);
    return v;
}

/* Define a new user type: (deftype {Point} {x y}) */
lval* builtin_deftype(lenv* e, lval* a) {
    LASSERT_NUM("deftype", a, 2);
    LASSERT_TYPE("deftype", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("deftype", a, 1, LVAL_QEXPR);

    lval* name_qexpr = lval_pop(a, 0);
    lval* field_names = lval_pop(a, 0);

    /* Name should be a Q-expr with a single symbol */
    if (name_qexpr->count != 1 || ((lval*)list_index(name_qexpr->cell, 0))->type != LVAL_SYM) {
        lval_del(name_qexpr);
        lval_del(field_names);
        lval_del(a);
        return lval_err("Type name must be a single symbol in a Q-expression");
    }
    lval* name = lval_pop(name_qexpr, 0);
    lval_del(name_qexpr);

    /* Verify all field names are symbols */
    list_start(field_names->cell);
    while (list_end(field_names->cell)) {
        lval* field = list_curr(field_names->cell);
        if (field->type != LVAL_SYM) {
            lval_del(name);
            lval_del(field_names);
            lval_del(a);
            return lval_err("Field names must be symbols");
        }
        list_iter(field_names->cell);
    }

    /* Create the type and bind it */
    lval* utype = lval_utype(name->str, field_names);
    lenv_def(e, name, utype);

    lval_del(name);
    lval_del(utype);
    lval_del(a);
    return lval_sexpr();
}

/* Create instance: (new {Point} 10 20) */
lval* builtin_new(lenv* e, lval* a) {
    LASSERT(a, a->count >= 1, "Function 'new' requires at least 1 argument");
    LASSERT_TYPE("new", a, 0, LVAL_QEXPR);

    /* Get the type name from Q-expr */
    lval* name_qexpr = lval_pop(a, 0);
    if (name_qexpr->count != 1 || ((lval*)list_index(name_qexpr->cell, 0))->type != LVAL_SYM) {
        lval_del(name_qexpr);
        lval_del(a);
        return lval_err("Type name must be a single symbol in a Q-expression");
    }
    lval* type_sym = lval_pop(name_qexpr, 0);
    lval_del(name_qexpr);

    lval* utype = lenv_get(e, type_sym);

    if (utype->type == LVAL_ERR) {
        lval_del(type_sym);
        lval_del(a);
        return utype;
    }

    if (utype->type != LVAL_UTYPE) {
        lval* err = lval_err("'%s' is not a user-defined type", type_sym->str);
        lval_del(type_sym);
        lval_del(utype);
        lval_del(a);
        return err;
    }

    /* Check argument count matches field count */
    int field_count = utype->fields->count;
    if (a->count != field_count) {
        lval* err = lval_err("Type '%s' expects %d fields, got %d",
                            type_sym->str, field_count, a->count);
        lval_del(type_sym);
        lval_del(utype);
        lval_del(a);
        return err;
    }

    /* Create the instance with values */
    lval* values = lval_qexpr();
    while (a->count > 0) {
        lval_add(values, lval_pop(a, 0));
    }

    lval* instance = lval_uval(type_sym->str, values);

    lval_del(type_sym);
    lval_del(utype);
    lval_del(a);
    return instance;
}

/* Get field value: (get instance {field_name}) */
lval* builtin_get(lenv* e, lval* a) {
    LASSERT_NUM("get", a, 2);
    LASSERT_TYPE("get", a, 0, LVAL_UVAL);
    LASSERT_TYPE("get", a, 1, LVAL_QEXPR);

    lval* instance = lval_pop(a, 0);
    lval* field_qexpr = lval_pop(a, 0);

    if (field_qexpr->count != 1 || ((lval*)list_index(field_qexpr->cell, 0))->type != LVAL_SYM) {
        lval_del(instance);
        lval_del(field_qexpr);
        lval_del(a);
        return lval_err("Field name must be a single symbol in a Q-expression");
    }
    lval* field_name = lval_pop(field_qexpr, 0);
    lval_del(field_qexpr);

    /* Look up the type to get field names */
    lval* type_sym = lval_sym(instance->type_name);
    lval* utype = lenv_get(e, type_sym);
    lval_del(type_sym);

    if (utype->type != LVAL_UTYPE) {
        lval_del(instance);
        lval_del(field_name);
        lval_del(utype);
        lval_del(a);
        return lval_err("Type '%s' not found", instance->type_name);
    }

    /* Find field index */
    int index = -1;
    int i = 0;
    list_start(utype->fields->cell);
    while (list_end(utype->fields->cell)) {
        lval* field = list_curr(utype->fields->cell);
        if (strcmp(field->str, field_name->str) == 0) {
            index = i;
            break;
        }
        i++;
        list_iter(utype->fields->cell);
    }

    if (index == -1) {
        lval* err = lval_err("Field '%s' not found in type '%s'",
                           field_name->str, instance->type_name);
        lval_del(instance);
        lval_del(field_name);
        lval_del(utype);
        lval_del(a);
        return err;
    }

    /* Get the value at that index */
    lval* result = lval_copy((lval*)list_index(instance->fields->cell, index));

    lval_del(instance);
    lval_del(field_name);
    lval_del(utype);
    lval_del(a);
    return result;
}

/* Set field value: (set instance {field_name} value) - returns new instance */
lval* builtin_set(lenv* e, lval* a) {
    LASSERT_NUM("set", a, 3);
    LASSERT_TYPE("set", a, 0, LVAL_UVAL);
    LASSERT_TYPE("set", a, 1, LVAL_QEXPR);

    lval* instance = lval_pop(a, 0);
    lval* field_qexpr = lval_pop(a, 0);
    lval* new_value = lval_pop(a, 0);

    if (field_qexpr->count != 1 || ((lval*)list_index(field_qexpr->cell, 0))->type != LVAL_SYM) {
        lval_del(instance);
        lval_del(field_qexpr);
        lval_del(new_value);
        lval_del(a);
        return lval_err("Field name must be a single symbol in a Q-expression");
    }
    lval* field_name = lval_pop(field_qexpr, 0);
    lval_del(field_qexpr);

    /* Look up the type to get field names */
    lval* type_sym = lval_sym(instance->type_name);
    lval* utype = lenv_get(e, type_sym);
    lval_del(type_sym);

    if (utype->type != LVAL_UTYPE) {
        lval_del(instance);
        lval_del(field_name);
        lval_del(new_value);
        lval_del(utype);
        lval_del(a);
        return lval_err("Type '%s' not found", instance->type_name);
    }

    /* Find field index */
    int index = -1;
    int i = 0;
    list_start(utype->fields->cell);
    while (list_end(utype->fields->cell)) {
        lval* field = list_curr(utype->fields->cell);
        if (strcmp(field->str, field_name->str) == 0) {
            index = i;
            break;
        }
        i++;
        list_iter(utype->fields->cell);
    }

    if (index == -1) {
        lval* err = lval_err("Field '%s' not found in type '%s'",
                           field_name->str, instance->type_name);
        lval_del(instance);
        lval_del(field_name);
        lval_del(new_value);
        lval_del(utype);
        lval_del(a);
        return err;
    }

    /* Create a new instance with the updated value */
    lval* new_values = lval_qexpr();
    i = 0;
    list_start(instance->fields->cell);
    while (list_end(instance->fields->cell)) {
        lval* v = list_curr(instance->fields->cell);
        if (i == index) {
            lval_add(new_values, lval_copy(new_value));
        } else {
            lval_add(new_values, lval_copy(v));
        }
        i++;
        list_iter(instance->fields->cell);
    }

    lval* result = lval_uval(instance->type_name, new_values);

    lval_del(instance);
    lval_del(field_name);
    lval_del(new_value);
    lval_del(utype);
    lval_del(a);
    return result;
}

/* Fraction implementation */

/* Greatest common divisor using Euclidean algorithm */
long gcd(long a, long b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        long t = b;
        b = a % b;
        a = t;
    }
    return a;
}

/* Create a new fraction, automatically simplified */
lval* lval_frac(long numer, long denom) {
    if (denom == 0) {
        return lval_err("Division by zero in fraction");
    }

    /* Normalize sign to numerator */
    if (denom < 0) {
        numer = -numer;
        denom = -denom;
    }

    /* Simplify using GCD */
    long g = gcd(numer, denom);
    numer /= g;
    denom /= g;

    /* If denominator is 1, return as integer */
    if (denom == 1) {
        return lval_long(numer);
    }

    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FRAC;
    v->numer = numer;
    v->denom = denom;
    count_inc(v->type);
    return v;
}

/* Create fraction: (frac 1 2) -> 1/2 */
lval* builtin_frac(lenv* e, lval* a) {
    LASSERT_NUM("frac", a, 2);
    LASSERT_TYPE("frac", a, 0, LVAL_LONG);
    LASSERT_TYPE("frac", a, 1, LVAL_LONG);

    lval* numer = lval_pop(a, 0);
    lval* denom = lval_pop(a, 0);

    lval* result = lval_frac((long)numer->num, (long)denom->num);

    lval_del(numer);
    lval_del(denom);
    lval_del(a);
    return result;
}

/* Get numerator: (numer (frac 3 4)) -> 3 */
lval* builtin_numer(lenv* e, lval* a) {
    LASSERT_NUM("numer", a, 1);

    lval* v = lval_pop(a, 0);
    lval_del(a);

    if (v->type == LVAL_FRAC) {
        long n = v->numer;
        lval_del(v);
        return lval_long(n);
    } else if (v->type == LVAL_LONG) {
        return v;
    } else {
        lval* err = lval_err("Function 'numer' requires Fraction or Integer, got %s", ltype_name(v->type));
        lval_del(v);
        return err;
    }
}

/* Get denominator: (denom (frac 3 4)) -> 4 */
lval* builtin_denom(lenv* e, lval* a) {
    LASSERT_NUM("denom", a, 1);

    lval* v = lval_pop(a, 0);
    lval_del(a);

    if (v->type == LVAL_FRAC) {
        long d = v->denom;
        lval_del(v);
        return lval_long(d);
    } else if (v->type == LVAL_LONG) {
        lval_del(v);
        return lval_long(1);
    } else {
        lval* err = lval_err("Function 'denom' requires Fraction or Integer, got %s", ltype_name(v->type));
        lval_del(v);
        return err;
    }
}
