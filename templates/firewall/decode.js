const rac = require("ripple-address-codec");

/**
 * Converts an r-address to the 20-byte AccountID hex used in hook parameters.
 * @input {Account} account Account to convert
 */

console.log(rac.decodeAccountID(process.env.account).toString("hex").toUpperCase());
