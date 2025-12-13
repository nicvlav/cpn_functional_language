# CiLisp - Cambridge Polish Notation Functional Language

A functional language interpreter using prefix notation. Built with Lex/Yacc.

## Notation

CPN uses prefix operators in parentheses(CPN):
- `(add 1 2)` → 3
- `(mult 2 (add 3 4))` → 14

## Building

```bash
make
```

Requires: GCC, Lex/Flex, Yacc/Bison

## Usage

**Interactive (stdin):**
```bash
./cilisp
> (add 1 2 3)
Integer : 6
> quit
```

**File input:**
```bash
./cilisp input.cilisp
```

## Features

**Arithmetic:** `add`, `sub`, `mult`, `div`, `remainder`, `neg`, `abs`

**Exponential/Logarithmic:** `exp`, `exp2`, `pow`, `log`

**Roots:** `sqrt`, `cbrt`, `hypot`

**Comparison:** `max`, `min`, `equal`, `less`, `greater`

**Conditionals:** `cond` - ternary operator for branching

**I/O:** `read`, `print`, `rand`

**Symbol Types:**
```lisp
> ( (let (int x 5.7)) x )
Precision loss on int cast from 5.7 to 5
Integer : 5
```
Declare typed symbols: `int`, `double`

**Scoped Variables:**
```lisp
> ( (let (x 1)) ( (let (y 2)) (add x y) ) )
Integer : 3
```
Supports nested scoping through `let` blocks.
