import { XrplClient } from "https://esm.sh/xrpl-client?bundle";

const lib = require("xrpl-accountlib");
const keypairs = require("ripple-keypairs");

/**
 * @input {Account} notary_account Notary Account
 * @input {Account.secret} approver_secret Approver Account
 * @input invoice_id Invoice Id
 */

const { notary_account, approver_secret, invoice_id } = process.env

const approver_keypair = lib.derive.familySeed(approver_secret);
const approver_account = keypairs.deriveAddress(approver_keypair.keypair.publicKey);

const client = new XrplClient('wss://hooks-testnet-v3.xrpl-labs.com');

const main = async (proposal) => {
    console.log("proposal", proposal);
    try {
        const { account_data } = await client.send({ command: 'account_info', 'account': approver_account });
        if (!account_data) {
            console.log('Approver account not found.');
            client.close();
            return;
        }

        console.log("sequence", account_data.Sequence);
        
        const tx = {
            Account: approver_account,
            TransactionType: "Payment",
            Amount: "1",
            Destination: notary_account,
            Fee: "1000000",
            InvoiceID: proposal,
            Sequence: account_data.Sequence,
            NetworkID: "21338"
        };

        const {signedTransaction} = lib.sign(tx, approver_keypair);

        const submit = await client.send({ command: 'submit', 'tx_blob': signedTransaction });
        console.log(submit);
    } catch (err) {
        error(err)
    }

    console.log('Shutting down...');
    client.close();
};

main(invoice_id);