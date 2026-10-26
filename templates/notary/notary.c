/**
 * Notary.c - Collect signatures for a multi-signed transaction without blocking the account's Sequence number.
 *
 * Original author: Richard Holland
 *
 * @invoke ttINVOKE
 */
#include "hookapi.h"

// An Invoke carries the proposal in a Blob field and the proposal is kept in one hook state entry,
// whose value may be at most 256 bytes.
#define MAX_BLOB_SIZE 256
// Working buffers need room for the fields added before emission (EmitDetails and Fee).
#define WORK_SIZE 512
// LastLedgerSequence of a proposal must be at least this far ahead of the current ledger.
#define MINIMUM_FUTURE_LEDGER 60
// Signer weights live under state keys ending in nibble 1..MAX_SIGNERS (8), the proposal under nibble F.
#define MAX_SIGNERS 8

#define HEADER "Notary: accepted, waiting for other signers. id: "
#define HEADER_LEN (sizeof(HEADER) - 1)

// Delete the proposal (nibble F) and every recorded signer weight. `key` is any 32 byte id of the proposal.
static inline __attribute__((always_inline)) void purge(uint8_t* key)
{
    for (uint8_t i = 1; GUARD(MAX_SIGNERS + 1), i <= MAX_SIGNERS + 1; ++i)
    {
        key[31] = (key[31] & 0xF0U) + (i <= MAX_SIGNERS ? i : 0x0FU);
        state_set(0, 0, key, 32);
    }
}

// Replace (or insert) one serialized field in the transaction. The result is written to `oth`, then
// `cur` and `oth` swap roles so that `cur` always holds the latest version of the transaction.
#define EMPLACE(field, data, data_len, msg, code)                                    \
    {                                                                                \
        int64_t r = sto_emplace(oth, WORK_SIZE, cur, tx_len, data, data_len, field); \
        if (r <= 0)                                                                  \
            rollback(SBUF(msg), code);                                                \
        tx_len = r;                                                                  \
        BUFFER_SWAP(cur, oth);                                                       \
    }

