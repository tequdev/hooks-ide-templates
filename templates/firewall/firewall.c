#include "hookapi.h"

/**
 * Firewall: rejects incoming payments from accounts listed in the state of a
 * blacklist hook (blacklist.c) installed on another account.
 *
 * @invoke ttPAYMENT
 * @field blaccid AccountID of the account running the blacklist hook (20-byte hex)
 * @field blns Hook namespace of the blacklist hook (32-byte hex)
 */

int64_t hook(uint32_t reserved)
{
    _g(1, 1);

    // the firewall filters payments only
    if (otxn_type() != ttPAYMENT)
        accept(SBUF("Firewall: not a payment, passing."), 1);

    uint8_t sender[20], me[20];
    if (otxn_field(SBUF(sender), sfAccount) != 20)
        rollback(SBUF("Firewall: could not read the sender."), 2);

    // our own outgoing payments are never blocked
    hook_account(SBUF(me));
    if (BUFFER_EQUAL_20(sender, me))
        accept(SBUF("Firewall: outgoing payment, passing."), 3);

    uint8_t blacklist_ns[32];
    if (hook_param(SBUF(blacklist_ns), "blns", 4) != 32)
        rollback(SBUF("Firewall: \"blns\" parameter must be 32 bytes."), 4);

    uint8_t blacklist_accid[20];
    if (hook_param(SBUF(blacklist_accid), "blaccid", 7) != 20)
        rollback(SBUF("Firewall: \"blaccid\" parameter must be 20 bytes."), 5);

    // same key shape as blacklist.c: account id right-aligned in 32 zero bytes
    uint8_t key[32] = {0};
    BYTES20_TO_BUF(key + 12, sender);

    uint8_t status[1];
    int64_t lookup = state_foreign(SBUF(status), SBUF(key), SBUF(blacklist_ns), SBUF(blacklist_accid));

    if (lookup == DOESNT_EXIST)
        accept(SBUF("Firewall: sender is not blacklisted, allowing."), 6);

    if (lookup == 1 && status[0] == 1)
        rollback(SBUF("Firewall: blocking transaction from blacklisted account."), 7);

    // INVALID_ACCOUNT (blacklist account missing), a malformed value, or any other error
    TRACEVAR(lookup);
    rollback(SBUF("Firewall: could not read the blacklist."), 8);

    return 0;
}
