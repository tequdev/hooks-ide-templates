# Peggy

Peggy is an automated, over-collateralised stable coin. Send XAH to the hook account and it opens a vault and sends back PUSD, a USD-denominated token issued by the hook account. Send PUSD back and the hook releases the matching XAH from your vault.

- **Vault.** Each user (account plus SourceTag) has a vault holding the PUSD issued and the XAH locked. A new vault is issued PUSD at 150% collateralization.
- **Oracle.** The USD/XAH price is the limit of a USD trust line between two oracle accounts. The hook reads the `LowLimit` of that line, so changing the price is a `TrustSet` from the oracle's low account.
- **Takeover.** If the price falls until a vault is below 120% collateralization, anyone can top it up and take it over. They send a Payment whose `InvoiceID` is the vault id (20 byte account, 4 byte tag, 8 zero bytes). The new owner is rewarded with the vault.
- **User trust line.** The user needs a USD trust line to the hook account with a limit of at least 10B.

## Try it in Hooks Builder

1. Create accounts Alice (the user), Carol (the hook and PUSD issuer), Carlos and Charlie (the oracle accounts).
2. Run `decode.js` twice, once for Carlos and once for Charlie. It prints the 20-byte hex AccountID. The oracle accounts must be named by their numeric AccountID: the account with the lower hex value is `oracle_lo`, the other is `oracle_hi`. If Carlos is higher than Charlie, swap their roles below.
3. Set the price. In the transaction pane send a `TrustSet` from the low oracle account:
   ```json
   {
     "TransactionType": "TrustSet",
     "Account": "<oracle_lo address>",
     "LimitAmount": { "currency": "USD", "issuer": "<oracle_hi address>", "value": "37" },
     "Flags": 131072
   }
   ```
   The limit (37) is the USD price of 1 XAH. Flag 131072 is `tfSetNoRipple`.
4. Let Alice trust Carol. Send a `TrustSet` from Alice:
   ```json
   {
     "TransactionType": "TrustSet",
     "Account": "<Alice address>",
     "LimitAmount": { "currency": "USD", "issuer": "<Carol address>", "value": "10000000000" }
   }
   ```
5. Compile `peggy.c` and deploy it on Carol (HookOn `Payment` for incoming transactions and HookCanEmit `Payment` are preselected) with the parameters `oracle_lo` and `oracle_hi` set to the two hex ids from step 2.
6. Send a Payment of 100 XAH from Alice to Carol.
7. Open the debug stream filtered on Carol. The hook accepts with "Sent you PUSD!" and emits a Payment of about 2466.66 USD from Carol to Alice. You can see the emitted payment in the explorer too.
8. To redeem, send PUSD from Alice back to Carol, for example 1000 USD issued by Carol. The hook emits the matching XAH.

## What changed from the original

- The old `PREPARE_PAYMENT_SIMPLE*` macros no longer exist. The two emitted Payments (XAH and PUSD) are built from Transaction Builder templates, and the PUSD amount is written with `float_sto` and `sfAmount`.
- The hook ignores everything but Payments, checks every API return value, and requires `oracle_lo` and `oracle_hi` to be exactly 20 bytes.
- `trust-user.js` and `trust-oracle.js` are gone. They only built `TrustSet` transactions, which you now send from the transaction pane as shown above. `decode.js` is kept.
- The vault and price maths are unchanged.
