const rac = require("ripple-address-codec");

/**
 * Convert an r-address into the 20-byte AccountID hex used in hook parameters.
 * @input {Account} account Account to convert
 */

const account = process.env.account;
console.log(rac.decodeAccountID(account).toString("hex").toUpperCase());
