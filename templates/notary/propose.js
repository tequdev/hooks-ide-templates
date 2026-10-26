const bin = require("ripple-binary-codec");

/**
 * Encodes the unsigned transaction a signer proposes to the Notary hook.
 * Paste the printed hex into the Blob field of an Invoke sent to the notary account.
 *
 * @input {Account} notary_account Notary (hook) account
 * @input {Account} destination Destination of the proposed payment
 * @input {text} [amount=100] Amount in drops
 * @input {text} last_ledger_sequence LastLedgerSequence (current ledger + at least 60)
 */

const { notary_account, destination, amount, last_ledger_sequence } = process.env;

const proposed_tx = {
    TransactionType: 'Payment',
    Account: notary_account,
    Destination: destination,
    Amount: String(amount),
    LastLedgerSequence: Number(last_ledger_sequence),
    Fee: '0',
    Sequence: 0
};

console.log('Blob (paste into the Invoke):');
console.log(bin.encode(proposed_tx));
