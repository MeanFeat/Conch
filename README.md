# Conch
"Conch" Programming Language Reference Manual
Basic Syntax:

Commands are typically in the format: COMMAND [ARG1] [ARG2].

Variables in functions: ARG0, ARG1.

Variables in threads: X, Y, Z, with cached counterparts XC, YC, ZC. All variables and their caches are 0 by default and do not require initialization. Custom variable names are illegal.

Cycle Cost Model:

Cycles are one of the metrics the player is incentivized to reduce.

Each instruction has its own cycle cost.

Operations that access or alter variables cost as many cycles as there are variables used in the thread (VarCount).

Example: Setting a variable costs [Command Cost] + [VarCount].

Inline functions can save cycles by not requiring another VarCount.

Example: SET X ADD Y 5

1 cycle for SET, 1 for ADD, and only one VarCount for the line.

Commands:
Assignment and Control:

SET [VAR] [VAL]
Cycles: VarCount
Sets the variable to value, shifting current value to cache.

SWP [VAR]
Cycles: 1
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

Cost: VarCount × number of variables used in the operation

Operating on one variable (with its own cache) = 1 cycle.

ADD [VAR] [VAL]

SUB [VAR] [VAL]

MULT [VAR] [VAL]

DIVI [VAR] [VAL]

Comparisons:

[VAR1] EQL [VAR2]

[VAR1] GRTR [VAR2]

[VAR1] LSSR [VAR2]

Cycles: VarCount of each var accessed

Communication:

SEND [DST] [VAR]
Cycles: VarCount

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
Cycles: VarCount

DECR [VAR]
Cycles: VarCount

AND [VAR] [VAL]

OR [VAR] [VAL]

XOR [VAR] [VAL]

NOT [VAR]

All: Cycles = VarCount

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

Cost depends on VarCount:

One variable: 1 cycle

Two variables: 2 cycles

Three variables: 3 cycles

Inline Optimization:

Inline commands don't count toward 3 cycles per line rule.

Executed at lower cycle cost.

Example:

ADD Y 3          // 1 (ADD) + 2 (VarCount) = 3
SET X Y          // 1 (SET) + 2 (VarCount) = 3
Total: 6


Inline version:

SET X ADD Y 3    // 1 (SET) + 1 (ADD) + 2 (VarCount) = 4

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

Only one VarCount applies to the entire line.

Syntax restrictions apply; not all commands can be combined inline.

Other Notes:

IF, IFN, LOOP, REDO, JUMP can use EQL, GRTR, LSSR comparisons.

NOOP is for timing/sync in advanced use.

ARG values can be used as conditions.

E.g., IF ARG0 is true if list is not empty.

Comparing to cached values is faster.

E.g., IF X GRTR: (uses XC as implied).

Code Blocks:

Defined by indentation.

No ENDIF, ENDLOOP, etc.

Comments:

Not allowed in real code.
(Training examples may include them, but they are invalid in-game.)