int64_t hook(uint32_t reserved)
{
    _g(1, 1); // every hook must import the guard function and use it at least once

    // This hook only speaks Invoke. Anything else passes untouched.
    if (otxn_type() != ttINVOKE)
        DONE("Notary: not an Invoke, passing");

    // Reserve the one emitted transaction up front (emission happens once a quorum is reached).
    if (etxn_reserve(1) != 1)
        NOPE("Notary: could not reserve an emit slot.");

    uint8_t hook_accid[20];
    if (hook_account(SBUF(hook_accid)) != 20)
        NOPE("Notary: could not read the hook account.");

    uint8_t sender[20];
    if (otxn_field(SBUF(sender), sfAccount) != 20)
        NOPE("Notary: could not read the sender account.");

    // Hooks also run for outgoing transactions of the account they are installed on.
    int equal = 0;
    BUFFER_EQUAL(equal, hook_accid, sender, 20);
    if (equal)
        DONE("Notary: outgoing transaction, passing");

    // ---- Which mode? ------------------------------------------------------------------------------------
    // Mode 1 (propose): sfBlob holds the serialized unsigned transaction.
    // Mode 2 (endorse): hook parameter "id" holds the 32 byte id of an already proposed transaction.
    uint8_t tx_blob[WORK_SIZE];
    int64_t tx_len = 0;

    // otxn_field returns a Blob field with its variable-length prefix (1 byte below 193 bytes of data,
    // 2 bytes above). Read it with room to spare, then strip the prefix.
    uint8_t raw[MAX_BLOB_SIZE + 3];
    int64_t raw_len = otxn_field(SBUF(raw), sfBlob);
    int64_t blob_len = 0;
    uint8_t* blob = raw;
    if (raw_len > 0)
    {
        if (raw[0] <= 192)
        {
            blob_len = raw[0];
            blob = raw + 1;
        }
        else if (raw[0] <= 240)
        {
            blob_len = 193 + (((int64_t)raw[0] - 193) << 8) + raw[1];
            blob = raw + 2;
        }
        if (blob_len <= 0 || blob_len > MAX_BLOB_SIZE || (blob - raw) + blob_len != raw_len)
            NOPE("Notary: the Blob is malformed or larger than 256 bytes.");
    }
    else if (raw_len != DOESNT_EXIST)
        NOPE("Notary: the Blob is malformed or larger than 256 bytes.");

    uint8_t id[32];
    int64_t id_len = otxn_param(SBUF(id), "id", 2);
    if (id_len != DOESNT_EXIST && id_len != 32)
        NOPE("Notary: parameter id must be exactly 32 bytes.");

    if (blob_len == 0 && id_len != 32)
        DONE("Notary: neither a Blob nor an id, nothing to do");
    if (blob_len > 0 && id_len == 32)
        NOPE("Notary: send either a Blob or an id, not both.");

    if (id_len == 32)
    {
        // Endorsement: the proposal lives in state under the id, with the last nibble set to F.
        id[31] = (id[31] & 0xF0U) + 0x0FU;
        tx_len = state(tx_blob, WORK_SIZE, SBUF(id));
        if (tx_len < 0)
            NOPE("Notary: the id does not match a proposed transaction.");

        // The proposal may have expired in the meantime.
        int64_t lls = sto_subfield(tx_blob, tx_len, sfLastLedgerSequence);
        if (lls < 0 || SUB_LENGTH(lls) != 4 ||
            UINT32_FROM_BUF(tx_blob + SUB_OFFSET(lls)) < ledger_seq())
        {
            purge(id);
            DONE("Notary: proposal expired (LastLedgerSequence passed) and was erased");
        }
    }

    // ---- Is the sender a signer? ----------------------------------------------------------------------------
    // Describe the signer list of the hook account with a keylet, then put that ledger object in a slot.
    uint8_t keylet[34];
    if (util_keylet(SBUF(keylet), KEYLET_SIGNERS, SBUF(hook_accid), 0, 0, 0, 0) != 34)
        NOPE("Notary: could not generate the signer list keylet.");

    int64_t slot_no = slot_set(SBUF(keylet), 0);
    if (slot_no < 0)
        NOPE("Notary: the hook account has no signer list.");

    // Quorum: the total weight needed to emit.
    int64_t result = slot_subfield(slot_no, sfSignerQuorum, 0);
    if (result < 0)
        NOPE("Notary: could not find SignerQuorum.");
    // Not called `buf`: the UINTxx_TO_BUF macros declare their own `buf` and would shadow it.
    uint8_t num[4];
    if (slot(SBUF(num), result) != 4)
        NOPE("Notary: could not read SignerQuorum.");
    uint32_t signer_quorum = UINT32_FROM_BUF(num);

    // Signer entries: reuse the slot, the parent object is no longer needed.
    if (slot_subfield(slot_no, sfSignerEntries, slot_no) < 0)
        NOPE("Notary: could not find SignerEntries.");
    int64_t signer_count = slot_count(slot_no);
    if (signer_count < 0)
        NOPE("Notary: could not count SignerEntries.");
    if (signer_count > MAX_SIGNERS)
        NOPE("Notary: more signers than this hook supports (8).");

    // Walk the entries; `found` becomes the 1-based index of the sender, `signer_weight` its weight.
    int64_t subslot = 0;
    uint8_t found = 0;
    uint16_t signer_weight = 0;
    for (int i = 0; GUARD(MAX_SIGNERS), i < signer_count; ++i)
    {
        subslot = slot_subarray(slot_no, i, subslot);
        if (subslot < 0)
            NOPE("Notary: could not read a signer entry.");

        result = slot_subfield(subslot, sfAccount, 0);
        if (result < 0)
            NOPE("Notary: could not read a signer account.");
        uint8_t signer_account[20];
        if (slot(SBUF(signer_account), result) != 20)
            NOPE("Notary: could not read a signer account.");

        result = slot_subfield(subslot, sfSignerWeight, 0);
        if (result < 0)
            NOPE("Notary: could not read a signer weight.");
        if (slot(num, 2, result) != 2)
            NOPE("Notary: could not read a signer weight.");

        int same = 0;
        BUFFER_EQUAL_GUARD(same, signer_account, 20, sender, 20, MAX_SIGNERS);
        if (same)
        {
            found = i + 1;
            signer_weight = UINT16_FROM_BUF(num);
            break;
        }
    }
    if (!found)
        NOPE("Notary: your account is not on the signer list.");

    // ---- Mode 1: record the proposal ---------------------------------------------------------------------------
    if (blob_len > 0)
    {
        // A transaction must at least have a TransactionType.
        if (sto_subfield(blob, blob_len, sfTransactionType) < 0)
            NOPE("Notary: the Blob is not a serialized transaction.");

        // LastLedgerSequence must leave the other signers time to endorse.
        int64_t lls = sto_subfield(blob, blob_len, sfLastLedgerSequence);
        if (lls < 0 || SUB_LENGTH(lls) != 4 ||
            UINT32_FROM_BUF(blob + SUB_OFFSET(lls)) < ledger_seq() + MINIMUM_FUTURE_LEDGER)
            NOPE("Notary: LastLedgerSequence must be at least 60 ledgers ahead.");

        // The id of the proposal is the hash of the blob; the state key adds F as last nibble.
        if (util_sha512h(SBUF(id), blob, blob_len) != 32)
            NOPE("Notary: could not hash the Blob.");
        id[31] = (id[31] & 0xF0U) + 0x0FU;

        if (state_set(blob, blob_len, SBUF(id)) < 0)
            NOPE("Notary: could not store the proposal in hook state.");
    }
    // Mode 1 now continues as an endorsement of its own proposal.

    // ---- Record this signer's weight --------------------------------------------------------------------------
    id[31] = (id[31] & 0xF0U) + found;
    UINT16_TO_BUF(num, signer_weight);
    if (state_set(num, 2, SBUF(id)) != 2)
        NOPE("Notary: could not record the signature in hook state.");

    // ---- Quorum? --------------------------------------------------------------------------------------------
    uint32_t total = 0;
    for (uint8_t i = 1; GUARD(MAX_SIGNERS), i <= MAX_SIGNERS; ++i)
    {
        id[31] = (id[31] & 0xF0U) + i;
        if (state(num, 2, SBUF(id)) == 2)
            total += UINT16_FROM_BUF(num);
    }
    id[31] = (id[31] & 0xF0U) + 0x0FU;

    if (total < signer_quorum)
    {
        // Return the id as uppercase hex in the hook result so it can be copied from the IDE log.
        uint8_t msg[HEADER_LEN + 64];
        uint8_t* ptr = msg;
        for (int i = 0; GUARD(HEADER_LEN), i < HEADER_LEN; ++i)
            *ptr++ = HEADER[i];
        for (int i = 0; GUARD(32), i < 32; ++i)
        {
            uint8_t hi = id[i] >> 4U;
            uint8_t lo = id[i] & 0xFU;
            *ptr++ = hi + (hi > 9 ? 'A' - 10 : '0');
            *ptr++ = lo + (lo > 9 ? 'A' - 10 : '0');
        }
        accept(SBUF(msg), 0);
    }

    // ---- Quorum reached: prepare the transaction and emit it ----------------------------------------------
    // Both modes load the proposal from state, so there is one code path from here on.
    tx_len = state(tx_blob, WORK_SIZE, SBUF(id));
    if (tx_len < 0)
        NOPE("Notary: the proposal is missing from hook state.");

    int64_t lls = sto_subfield(tx_blob, tx_len, sfLastLedgerSequence);
    if (lls < 0 || SUB_LENGTH(lls) != 4)
        NOPE("Notary: the proposal has no LastLedgerSequence.");
    uint32_t lls_old = UINT32_FROM_BUF(tx_blob + SUB_OFFSET(lls));
    if (lls_old < ledger_seq())
        NOPE("Notary: the proposal expired before it could be emitted.");

    // Delete everything from state first; a later rollback restores it.
    purge(id);

    // `cur` is the latest version of the transaction, `oth` the buffer the next step writes to.
    uint8_t scratch[WORK_SIZE];
    uint8_t* cur = tx_blob;
    uint8_t* oth = scratch;

    // 1. Remove sfSigners and sfTxnSignature if present (erase fails when the field is absent, which is fine).
    result = sto_erase(oth, WORK_SIZE, cur, tx_len, sfSigners);
    if (result > 0)
    {
        tx_len = result;
        BUFFER_SWAP(cur, oth);
    }
    result = sto_erase(oth, WORK_SIZE, cur, tx_len, sfTxnSignature);
    if (result > 0)
    {
        tx_len = result;
        BUFFER_SWAP(cur, oth);
    }

    // 2. A hook-emitted transaction has Sequence 0 and a signing key of 33 zero bytes.
    uint8_t field[35];
    CLEARBUF(field);
    field[0] = 0x24U; // sfSequence header, then 4 zero bytes
    EMPLACE(sfSequence, field, 5, "Notary: could not set sfSequence.", 20);
    field[0] = 0x73U; // sfSigningPubKey header, then length 0x21 (33) and 33 zero bytes
    field[1] = 0x21U;
    EMPLACE(sfSigningPubKey, field, 35, "Notary: could not set sfSigningPubKey.", 22);

    // 3. The emission window: FirstLedgerSequence is the next ledger, LastLedgerSequence at most 4 later.
    uint32_t fls = ledger_seq() + 1;
    field[0] = 0x20U;
    field[1] = 0x1AU; // sfFirstLedgerSequence
    UINT32_TO_BUF(field + 2, fls);
    EMPLACE(sfFirstLedgerSequence, field, 6, "Notary: could not set sfFirstLedgerSequence.", 23);

    uint32_t lls_new = fls + 4;
    if (lls_old > lls_new)
    {
        field[1] = 0x1BU; // sfLastLedgerSequence
        UINT32_TO_BUF(field + 2, lls_new);
        EMPLACE(sfLastLedgerSequence, field, 6, "Notary: could not set sfLastLedgerSequence.", 24);
    }

    // 4. Emit details prove the transaction was emitted by this hook.
    uint8_t emitdet[138];
    result = etxn_details(SBUF(emitdet));
    if (result < 0)
        NOPE("Notary: could not generate EmitDetails.");
    EMPLACE(sfEmitDetails, emitdet, result, "Notary: could not set sfEmitDetails.", 25);

    // 5. Fee for an emitted transaction is computed by the hook API from the final transaction.
    //    sfFee is 0x68 followed by the 8 byte native amount (top two bits 01 mark "native, positive").
    int64_t fee = etxn_fee_base(cur, tx_len);
    if (fee < 0)
        NOPE("Notary: could not compute the emit fee.");
    field[0] = 0x68U;
    field[1] = 0x40U + ((fee >> 56) & 0x3FU);
    field[2] = (fee >> 48) & 0xFFU;
    field[3] = (fee >> 40) & 0xFFU;
    field[4] = (fee >> 32) & 0xFFU;
    field[5] = (fee >> 24) & 0xFFU;
    field[6] = (fee >> 16) & 0xFFU;
    field[7] = (fee >> 8) & 0xFFU;
    field[8] = fee & 0xFFU;
    EMPLACE(sfFee, field, 9, "Notary: could not set sfFee.", 26);

    uint8_t emithash[32];
    int64_t erv = emit(SBUF(emithash), cur, tx_len);
    if (erv < 0)
        NOPE("Notary: quorum reached but emission failed, the proposed transaction is malformed.");

    DONE("Notary: emitted multisigned txn");
    return 0;
}
