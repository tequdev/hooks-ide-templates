const rac = require("ripple-address-codec");

/**
 * @input {Account} a Account
 */

const a = process.env.a,
      b = rac.decodeAccountID(a),
      h = b.toString('hex').toUpperCase();

console.log(h);