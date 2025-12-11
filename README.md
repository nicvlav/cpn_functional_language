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

**Comparison:** `max`, `min`

**Scoped Variables:**
```lisp
( (let (x 1)) ( (let (y 2)) (add x y) ) )
 
```
Returns 3. Supports nested scoping through `let` blocks.
