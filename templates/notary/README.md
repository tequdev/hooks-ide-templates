# Notary

Notary hook collects signatures for multi-sign transactions. It has two modes of operation:
1. Attach a proposed transaction to a memo and send it to the hook account
2. Endorse an already proposed transaction by using its unique ID as invoice ID and sending a 1 drop payment to the hook.

It relies on the signer list on the account the hook is running on. Only accounts on this list can propose and endorse multisign transactions through this hook.

to test:
- make sure Hooks Builder has at least 3 accounts: Alice, Bob and Carol
- set up SignerListSet on Alice's account w/ SignerQuorum 2 to Bob & Carol
- compile notary.c and deploy it to Alice account
- open debug stream filtered on Alice and run propose.js; it asks for 2 parameters:
	1. notary account is where the hook is running, i.e. Alice
	2. proposer is the account authorized to propose a new transaction, e.g. Bob; the account secret is used to sign the transaction
- see the hook debug stream, especially that it succeeds and produces and invoice ID (hex-quoted 32 bytes) for the other signers
- save the invoice ID somewhere
- run sign.js, as the other authorized user; it asks for 3 parameters:
	1. notary account is Alice, as above
	2. approver is the other authorized account, i.e. Carol
	3. invoice ID is the value saved above
- see in the hook debug stream (and account balances) that the hook emited the multi-signed transaction

Modifying the example for more signers is left as an exercise for the reader.
