#include "hookapi.h"

/**
 * Blacklist: keeps a list of blocked accounts in this account's hook state.
 * Other accounts read it with state_foreign (see firewall.c).
 *
 * The list is managed with Invoke transactions. The transaction signature
 * already proves who the sender is, so no extra signature or replay counter
 * is needed: the hook only compares the sender with the admin.
 *
 * Invoke HookParameters: acc = 20-byte AccountID, op = 01 (block) or 00 (unblock).
 *
 * @invoke ttINVOKE
 * @field [admin] Account allowed to manage the list (20-byte AccountID hex, optional; defaults to the hook account)
 */

int64_t hook(uint32_t reserved)
{
    _g(1, 1);

    if (otxn_type() != ttINVOKE)
        accept(SBUF("Blacklist: not an Invoke, passing."), 1);

    uint8_t sender[20];
    if (otxn_field(SBUF(sender), sfAccount) != 20)
        rollback(SBUF("Blacklist: could not read the sender."), 2);

    // authorization: the sender must be the admin parameter, or the hook account itself
    uint8_t admin[20];
    int64_t alen = hook_param(SBUF(admin), "admin", 5);
    if (alen == DOESNT_EXIST)
        hook_account(SBUF(admin));
    else if (alen != 20)
        rollback(SBUF("Blacklist: \"admin\" parameter must be 20 bytes."), 3);

    if (!BUFFER_EQUAL_20(sender, admin))
        rollback(SBUF("Blacklist: sender is not the admin."), 4);

    // instruction, taken from the HookParameters of the Invoke
    uint8_t acc[20];
    if (otxn_param(SBUF(acc), "acc", 3) != 20)
        rollback(SBUF("Blacklist: \"acc\" parameter must be a 20-byte AccountID."), 5);

    uint8_t op[1];
    int64_t olen = otxn_param(SBUF(op), "op", 2);
    if (olen != 1)
        rollback(SBUF("Blacklist: \"op\" parameter missing (1 byte: 01 block, 00 unblock)."), 6);
    if (op[0] > 1)
        rollback(SBUF("Blacklist: \"op\" must be 01 (block) or 00 (unblock)."), 7);

    // state key: the account id right-aligned in 32 zero bytes (firewall.c builds the same key)
    uint8_t key[32] = {0};
    BYTES20_TO_BUF(key + 12, acc);

    if (op[0] == 1)
    {
        uint8_t blocked[1] = {1};
        if (state_set(SBUF(blocked), SBUF(key)) < 0)
            rollback(SBUF("Blacklist: could not store the account."), 8);
        accept(SBUF("Blacklist: blocked account."), 9);
    }

    // a zero-length value deletes the entry
    if (state_set(0, 0, SBUF(key)) < 0)
        rollback(SBUF("Blacklist: could not remove the account."), 10);
    accept(SBUF("Blacklist: unblocked account."), 11);

    return 0;
}
