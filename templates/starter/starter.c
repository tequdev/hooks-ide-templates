/**
 * Starter - the smallest useful Hook: it accepts every transaction that
 * touches the account it is installed on and leaves a line in the debug
 * stream so you can see it run.
 *
 * @name Starter
 * @on ttPAYMENT
 */
#include "hookapi.h"

int64_t hook(uint32_t reserved)
{
    // Every hook (and cbak) starts with the guard: the body runs at most once.
    _g(1, 1);

    // trace() output shows up in the IDE's debug stream for the hook account.
    TRACESTR("Starter: called");

    // accept() never returns; the originating transaction is applied.
    // The message and code are recorded in the transaction metadata.
    accept(SBUF("Starter: accepted"), 0);

    // Unreachable, but the C compiler wants a return.
    return 0;
}
