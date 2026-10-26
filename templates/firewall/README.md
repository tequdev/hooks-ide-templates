# Firewall

Firewall is an example of cooperating hooks: firewall hook instances filter incoming payments, based on shared state maintained by blacklist hook.

To cooperate, the hooks need information about each other, which must be obtained before deploying them, because it's passed to them as [install-time parameters](https://xrpl-hooks.readme.io/docs/parameters).

to test:
- make sure Hooks Builder has at least 4 accounts: Alice, Bob, Carol and Carlos
- run key.js to get a public key of the blacklist admin account, e.g. Alice; it asks for that account's secret key
	- verify the script is using it just to derive the public key
	- if you don't believe the script, or its dependencies, do the conversion in a trusted environment
- compile blacklist.c and deploy it to Bob account
	- for ttPAYMENT + ttACCOUNT_SET
	- noting the hook namespace and saving it somewhere
        - with "admin" param set to Alice public key obtained above
- run block-unblock.js to block an account; the script requires 4 parameters:
	1. admin is the account authorized to sign blacklist updates, i.e. Alice; the script needs its private key
	2. blacklist is the account sending AccountSet, i.e. Bob; its private key is used to sign that transaction
	3. suspect is the 3rd-party account to be blocked or unblocked, i.e. Carlos; its private key is not needed
	4. execution mode should be set to 1, to block
- run decode.js to convert Bob account to binary form (the hook could do that, but it's extra computation that doesn't need to be replicated on chain) and save it somewhere
	- enter Bob account as the script parameter
- compile firewall.c and deploy it to Carol account, with 2 parameters:
	1. "blaccid" parameter set to Bob binary account
	2. and "blns" parameter set to the blacklist hook namespace
- set up payment transaction from Carlos to Carol
- open debug stream filtered on Carol
- run the transaction, see it rejected:
	- "Firewall: Blocking transaction from blacklisted account." in the debug stream
	- "[tecHOOK_REJECTED] Rejected by hook on sending or receiving account." in the development log
- run block-unblock.js to unblock Carlos account; the parameters are mostly as before:
	1. admin is the account authorized to sign blacklist updates, i.e. Alice; the script needs its private key
	2. blacklist is the account sending AccountSet, i.e. Bob; its private key is used to sign that transaction
	3. suspect is the 3rd-party account to be blocked or unblocked, i.e. Carlos; its private key is not needed
	4. execution mode should be set to 0, to unblock
- repeat the payment transaction, see it succeed:
	- "Firewall: Allowing transaction." in the debug stream
	- "[tesSUCCESS] The transaction was applied. Only final in a validated ledger." in the development log
