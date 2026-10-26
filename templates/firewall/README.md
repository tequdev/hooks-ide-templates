# Firewall

Two cooperating hooks. `blacklist.c` keeps a list of blocked accounts in its own hook state. `firewall.c` runs on a protected account and rejects incoming payments from any account on that list. It reads the list with `state_foreign`, so the protected account never holds a copy of it.

Both hooks need to know about each other before deployment, so that information is passed as [install-time parameters](https://docs.xahau.tools).

## How it works

- The blacklist admin sends an **Invoke** to the blacklist account with two HookParameters: `acc` (the 20-byte AccountID to change) and `op` (`01` block, `00` unblock).
- The transaction signature already proves who sent it, so `blacklist.c` only checks that the sender is the `admin` parameter (or the hook account itself when `admin` is not set).
- The state key is the AccountID right-aligned in 32 zero bytes; the value is a single byte `01`. Unblocking deletes the entry.
- `firewall.c` builds the same key for the payment sender and looks it up in the blacklist account's state, using the blacklist namespace.

## Try it in the Hooks Builder

1. Make sure you have at least 4 accounts: Alice (admin), Bob (blacklist hook), Carol (firewall hook) and Carlos (suspect).
2. Run `decode.js` for Alice and for Bob to get their hex AccountIDs. Parameter values must be hex.
3. Compile `blacklist.c` and deploy it to Bob with HookOn **Invoke**. Set `admin` to Alice's hex AccountID, or leave it empty to let Bob manage his own list. Note the HookNamespace.
4. Run `decode.js` for Carlos as well.
5. Compile `firewall.c` and deploy it to Carol with HookOn **Payment** and two parameters: `blaccid` = Bob's hex AccountID, `blns` = the blacklist hook namespace (32-byte hex).
6. In the transaction pane send an **Invoke** from Alice to Bob with HookParameters `acc` = Carlos's hex AccountID and `op` = `01`. The debug stream shows "Blacklist: blocked account."
7. Send a **Payment** from Carlos to Carol with the debug stream filtered on Carol. It is rejected:
   - "Firewall: blocking transaction from blacklisted account." in the debug stream
   - "[tecHOOK_REJECTED] Rejected by hook on sending or receiving account." in the development log
8. Send the Invoke again with `op` = `00`. The payment from Carlos now succeeds.

## What changed from the original

The original `blacklist.c` accepted a signed-memo triple (payload, signature, public key), verified it with `util_verify` and kept a replay sequence number in state. Invoke transactions make all of that unnecessary: the ledger already authenticates the sender, and the transaction sequence number already prevents replays. The `key.js` and `block-unblock.js` scripts are gone for the same reason. `firewall.c` keeps its logic but now only looks at payments, checks the result of the foreign state lookup properly, and no longer lets outgoing payments be blocked.
