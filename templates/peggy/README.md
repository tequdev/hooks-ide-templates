# Peggy

Peggy hook provides an automated way to issue and redeem an over-collateralised XRP-backed stablecoin.

Sending XRP to it creates a vault and sends back stablecoin, which can be redeemed against the vault to get back the original XRP.

The exchange rate between XRP and stablecoin is the limit set on a [trustline](https://xrpl.org/trust-lines-and-issuing.html) established between two special oracle accounts.

If the value of XRP falls too far, the vault gets under a minimum collateralization threshold. This means anyone who wants to top up the vault above the threshold can come along and take it over.

to test:
- make sure Hooks Builder has at least 5 accounts: Alice, Bob, Carol, Carlos and Charlie
- run decode.js, twice, to convert Carlos and Charlie accounts to binary form; save the values somewhere
- if the Carlos account number is numerically higher than Charlie, switch the accounts (either re-import them, or just switch their names in the following text)
- set up trust limit for the stablecoin user, by running trust-user.js; the script requires 2 parameters:
	1. the user account (Alice) sends the TrustSet transaction, so that the script requires its private key
	2. the hook account (Carol) is set up as the trusted issuer
- set up trust limit on the oracle, by running trust-oracle.js; the script requires 2 parameters:
	1. the low account (Carlos) sends the TrustSet transaction, so that the script requires its private key
	2. the high account (Charlie) is set up as the trusted issuer
- compile peggy.c and deploy it to Carol account, with 2 parameters:
        1. "oracle_lo" set Carlos binary account
	2. and "oracle_hi" set to the Charlie binary account
- set up payment transaction from Alice to Carol
- open debug stream filtered on Carol
- run the transaction, see it succeed:
	- "Flow:TRC rippleCredit: Carol -> Alice : before=-66669133.31781069/USD/1 amount=2466.666666092349/USD/Carol after=-66671599.98447678/USD/1" in the debug stream
	- "[tesSUCCESS]" in the development log
