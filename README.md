# Interpreter

An interpreter for a small imperative language, written from scratch in C++.
A student project: my first interpreter, made at the Kazakhstan branch of MSU.

The language has integer arithmetic, variables, `if`/`else`, `while`, `goto`
with labels, and functions that can call each other and themselves.

## Build

Needs `make` and a C++11 compiler. The overflow checks use the
`__builtin_*_overflow` intrinsics, so `g++` or `clang++` is expected.

```bash
make
```

The binary lands in `bin/interpreter`. `make fastcreate` does the same thing
and is kept because older notes use that name.

## Running a program

Pass the file as an argument:

```bash
bin/interpreter tests/test1.txt
```

or feed it on standard input:

```bash
bin/interpreter < tests/test1.txt
```

With no file and a terminal on standard input, the interpreter prompts with
`> ` for each line and runs the whole program once you close the input with
`Ctrl-D`.

`--verbose` adds a debug trace: the lexems of every line, the postfix form, and
the evaluation stack step by step.

```bash
bin/interpreter --verbose tests/test5.txt
```

Exit codes: `0` when the program ran, `1` when the program has an error, `2`
when the command line itself is wrong.

## Output

The language has no print statement. Instead, **every line that produces a
value prints it**, prefixed with `>>>>>>`:

```
def main()
	2 + 3 * 4
	return 0
fed
```

```
>>>>>>14
>>>>>>0
```

Lines that produce nothing — a label, `@`, a call to a function that returns
nothing — print an empty line.

## The language

### Structure of a program

A program is a list of functions. Execution starts at `main`, which takes no
parameters and must be present. Functions may be defined in any order: one can
call a function that is defined further down the file.

```
def main()
	...
	return 0
fed
```

The value `main` returns is printed like any other value; it is not the exit
code of the process.

### Values

One type only: a signed 32 bit integer. There are no strings, no arrays and no
floating point. An operation whose result does not fit is an error, not a
silent wrap-around.

### Comments

Everything after `#` up to the end of the line is ignored.

```
# how many times to repeat
n := 3	# a note after the code
```

### Variables

A variable comes into being on assignment with `:=` and lives until the
function returns. Names are made of letters, digits and `_`, and cannot start
with a digit. Reading a variable that was never assigned is an error.

Variables are local to the function they are used in. There are no globals, and
arguments are passed by value: assigning to a parameter does not touch the
caller's variable.

```
x := 1
y := x + 2
x := y := 10	# assignment is right associative, both become 10
```

### Operators

| Operator | Meaning |
|---|---|
| `+` `-` `*` `/` `%` | add, subtract, multiply, integer divide, remainder |
| `-` (in front of a value) | negation, as in `-5` or `-(a + b)` |
| `<` `<=` `>` `>=` `==` `!=` | comparisons, giving `1` or `0` |
| `and` `or` | logical, giving `1` or `0` |
| `&` `\|` `^` | bitwise and, or, exclusive or |
| `<<` `>>` | bit shift, the count has to be between 0 and 31 |
| `:=` | assignment |

Division truncates towards zero: `-7 / 2` is `-3` and `-7 % 3` is `-1`.

### Precedence

From the tightest to the loosest. Everything except `:=` groups to the left.

| Level | Operators |
|---|---|
| 1 | `-` in front of a value |
| 2 | `*` `/` `%` |
| 3 | `+` `-` |
| 4 | `<<` `>>` |
| 5 | `<` `<=` `>` `>=` |
| 6 | `==` `!=` |
| 7 | `&` |
| 8 | `^` |
| 9 | `\|` |
| 10 | `and` |
| 11 | `or` |
| 12 | `:=` (right associative) |

So `1 + 2 << 3` is `(1 + 2) << 3`, and `0 or 1 and 0` is `0 or (1 and 0)`.
Brackets override all of it.

### if, else and while

The condition ends with a colon, the block ends with `@`. Any non-zero value
counts as true.

```
if i > 0:
	s := s + i
else:
	s := 0
@

while i < 4:
	s := s + i
	i := i + 1
@
```

There is no `elif`, and no `break` or `continue`.

### goto and labels

A label is a name followed by a colon on a line of its own. `goto` jumps to it.

```
def main()
	i := 0
	s := 0
L:
	i := i + 1
	s := s + i
	if i < 3:
		goto L
	@
	s
	return 0
fed
```

A label belongs to the function it is written in. Two functions may use the
same label name, and a `goto` cannot jump into a function that is not running.

### Functions

A definition starts with `def` and ends with `fed`. Parameters are names
separated by commas.

```
def sum(a, b)
	return a + b
fed

def fact(n)
	if n < 2:
		return 1
	@
	return n * fact(n - 1)
fed
```

`return` with a value gives that value back; `return` on its own returns
nothing, and using such a call as a value is an error. A function may call
itself, and two functions may call each other.

```
def main()
	sum(2, 3)
	x := fact(5) + 1
	return 0
fed
```

## Errors

An error stops the program, prints the line it happened on to standard error,
and gives exit code 1.

```
problems in 2 string:
Division by zero
```

| Message | When |
|---|---|
| Invalid syntax | a character that belongs to no lexem |
| Brackets are not balanced | a bracket without its pair |
| Not enough operands for the operator | as in `1 +` |
| The line leaves more than one value behind | as in `1 2` |
| The left side of := is not a variable | as in `1 := 2` |
| Use of a variable that has no value yet | reading a name never assigned |
| The program needs a function main | no `main` in the file |
| def and fed are not balanced | a definition not closed, or `fed` alone |
| Bad function name after def | `def` not followed by a name |
| Bad parameter list in the function definition | as in `def f(a, a)` or `def f(1)` |
| Function was called with a wrong number of arguments | as in `sum(1)` |
| A function that returns nothing was used as a value | using a bare `return` result |
| No colon after the condition of if, else or while | missing `:` |
| if, else or while is left unclosed | missing `@` |
| Incorrect use of if, else or while | `else` or `@` with no `if` above |
| fed inside an if or while that is still open | a block crossing the end of a function |
| Unknown label, or a label that belongs to another function | a `goto` that cannot land |
| Division by zero | `/` or `%` by zero |
| The result does not fit in a 32 bit integer | overflow of a literal or of an operation |
| A shift count has to be between 0 and 31 | as in `1 << 64` |

## Tests

```bash
make check
```

Every program in `tests/` has to succeed and print what `tests/expected/*.out`
records; every program in `tests/errors/` has to fail with the message in
`tests/expected/*.err`. The command line is checked too.

To watch one program run:

```bash
make test5
```

After a change that alters the output on purpose:

```bash
make update-tests
```

then read `git diff` over `tests/expected/` before keeping it.

## Not there yet

- a real interactive mode, where a line runs as soon as it is typed
- a way to print on purpose, instead of every line echoing its value
- `elif`, `break` and `continue`
