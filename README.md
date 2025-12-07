# CiLisp - A Cambridge Polish Notation Functional Language

A simple functional programming language interpreter that uses Cambridge Polish Notation (CPN) for mathematical expressions. Built with Lex and Yacc as a fun exploration of language design and compiler construction.

## What is Cambridge Polish Notation?

CPN is a prefix notation where operators come before their operands, all wrapped in parentheses. For example:
- Standard notation: `1 + 2`
- CPN notation: `(add 1 2)`
- Nested: `(add 1 (mult 2 3))` equals `1 + (2 * 3)`

## Project Overview

This interpreter is built using Unix compiler tools:
- **Lex** ([cilisp.l](cilisp.l)) - Lexical analyzer for tokenizing input
- **Yacc** ([cilisp.y](cilisp.y)) - Parser generator for syntax analysis and evaluation
- **GCC** - Compiles the generated C code into the final interpreter

The language supports various mathematical operations including `add`, `sub`, `mult`, `div`, `abs`, `neg`, `sqrt`, `pow`, `log`, `exp`, and more.

## Building the Interpreter

### Prerequisites
- Unix-like environment (Linux/macOS)
- GCC compiler
- Lex/Flex
- Yacc/Bison

**Note:** This project has only been tested on a Unix system with GCC. Your mileage may vary on other platforms.

### Build Steps

1. Clone the repository
2. Make the build script executable:
   ```bash
   chmod 700 run
   ```
3. Run the build script:
   ```bash
   ./run
   ```

The build script performs the following:
- Generates parser code from [cilisp.y](cilisp.y) using Yacc
- Generates lexer code from [cilisp.l](cilisp.l) using Lex
- Concatenates and compiles all C files into the `cilisp` executable

## Usage

The interpreter supports two modes of operation:

### Interactive Mode (REPL)
Run the interpreter without arguments to enter an interactive prompt:
```bash
./cilisp
```
Then enter expressions one at a time:
```
> (add 1 2 3)
> (neg (abs -42))
> quit
```

### File Mode
Provide a /cilisp file containing CiLisp expressions:
```bash
./cilisp input_file.cilisp
```

The interpreter will evaluate each expression in the file sequentially.

### Example Expressions

```
(add 1 2 3)                    # Returns 6
(neg (abs -42))                # Returns -42
(neg (add 1 2 3 (abs -5)))     # Returns -11
```

## Limitations

- Currently only tested on Unix systems with GCC
- Limited error handling and recovery
- No variable assignment or function definition (purely expression-based)
- Fixed set of built-in mathematical functions
