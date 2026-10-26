const lib = require("xrpl-accountlib");


/**
 * @input {Account.secret} admin_secret Admin Account
 */

// Alice
const { admin_secret } = process.env;
const admin_keypair = lib.derive.familySeed(admin_secret);

console.log(admin_keypair.keypair.publicKey);