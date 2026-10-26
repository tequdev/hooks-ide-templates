import { XrplClient } from "https://esm.sh/xrpl-client?bundle";

const lib = require("xrpl-accountlib");
const bin = require("ripple-binary-codec");
const keypairs = require("ripple-keypairs");


/**
 * @input {Account.secret} admin_secret Admin Account
 * @input {Account.secret} blacklist_secret Blacklist Account
 * @input {Account} suspect_account Suspect Account
 * @input {text} exec_mode Exec mode (Block=1, Unblock=0)
*/
const { admin_secret, blacklist_secret, suspect_account, exec_mode } = process.env

const admin_keypair = lib.derive.familySeed(admin_secret);
      
const blacklist_keypair = lib.derive.familySeed(blacklist_secret);
const blacklist_account = keypairs.deriveAddress(blacklist_keypair.keypair.publicKey);

if ((exec_mode !== "1") && (exec_mode !== "0"))
{
    console.error("unknown execution mode \"" + exec_mode + "\" - must be either 0 or 1");
    throw "invalid argument";
}

const flag = (exec_mode === "1") ? 1 : 0;

const client = new XrplClient('wss://hooks-testnet-v3.xrpl-labs.com');

const main = async () => {    
    let blacklist_instruction = bin.encode(
    {
        Sequence: Math.floor(Date.now()/1000),
        Flags: flag,
        Template: [
            {
                Account: suspect_account
            }
        ],
        NetworkID: "21338"
    });

    const { account_data } = await client.send({ command: 'account_info', 'account': blacklist_account });
    if (!account_data) {
        console.log('Blacklist account not found.');
        client.close();
        return;
    }

    console.log("sequence", account_data.Sequence);

    const tx = {
        Account: blacklist_account,
        TransactionType: 'AccountSet',
        Fee: '1200000',
        Memos: [
            {
                Memo: {
                    MemoData: blacklist_instruction,
                    MemoFormat: "signed/payload+1",
                    MemoType: "liteacc/payment"
                }
            },
            {
                Memo:{
                    MemoData: keypairs.sign(blacklist_instruction, admin_keypair.keypair.privateKey),
                    MemoFormat: "signed/signature+1",
                    MemoType: "liteacc/signature"
                }
            },
            {
                Memo:{
                    MemoData: admin_keypair.keypair.publicKey,
                    MemoFormat: "signed/publickey+1",
                    MemoType: "liteacc/publickey"
                }
            }
        ],
        Sequence: account_data.Sequence,
        NetworkID: "21338"
    };

    hexlify_memos(tx);

    // console.log(JSON.stringify(tx.Memos));

    const {signedTransaction} = lib.sign(tx, blacklist_keypair);
    const submit = await client.send({ command: 'submit', 'tx_blob': signedTransaction });
    console.log(submit);
    if (!submit || (submit.engine_result_code !== 0))
    {
	console.error("submit failed");
    }

    console.log('Shutting down...');
    client.close();
};

function hexlify_memos(x)
{
    if (!("Memos" in x))
        return;

    for (let y = 0; y < x["Memos"].length; ++y)
    {
        let Memo = x["Memos"][y]["Memo"];
        let Fields = ["MemoFormat", "MemoType", "MemoData"];
        for (let z = 0; z < Fields.length; ++z)
        {
            if (Fields[z] in Memo)
            {
                let u = Memo[Fields[z]].toUpperCase()
                if (u.match(/^[0-9A-F]+$/))
                {
                    Memo[Fields[z]] = u;
                    continue;
                }

		let v = Memo[Fields[z]], q = "";
		for (let i = 0; i < v.length; ++i)
		{
		    q += Number(v.charCodeAt(i)).toString(16).padStart(2, '0');
		}
		
                Memo[Fields[z]] = q.toUpperCase();
            }
        }
    }
}

main()