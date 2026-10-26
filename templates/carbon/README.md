# Carbon

Carbon sends a percentage of every outgoing payment to a beneficiary account. When the hook account pays more than 0.1 XAH, the hook emits a second payment of 1% of the amount to the beneficiary. Smaller payments and IOU payments send a default 1000 drops. Incoming payments pass through untouched.

## Test it in the Hooks Builder

1. Make sure you have at least three accounts: Alice (the hook account), Bob and Carol (the beneficiary).
2. Run `decode.js` with Carol's address to get her 20-byte AccountID as hex (parameter values must be hex).
3. Compile `carbon.c`.
4. Deploy it to Alice with HookOn = Payment and the parameter `carbon_acc` = Carol's hex AccountID.
5. From the transaction pane, send a Payment from Alice to Bob of more than 0.1 XAH.
6. Open the debug stream filtered on Carol. You will see the hook trace and the emitted payment to Carol arriving a ledger later.

When the emitted payment is applied, the hook's `cbak` function runs. It is only an example and not essential.

## What changed from the original

- The beneficiary is a HookParameter (`carbon_acc`) instead of a hard-coded r-address.
- The emitted payment is built with a Transaction Builder template instead of the removed `PREPARE_PAYMENT_SIMPLE` macro.
- The 1% is computed with integer arithmetic instead of floating point.
