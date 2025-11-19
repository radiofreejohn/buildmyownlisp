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
* help system - `(help print)` or `(help)` to list all builtins

## TODO

### Data Structures
* hash maps - `(dict {a 1 b 2})`, `(dict-get d {a})`, `(dict-set d {c} 3)`
* sets - `(set 1 2 3)`, `(set-add s 4)`, `(set-contains s 2)`

### Standard Library
* prelude file - auto-load a `prelude.lspy` with common functions (map, filter, fold, range)
* higher-order functions - built-in map, filter, reduce for better performance

### Error Handling
* error returns - `(def {result err} (safe-div 10 0))` with explicit (value, err) tuples
* try block - `(try {expr})` catches error returns in nested calls
* stack traces - show call stack on errors

### I/O Operations
* file I/O - `(read-file "path")`, `(write-file "path" content)`
* stdin/stdout - `(read-line)`, `(getchar)`

### Module System
* namespaces - `(namespace math {def pi 3.14})`, `(math/pi)`
* selective imports - `(from "utils.lspy" import {add sub})`

### Language Features
* pattern matching - `(match x {0 "zero"} {1 "one"} {_ "other"})`
* tail call optimization - prevent stack overflow on recursion
* lazy evaluation - `(lazy {expensive-computation})`

### REPL Improvements
* tab completion - complete builtins and defined symbols
* persistent history - save command history to `~/.lispy_history`

### Debugging
* breakpoints - `(break)` pauses execution
* step mode - step through evaluation one expression at a time
