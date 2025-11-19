#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ptest.h"
#include "lispy.h"

/* Test helper to evaluate a lispy expression and return result */
lval* eval_string(lenv* e, const char* input) {
    mpc_result_t r;
    if (mpc_parse("<test>", input, Lispy, &r)) {
        lval* x = lval_read(r.output);
        mpc_ast_delete(r.output);
        return lval_eval(e, x);
    }
    return lval_err("Parse error");
}

/* Test suite for basic arithmetic */
void test_addition(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(+ 1 2 3)");
    PT_ASSERT(result->type == LVAL_LONG);
    PT_ASSERT((long)result->num == 6);
    lval_del(result);

    lenv_del(e);
}

void test_subtraction(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(- 10 3 2)");
    PT_ASSERT(result->type == LVAL_LONG);
    PT_ASSERT((long)result->num == 5);
    lval_del(result);

    lenv_del(e);
}

void test_multiplication(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(* 2 3 4)");
    PT_ASSERT(result->type == LVAL_LONG);
    PT_ASSERT((long)result->num == 24);
    lval_del(result);

    lenv_del(e);
}

void test_division(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(/ 20 4)");
    PT_ASSERT(result->type == LVAL_FLOAT);
    PT_ASSERT(result->num == 5.0);
    lval_del(result);

    lenv_del(e);
}

void suite_arithmetic(void) {
    pt_add_test(test_addition, "Test Addition", "Arithmetic");
    pt_add_test(test_subtraction, "Test Subtraction", "Arithmetic");
    pt_add_test(test_multiplication, "Test Multiplication", "Arithmetic");
    pt_add_test(test_division, "Test Division", "Arithmetic");
}

/* Test suite for list operations */
void test_list(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(list 1 2 3)");
    PT_ASSERT(result->type == LVAL_QEXPR);
    PT_ASSERT(result->count == 3);
    lval_del(result);

    lenv_del(e);
}

void test_head(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(head {1 2 3})");
    PT_ASSERT(result->type == LVAL_QEXPR);
    PT_ASSERT(result->count == 1);
    lval_del(result);

    lenv_del(e);
}

void test_tail(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(tail {1 2 3})");
    PT_ASSERT(result->type == LVAL_QEXPR);
    PT_ASSERT(result->count == 2);
    lval_del(result);

    lenv_del(e);
}

void test_join(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(join {1 2} {3 4})");
    PT_ASSERT(result->type == LVAL_QEXPR);
    PT_ASSERT(result->count == 4);
    lval_del(result);

    lenv_del(e);
}

void suite_lists(void) {
    pt_add_test(test_list, "Test List", "Lists");
    pt_add_test(test_head, "Test Head", "Lists");
    pt_add_test(test_tail, "Test Tail", "Lists");
    pt_add_test(test_join, "Test Join", "Lists");
}

/* Test suite for conditionals */
void test_if_true(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(if true {1} {0})");
    PT_ASSERT(result->type == LVAL_LONG);
    PT_ASSERT((long)result->num == 1);
    lval_del(result);

    lenv_del(e);
}

void test_if_false(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(if false {1} {0})");
    PT_ASSERT(result->type == LVAL_LONG);
    PT_ASSERT((long)result->num == 0);
    lval_del(result);

    lenv_del(e);
}

void test_equality(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(eq 1 1)");
    PT_ASSERT(result->type == LVAL_BOOL);
    PT_ASSERT((int)result->num == 1);
    lval_del(result);

    result = eval_string(e, "(eq 1 2)");
    PT_ASSERT(result->type == LVAL_BOOL);
    PT_ASSERT((int)result->num == 0);
    lval_del(result);

    lenv_del(e);
}

void suite_conditionals(void) {
    pt_add_test(test_if_true, "Test If True", "Conditionals");
    pt_add_test(test_if_false, "Test If False", "Conditionals");
    pt_add_test(test_equality, "Test Equality", "Conditionals");
}

/* Test suite for type casting */
void test_int_cast(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(int 3.7)");
    PT_ASSERT(result->type == LVAL_LONG);
    PT_ASSERT((long)result->num == 3);
    lval_del(result);

    result = eval_string(e, "(int \"42\")");
    PT_ASSERT(result->type == LVAL_LONG);
    PT_ASSERT((long)result->num == 42);
    lval_del(result);

    lenv_del(e);
}

void test_float_cast(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(float 3)");
    PT_ASSERT(result->type == LVAL_FLOAT);
    PT_ASSERT(result->num == 3.0);
    lval_del(result);

    lenv_del(e);
}

void test_bool_cast(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    lval* result = eval_string(e, "(bool 0)");
    PT_ASSERT(result->type == LVAL_BOOL);
    PT_ASSERT((int)result->num == 0);
    lval_del(result);

    result = eval_string(e, "(bool 1)");
    PT_ASSERT(result->type == LVAL_BOOL);
    PT_ASSERT((int)result->num == 1);
    lval_del(result);

    lenv_del(e);
}

void suite_casting(void) {
    pt_add_test(test_int_cast, "Test Int Cast", "Casting");
    pt_add_test(test_float_cast, "Test Float Cast", "Casting");
    pt_add_test(test_bool_cast, "Test Bool Cast", "Casting");
}

/* Test suite for user-defined types */
void test_deftype(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    /* Define a type and create instance */
    eval_string(e, "(deftype {Point} {x y})");
    lval* result = eval_string(e, "(new {Point} 10 20)");
    PT_ASSERT(result->type == LVAL_UVAL);
    PT_ASSERT(strcmp(result->type_name, "Point") == 0);
    lval_del(result);

    lenv_del(e);
}

void test_get_field(void) {
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    eval_string(e, "(deftype {Point} {x y})");
    eval_string(e, "(def {p} (new {Point} 10 20))");

    lval* result = eval_string(e, "(get p {x})");
    PT_ASSERT(result->type == LVAL_LONG);
    PT_ASSERT((long)result->num == 10);
    lval_del(result);

    result = eval_string(e, "(get p {y})");
    PT_ASSERT(result->type == LVAL_LONG);
    PT_ASSERT((long)result->num == 20);
    lval_del(result);

    lenv_del(e);
}

void suite_user_types(void) {
    pt_add_test(test_deftype, "Test Deftype", "User Types");
    pt_add_test(test_get_field, "Test Get Field", "User Types");
}

/* Initialize parsers - must be called before tests */
void init_parsers(void) {
    Number  = mpc_new("number");
    Bool    = mpc_new("bool");
    String  = mpc_new("string");
    Symbol  = mpc_new("symbol");
    Comment = mpc_new("comment");
    Sexpr   = mpc_new("sexpr");
    Qexpr   = mpc_new("qexpr");
    Expr    = mpc_new("expr");
    Lispy   = mpc_new("lispy");

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
}

void cleanup_parsers(void) {
    mpc_cleanup(9, Number, Symbol, Bool, String, Comment, Sexpr, Qexpr, Expr, Lispy);
}

int main(int argc, char** argv) {
    init_parsers();

    pt_add_suite(suite_arithmetic);
    pt_add_suite(suite_lists);
    pt_add_suite(suite_conditionals);
    pt_add_suite(suite_casting);
    pt_add_suite(suite_user_types);

    int result = pt_run();

    cleanup_parsers();
    return result;
}
