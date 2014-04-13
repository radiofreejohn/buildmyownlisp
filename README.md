buildmyownlisp
==============

My continuation of https://github.com/orangeduck/BuildYourOwnLisp
Read it online: http://www.buildyourownlisp.com/

## Differences:
* bool type, incompatible with number type
* all string typed lvals share the same string space
* strdup instead of malloc/strcpy
* floating point type

## TODO
* user defined types
* type casting
* internal string representation that's not raw char array
* use ptest https://github.com/orangeduck/ptest
* fraction representation of numbers

## Crazy Ideas
* threads
* multi-line REPL
* change arrays to linked list
* a builtin eval called debug that's very verbose
