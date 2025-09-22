# Conch
"Conch" Programming Language Reference Manual
Basic Syntax:

Commands are typically in the format: COMMAND [ARG1] [ARG2].

Variables in functions: ARG0, ARG1.

Variables in threads: X, Y, Z, with cached counterparts XC, YC, ZC. All variables and their caches are 0 by default and do not require initialization. Custom variable names are illegal.

Cycle Cost Model:

Cycles are one of the metrics the player is incentivized to reduce.

Each instruction has its own cycle cost.

Operations that access or alter variables cost BaseCost + VarCount × (number of distinct thread variables the op touches).

BaseCost is 1 cycle for most instructions. Touching fewer thread variables keeps the multiplier low, so leaning on literals or cached values is a great optimization target.

Example: copying a literal into X costs 1 + VarCount (only X is touched). Copying X from Y touches two distinct thread variables, so it costs 1 + VarCount × 2.

Inline functions still save cycles because they reduce how many ops you run.

Example (with three thread variables available, so VarCount = 3):

ADD Y 3          // 1 (ADD) + 3 × 1 thread var (Y) = 4
SET X Y          // 1 (SET) + 3 × 2 thread vars (X and Y) = 7
Total: 11

Inline version:

SET X ADD Y 3    // Single ADD op touching X and Y = 1 + 3 × 2 = 7

Commands:
Assignment and Control:

SET [VAR] [VAL]
Cycles: 1 + VarCount × (distinct thread vars touched; destination always counts, and sources count if they are thread vars)
Sets the variable to value, shifting current value to cache.

SWP [VAR]
Cycles: 1 + VarCount (touches one thread variable and its cache)
Swaps variable with cache.
Inline example: SWP SET X 1.

Flow Control:

IF [CONDITION]
Cycles: 1 + cost of condition
Executes block if condition is true.

IFN [CONDITION]
Cycles: 1 + cost of condition
Executes block if condition is false.

LOOP
Cycles: 0
Sets marker that REDO returns to.

REDO [VAL]
Cycles: 1 + cost of condition
Decrements [VAL], loops back if not zero.
If no value is provided, loop forever (must exit with RET or JUMP).

Math Instructions:

Cost: 1 + VarCount × number of distinct thread variables used in the operation

Operating on one thread variable alongside a literal = 1 + VarCount.
Reusing the same thread variable multiple times still counts as a single distinct touch, so tricks like squaring `X` via `MUL X X` pay only one VarCount multiplier.

ADD [VAR] [VAL]

SUB [VAR] [VAL]

MULT [VAR] [VAL]

DIVI [VAR] [VAL]

Comparisons:

[VAR1] EQL [VAR2]

[VAR1] GTR [VAR2]

[VAR1] LSR [VAR2]

Cycles: 1 + VarCount × (distinct thread vars compared)

Communication:

SEND [DST] [VAR]
Cycles: 1 + VarCount (touches a single thread variable)

LSTN [DST] [VAR]
Cycles: 1

Jumps & Flow Control:

JUMP [LABEL]
Cycles: 1

NOOP
Cycles: 1
No operation (used for timing/sync)

Increments and Logic:

INCR [VAR]
Cycles: 1 + VarCount

DECR [VAR]
Cycles: 1 + VarCount

AND [VAR] [VAL]

OR [VAR] [VAL]

XOR [VAR] [VAL]

NOT [VAR]

All: Cycles = 1 + VarCount × (distinct thread vars touched by the op)

Return and Stack:

RET [VAR]
Cycles: 1

POP [ARG]
Cycles: 1
Retrieves next value from LIST in [ARG].

AT [IDX]
Cycles: 1
Gets value of a LIST at index.

Variable Usage Impact:

Each distinct thread variable an op touches multiplies VarCount into the cost.

One thread variable: 1 + VarCount

Two thread variables: 1 + VarCount × 2

Three thread variables: 1 + VarCount × 3

Inline Optimization:

Inline commands don't count toward 3 ops per line, and they can keep costs down by avoiding extra ops that would each add their own BaseCost.

Revisiting the example with VarCount = 3:

ADD Y 3          // 1 (ADD) + 3 × 1 = 4
SET X Y          // 1 (SET) + 3 × 2 = 7
Total: 11


Inline version:

SET X ADD Y 3    // Single op = 1 + 3 × 2 = 7

Optimization bonus:

SET X MUL X X    // Only touches X, so 1 + 3 × 1 = 4 even though X is used twice

Thread and Function Specifics:
Functions:

ARG Usage: Use ARG0, ARG1, etc. (no cache).

LIST Usage: Use LIST0, LIST1, etc.

Can be iterated, accessed via AT, or popped.

Mixed Input Types: Functions may accept both ARG and LIST.

Threads:

Use X, Y, Z (with XC, YC, ZC)

May access list variables like LISTX, LISTY in advanced puzzles.

Rules and Efficiency:
Rule Name: Inline Operation Efficiency:

Combining multiple operations inline reduces variable preparation overhead.

Inline forms let you pay the VarCount multiplier once per combined op instead of spreading it across separate commands, and reusing the same register keeps the distinct-variable multiplier low.

Syntax restrictions apply; not all commands can be combined inline.

Other Notes:

IF, IFN, LOOP, REDO, JUMP can use EQL, GTR, LSR comparisons.

NOOP is for timing/sync in advanced use.

ARG values can be used as conditions.

E.g., IF ARG0 is true if list is not empty.

Comparing to cached values is faster.

E.g., IF X GTR: (uses XC as implied).

Code Blocks:

Defined by indentation.

No ENDIF, ENDLOOP, etc.

Comments:

Not allowed in real code.
(Training examples may include them, but they are invalid in-game.)
