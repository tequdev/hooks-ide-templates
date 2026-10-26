# Notary

Notary collects signatures for a multi-signed transaction without blocking the Sequence number of the account.
It relies on the signer list of the account the hook is installed on. Only accounts on that list can propose or
endorse a transaction, and the transaction is emitted by the hook once the signer weights reach the quorum.

It speaks `Invoke` and has two modes:

1. **Propose.** The Invoke carries a `Blob` field holding the serialized unsigned transaction. The hook stores it
   and logs a 32-byte id.
2. **Endorse.** The Invoke carries a hook parameter named `id` with the 32-byte id of a proposed transaction.

Send either a Blob or an id, not both. The Blob can be at most 256 bytes, because the proposal is kept in one hook
state entry. The proposal's `LastLedgerSequence` must be at least 60 ledgers ahead when it is proposed. Up to 8
signers are supported. The proposer's own weight counts as the first signature.

## Try it in Hooks Builder

1. Make sure Hooks Builder has three accounts: Alice (the notary), Bob and Carol (the signers).
2. Send a `SignerListSet` from Alice with `SignerQuorum` 2 and Bob and Carol as signers with weight 1 each.
   The transaction pane has a `SignerListSet` template.
3. Compile `notary.c` and deploy it to Alice with HookOn `Invoke` (the `@invoke` tag preselects it).
4. Run `propose.js`. Fill in Alice as the notary account, a destination, an amount in drops and a
   `LastLedgerSequence` (current ledger + at least 60). It prints the hex Blob.
5. Open the debug stream filtered on Alice. Send an `Invoke` from Bob to Alice and paste the Blob into its `Blob`
   field.
6. The hook result reads `Notary: accepted, waiting for other signers. id: <64 hex>`. Copy the id.
7. Send an `Invoke` from Carol to Alice with a hook parameter named `id` whose value is that id.
8. The quorum is reached, the hook result reads `Notary: emitted multisigned txn`, and Alice's payment is emitted.

Modifying the example for more signers is left as an exercise for the reader.

## What changed from the original

- The proposal travels in the `Blob` field of an Invoke instead of a memo of a 1 drop payment.
- Endorsing uses an `id` hook parameter on an Invoke instead of the InvoiceID of a 1 drop payment, so `sign.js` is
  gone: the transaction pane does it.
- `propose.js` is now a pure encoder. It no longer signs or submits anything.
- The Blob is limited to 256 bytes (it was 4096) because of the hook state value limit.
- The fee field is written by hand, since the `ENCODE_DROPS` macros are no longer in the bundled headers.
