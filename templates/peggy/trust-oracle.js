import { XrplClient } from "https://esm.sh/xrpl-client?bundle";

const lib = require("xrpl-accountlib");
const keypairs = require("ripple-keypairs");

/**
 * @input {Account.secret} low_secret Low Account
 * @input {Account} high_account High Account
 */

const { low_secret, high_account } = process.env

const low_keypair = lib.derive.familySeed(low_secret);
const low_account = keypairs.deriveAddress(low_keypair.keypair.publicKey);

console.log(low_account);
console.log(high_account);

const client = new XrplClient('wss://hooks-testnet-v3.xrpl-labs.com');

const main = async () => {
    const { account_data } = await client.send({ command: 'account_info', 'account': low_account });
    if (!account_data) {
        console.log('Account not found.');
        client.close();
        return;
    }

    const tx = {
        "TransactionType": "TrustSet",
        "Account": low_account,
        "Fee": "12",
        "Flags": 262144,
        "LimitAmount": {
            "currency": "USD",
            "issuer": high_account,
            "value": "37" // $/XRP
        },
        "Sequence": account_data.Sequence,
        "NetworkID": "21338"
    };

    const { signedTransaction } = lib.sign(tx, low_keypair);
    const submit = await client.send({ command: 'submit', 'tx_blob': signedTransaction });
    console.log(submit);

    console.log('Shutting down...');
    client.close();
};

main();