# Carbon

Carbon hook sends a percentage of outgoing payments to a dedicated account.

The beneficiary account is hard-coded, so while the default version of carbon.c works, it works with an account that cannot be imported into Hooks Builder (unless you know its secret key). See later examples for how to parametrize a hook.

to test:
- make sure Hooks Builder has at least 3 accounts: Alice, Bob and Carol
- in carbon.c, change hardcoded beneficiary to Carol account
- compile carbon.c and deploy it to Alice account
- set up payment transaction from Alice to Bob
- open debug stream filtered on Carol
- run the transaction and see incoming payment

Other hook changes, such as removing the cbak function (which is included just as an example and not essential to the hook functionality), or fixing the hooks-raddr-conv-pure warning, are left as an exercise for the reader.
