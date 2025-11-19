buildmyownlisp
==============

My continuation of https://github.com/orangeduck/BuildYourOwnLisp
Read it online: http://www.buildyourownlisp.com/

## Differences:
* bool type, incompatible with number type
* all string typed lvals share the same string space
* strdup instead of malloc/strcpy
* floating point type
* change "cell" array to linked list

## Features
* user defined types - `(deftype {Point} {x y})`, `(new {Point} 10 20)`, `(get p {x})`
* type casting - `(int 3.7)`, `(float 3)`, `(bool 1)`
* internal string representation (lstr) with length prefix
* ptest testing framework with 24 tests
* fraction representation - `(frac 3 4)`, `(numer ...)`, `(denom ...)`
* threads - `(spawn {expr})`, `(wait thread-id)`
* multi-line REPL - continues reading on unclosed brackets
* debug builtin - `(debug {expr})` for verbose step-by-step evaluation
