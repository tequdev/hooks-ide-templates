# Starter

The simplest possible Hook: it accepts every transaction and logs that it ran.

## Try it in the Hooks Builder

1. In the Develop pane, compile `starter.c`.
2. In the Deploy pane, deploy it to Alice (HookOn: Payment is preselected).
3. In the Test pane, send a Payment from Alice to Bob.
4. Open the debug stream for Alice and look for `Starter: called`; the
   development log shows the hook's accept message `Starter: accepted`.

## What to look at

- `_g(1, 1)` is the first statement: every hook must call the guard at least
  once, and loops must guard every iteration.
- `accept()` (and `rollback()`) never return. The message and code you pass
  are stored in the transaction's `HookExecutions` metadata, which is how the
  IDE and explorers show why a hook accepted or rejected a transaction.
- `TRACESTR` writes to the node's trace log, which the debug stream shows.

Changing the message and compiling with different optimization levels is a
good first exercise.
