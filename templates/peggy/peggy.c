/**
 * Peggy.c - An oracle based stable coin hook
 *
 * Author: Richard Holland
 * Date: 1 Mar 2021
 *
 * Send XAH to the hook account and receive PUSD (an over-collateralised USD
 * stable coin issued by the hook account). Send PUSD back to redeem the XAH.
 * The USD/XAH price is the limit of a USD trust line between two oracle
 * accounts.
 *
 * @on_incoming ttPAYMENT
 * @emit ttPAYMENT
 * @param oracle_lo Low oracle account (20-byte AccountID hex)
 * @param oracle_hi High oracle account (20-byte AccountID hex)
 **/

#include <stdint.h>
#include "hookapi.h"

// your vault starts at 150% collateralization
#define NEW_COLLATERALIZATION_NUMERATOR 2
#define NEW_COLLATERALIZATION_DENOMINATOR 3

// at 120% collateralization your vault may be taken over
#define LIQ_COLLATERALIZATION_NUMERATOR 5
#define LIQ_COLLATERALIZATION_DENOMINATOR 6

// Transaction Builder output: one emitted Payment template per direction.
// txn_xah pays XAH (native amount), txn_iou pays PUSD (issued amount).
// Both carry a DestinationTag and a SourceTag. Only one of them is used
// per execution.
// clang-format off
uint8_t txn_xah[248] =
{
/* size, upto, field name               */
/*    3,    0, tt = Payment             */   0x12U, 0x00U, 0x00U,
/*    5,    3, flags                    */   0x22U, 0x00U, 0x00U, 0x00U, 0x00U,
/*    5,    8, sequence                 */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,
/*    5,   13, destinationtag           */   0x2EU, 0x00U, 0x00U, 0x00U, 0x00U,
/*    5,   18, sourcetag                */   0x23U, 0x00U, 0x00U, 0x00U, 0x00U,
/*    6,   23, firstledgersequence      */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,
/*    6,   29, lastledgersequence       */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,
/*    9,   35, amount                   */   0x61U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
/*    9,   44, fee                      */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/*   35,   53, signingpubkey            */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*   22,   88, account                  */   0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*   22,  110, destination              */   0x83U, 0x14U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/*  116,  132, emit details             */
/*    0,  248,                          */
};

uint8_t txn_iou[288] =
{
/* size, upto, field name               */
/*    3,    0, tt = Payment             */   0x12U, 0x00U, 0x00U,
/*    5,    3, flags                    */   0x22U, 0x00U, 0x00U, 0x00U, 0x00U,
/*    5,    8, sequence                 */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,
/*    5,   13, destinationtag           */   0x2EU, 0x00U, 0x00U, 0x00U, 0x00U,
/*    5,   18, sourcetag                */   0x23U, 0x00U, 0x00U, 0x00U, 0x00U,
/*    6,   23, firstledgersequence      */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,
/*    6,   29, lastledgersequence       */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,
/*   49,   35, amount                   */   0x61U, 0xD4U, 0x83U, 0x8DU, 0x7EU, 0xA4U, 0xC6U, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x55U, 0x53U, 0x44U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/*    9,   84, fee                      */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/*   35,   93, signingpubkey            */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*   22,  128, account                  */   0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*   22,  150, destination              */   0x83U, 0x14U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/*  116,  172, emit details             */
/*    0,  288,                          */
};
// clang-format on

// TX BUILDER
#define FLAGS_OUT_XAH (txn_xah + 4U)
#define DTAG_OUT_XAH (txn_xah + 14U)
#define STAG_OUT_XAH (txn_xah + 19U)
#define FLS_OUT_XAH (txn_xah + 25U)
#define LLS_OUT_XAH (txn_xah + 31U)
#define AMOUNT_OUT_XAH (txn_xah + 36U)
#define FEE_OUT_XAH (txn_xah + 45U)
#define ACCOUNT_OUT_XAH (txn_xah + 90U)
#define DEST_OUT_XAH (txn_xah + 112U)
#define EMIT_OUT_XAH (txn_xah + 132U)

#define FLAGS_OUT_IOU (txn_iou + 4U)
#define DTAG_OUT_IOU (txn_iou + 14U)
#define STAG_OUT_IOU (txn_iou + 19U)
#define FLS_OUT_IOU (txn_iou + 25U)
#define LLS_OUT_IOU (txn_iou + 31U)
#define AMOUNT_OUT_IOU (txn_iou + 36U)
#define FEE_OUT_IOU (txn_iou + 85U)
#define ACCOUNT_OUT_IOU (txn_iou + 130U)
#define DEST_OUT_IOU (txn_iou + 152U)
#define EMIT_OUT_IOU (txn_iou + 172U)

#define COPY_20(ptr_to, ptr_from)                                              \
  do {                                                                         \
    unsigned char *buf_to = (unsigned char *)ptr_to;                           \
    unsigned char *buf_from = (unsigned char *)ptr_from;                       \
    *(uint64_t *)(buf_to + 0) = *(uint64_t *)(buf_from + 0);                   \
    *(uint64_t *)(buf_to + 8) = *(uint64_t *)(buf_from + 8);                   \
    *(uint32_t *)(buf_to + 16) = *(uint32_t *)(buf_from + 16);                 \
  } while (0)

#define SET_UINT32(ptr, value) *((uint32_t *)(ptr)) = FLIP_ENDIAN_32(value);

#define SET_NATIVE_AMOUNT(ptr, amount)                                         \
  do {                                                                         \
    uint8_t *b = (ptr);                                                        \
    *b++ = 0b01000000 + ((amount >> 56) & 0b00111111);                         \
    *b++ = (amount >> 48) & 0xFFU;                                             \
    *b++ = (amount >> 40) & 0xFFU;                                             \
    *b++ = (amount >> 32) & 0xFFU;                                             \
    *b++ = (amount >> 24) & 0xFFU;                                             \
    *b++ = (amount >> 16) & 0xFFU;                                             \
    *b++ = (amount >> 8) & 0xFFU;                                              \
    *b++ = (amount >> 0) & 0xFFU;                                              \
  } while (0)

#define SET_ACCOUNT(ptr_to, ptr_from)                                          \
  {                                                                            \
    COPY_20(ptr_to, ptr_from);                                                 \
  }

// Fills in the ledger sequence window, our account, the emit details and the
// fee. Call it last, after the amount, destination and tags are set.
#define PREPARE_TXN_XAH()                                                      \
  do {                                                                         \
    etxn_reserve(1);                                                           \
    uint32_t fls = (uint32_t)ledger_seq() + 1;                                 \
    SET_UINT32(FLS_OUT_XAH, fls);                                              \
    SET_UINT32(LLS_OUT_XAH, fls + 4);                                          \
    hook_account(ACCOUNT_OUT_XAH, 20);                                         \
    etxn_details(EMIT_OUT_XAH, 116U);                                          \
    int64_t fee = etxn_fee_base(SBUF(txn_xah));                                \
    SET_NATIVE_AMOUNT(FEE_OUT_XAH, fee);                                       \
    TRACEHEX(txn_xah);                                                         \
  } while (0)

#define PREPARE_TXN_IOU()                                                      \
  do {                                                                         \
    etxn_reserve(1);                                                           \
    uint32_t fls = (uint32_t)ledger_seq() + 1;                                 \
    SET_UINT32(FLS_OUT_IOU, fls);                                              \
    SET_UINT32(LLS_OUT_IOU, fls + 4);                                          \
    hook_account(ACCOUNT_OUT_IOU, 20);                                         \
    etxn_details(EMIT_OUT_IOU, 116U);                                          \
    int64_t fee = etxn_fee_base(SBUF(txn_iou));                                \
    SET_NATIVE_AMOUNT(FEE_OUT_IOU, fee);                                       \
    TRACEHEX(txn_iou);                                                         \
  } while (0)

int64_t hook(uint32_t reserved)
{
    _g(1, 1);

    // only payments are of interest; anything else passes through
    if (otxn_type() != ttPAYMENT)
        accept(SBUF("Peggy: not a payment"), 0);

    uint8_t currency[20] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 'U', 'S', 'D', 0,0,0,0,0};

    // get the account the hook is running on and the account that created the txn
    uint8_t hook_accid[20];
    hook_account(SBUF(hook_accid));

    uint8_t otxn_accid[20];
    int32_t otxn_accid_len = otxn_field(SBUF(otxn_accid), sfAccount);
    if (otxn_accid_len != 20)
        rollback(SBUF("Peggy: sfAccount field missing!!!"), 10);

    // get the source tag if any... negative means it wasn't provided
    int64_t source_tag = otxn_field(0,0, sfSourceTag);
    if (source_tag < 0)
        source_tag = 0xFFFFFFFFU;

    // compare the "From Account" (sfAccount) on the transaction with the account the hook is running on
    if (BUFFER_EQUAL_20(hook_accid, otxn_accid))
        accept(SBUF("Peggy: Outgoing transaction"), 20);

    // invoice id if present is used for taking over undercollateralized vaults
    // format: { 20 byte account id | 4 byte tag [FFFFFFFFU if absent] | 8 bytes of 0 }
    uint8_t invoice_id[32];
    int64_t invoice_id_len = otxn_field(SBUF(invoice_id), sfInvoiceID);

    // check if a trustline exists between the sender and the hook for the USD currency [ PUSD ]
    uint8_t keylet[34];
    if (util_keylet(SBUF(keylet), KEYLET_LINE, SBUF(hook_accid), SBUF(otxn_accid), SBUF(currency)) != 34)
        rollback(SBUF("Peggy: Internal error, could not generate keylet"), 10);
    
    int64_t user_peggy_trustline_slot = slot_set(SBUF(keylet), 0);
    TRACEVAR(user_peggy_trustline_slot);
    if (user_peggy_trustline_slot < 0)
        rollback(SBUF("Peggy: You must have a trustline set for USD to this account."), 10);


    // because the trustline is actually a ripplestate object with a 'high' and a 'low' account
    // we need to compare the hook account with the user's account to determine which side of the line to
    // examine for an adequate limit
    int compare_result = 0;
    ACCOUNT_COMPARE(compare_result, hook_accid, otxn_accid);
    if (compare_result == 0)
        rollback(SBUF("Peggy: Invalid trustline set hi=lo?"), 1);

    int64_t lim_slot = slot_subfield(user_peggy_trustline_slot, ((compare_result > 0) ? sfLowLimit : sfHighLimit), 0); 
    if (lim_slot < 0)
        rollback(SBUF("Peggy: Could not find sfLowLimit on oracle trustline"), 20);

    int64_t user_trustline_limit = slot_float(lim_slot);
    if (user_trustline_limit < 0)
        rollback(SBUF("Peggy: Could not parse user trustline limit"), 1);

    int64_t required_limit = float_set(10, 1);
    if (float_compare(user_trustline_limit, required_limit, COMPARE_EQUAL | COMPARE_GREATER) != 1)
        rollback(SBUF("Peggy: You must set a trustline for USD to peggy for limit of at least 10B"), 1);

    // execution to here means the invoking account has the required trustline with the required limit
    // now fetch the price oracle accounts and data (which also lives in a trustline)
    // both parameters must be exactly one 20-byte AccountID
    uint8_t oracle_lo[20];
    if (hook_param(SBUF(oracle_lo), "oracle_lo", 9) != 20)
        rollback(SBUF("Peggy: \"oracle_lo\" parameter missing or not 20 bytes"), 4);

    uint8_t oracle_hi[20];
    if (hook_param(SBUF(oracle_hi), "oracle_hi", 9) != 20)
        rollback(SBUF("Peggy: \"oracle_hi\" parameter missing or not 20 bytes"), 6);

    if (util_keylet(SBUF(keylet), KEYLET_LINE, SBUF(oracle_lo), SBUF(oracle_hi), SBUF(currency)) != 34)
        rollback(SBUF("Peggy: Internal error, could not generate keylet"), 10);

    int64_t slot_no = slot_set(SBUF(keylet), 0);
    TRACEVAR(slot_no);
    if (slot_no < 0)
        rollback(SBUF("Peggy: Could not find oracle trustline"), 10);

    lim_slot = slot_subfield(slot_no, sfLowLimit, 0);
    if (lim_slot < 0)
        rollback(SBUF("Peggy: Could not find sfLowLimit on oracle trustline"), 20);

    int64_t exchange_rate = slot_float(lim_slot);
    if (exchange_rate < 0) 
        rollback(SBUF("Peggy: Could not get exchange rate float"), 20);
   
    // execution to here means we have retrieved the exchange rate from the oracle
    TRACEXFL(exchange_rate);
   
    // process the amount sent, which could be either xrp or pusd
    // to do this we 'slot' the originating txn, that is: we place it into a slot so we can use the slot api
    // to examine its internals
    int64_t oslot = otxn_slot(0);
    if (oslot < 0)
        rollback(SBUF("Peggy: Could not slot originating txn."), 1);

    // specifically we're interested in the amount sent
    int64_t amt_slot = slot_subfield(oslot, sfAmount, 0);
    if (amt_slot < 0)
        rollback(SBUF("Peggy: Could not slot otxn.sfAmount"), 2);

    int64_t amt = slot_float(amt_slot);
    if (amt < 0)
        rollback(SBUF("Peggy: Could not parse amount."), 1);

    // the slot_type api allows determination of fields and subtypes of fields according to the doco
    // in this case we're examining an amount field to see if it's a native (xrp) amount or an iou amount
    // this means passing flag=1
    int64_t is_xrp = slot_type(amt_slot, 1);
    if (is_xrp < 0)
        rollback(SBUF("Peggy: Could not determine sent amount type"), 3);


    // In addition to determining the amount sent (and its type) we also need to handle the "recollateralization"
    // takeover mode. This is where another user, not the original vault owner, passes the vault ID as invoice ID
    // and sends a payment that brings the vault back into a valid state. We then assign ownership to this person
    // as a reward for stablising the vault. So account for this and record whether or not we are proceeding as
    // the original vault owner (or a new vault) or in takeover mode.
    uint8_t is_vault_owner = 1;
    uint8_t vault_key[32] = { 0 };
    if (invoice_id_len != 32)
    {
        // this is normal mode
        BYTES20_TO_BUF(vault_key, otxn_accid);
        UINT32_TO_BUF(vault_key + 20, source_tag);
    }
    else
    {
        // this is the takeover mode: the first 24 bytes of the invoice id are the vault key
        *(uint64_t*)(vault_key +  0) = *(uint64_t*)(invoice_id +  0);
        *(uint64_t*)(vault_key +  8) = *(uint64_t*)(invoice_id +  8);
        *(uint64_t*)(vault_key + 16) = *(uint64_t*)(invoice_id + 16);
        is_vault_owner = 0;
    }

    // check if state currently exists
    uint8_t vault[16];
    int64_t vault_pusd = 0;
    int64_t vault_xrp = 0;
    uint8_t vault_exists = 0;
    if (state(SBUF(vault), SBUF(vault_key)) == 16)
    {
        vault_pusd = float_sto_set(vault, 8);
        vault_xrp  = float_sto_set(vault + 8, 8);
        vault_exists = 1;
    }
    else if (is_vault_owner == 0)
        rollback(SBUF("Peggy: You cannot takeover a vault that does not exist!"), 1);

    if (is_xrp)
    {
        // XRP INCOMING
        
        // decide whether the vault is liquidatable
        int64_t required_vault_xrp = float_divide(vault_pusd, exchange_rate);
        required_vault_xrp =
            float_mulratio(required_vault_xrp, 0, LIQ_COLLATERALIZATION_DENOMINATOR, LIQ_COLLATERALIZATION_NUMERATOR);
        uint8_t can_liq = (required_vault_xrp < vault_xrp);
        
        // compute new vault xrp by adding the xrp they just sent
        vault_xrp = float_sum(amt, vault_xrp);

        // compute the maximum amount of pusd that can be out according to the collateralization
        int64_t max_vault_pusd = float_multiply(vault_xrp, exchange_rate);
        max_vault_pusd =
            float_mulratio(max_vault_pusd, 0, NEW_COLLATERALIZATION_NUMERATOR, NEW_COLLATERALIZATION_DENOMINATOR);

        // compute the amount we can send them
        int64_t pusd_to_send =
            float_sum(max_vault_pusd, float_negate(vault_pusd));
        if (pusd_to_send < 0)
            rollback(SBUF("Peggy: Error computing pusd to send"), 1);

        // is the amount to send negative, that means the vault is undercollateralized
        if (float_compare(pusd_to_send, 0, COMPARE_LESS))
        {
            if (!is_vault_owner)
                rollback(SBUF("Peggy: Vault is undercollateralized and your deposit would not redeem it."), 1);
            else
            {
                if (float_sto(vault + 8, 8, 0,0,0,0, vault_xrp, -1) != 8)
                    rollback(SBUF("Peggy: Internal error writing vault"), 1);
                if (state_set(SBUF(vault), SBUF(vault_key)) != 16)
                    rollback(SBUF("Peggy: Could not set state"), 1);
                accept(SBUF("Peggy: Vault is undercollateralized, absorbing without sending anything."), 0);
            }
        }

        if (!is_vault_owner && !can_liq)
            rollback(SBUF("Peggy: Vault is not sufficiently undercollateralized to take over yet."), 2);

        // execution to here means we will send out pusd

        // update the vault
        vault_pusd = float_sum(vault_pusd, pusd_to_send);

        // if this is a takeover we destroy the vault on the old key and recreate it on the new key
        if (!is_vault_owner)
        {
            // destroy 
            if (state_set(0,0,SBUF(vault_key)) < 0)
                rollback(SBUF("Peggy: Could not destroy old vault."), 1);

            // reset the key; bytes 24..31 are still zero, only the first 24 bytes are ever written
            BYTES20_TO_BUF(vault_key, otxn_accid);
            UINT32_TO_BUF(vault_key + 20, source_tag);
        }

        // set / update the vault
        if (float_sto(vault, 8, 0,0,0,0, vault_pusd, -1) != 8 ||
            float_sto(vault + 8, 8, 0,0,0,0, vault_xrp, -1) != 8)
            rollback(SBUF("Peggy: Internal error writing vault"), 1);

        if (state_set(SBUF(vault), SBUF(vault_key)) != 16)
            rollback(SBUF("Peggy: Could not set state"), 1);

        // fill in the PUSD Payment: destination and tags first
        SET_ACCOUNT(DEST_OUT_IOU, otxn_accid);
        SET_UINT32(DTAG_OUT_IOU, source_tag);
        SET_UINT32(STAG_OUT_IOU, source_tag);

        // float_sto with field code sfAmount writes the whole 49-byte issued amount
        // (field header, value, currency, issuer). The template already starts the
        // amount with that header byte, one byte before AMOUNT_OUT_IOU.
        if (float_sto(AMOUNT_OUT_IOU - 1, 49, SBUF(currency), SBUF(hook_accid), pusd_to_send, sfAmount) != 49)
            rollback(SBUF("Peggy: Could not write the PUSD amount"), 1);

        PREPARE_TXN_IOU();

        uint8_t emithash[32];
        if (emit(SBUF(emithash), SBUF(txn_iou)) != 32)
            rollback(SBUF("Peggy: Emitting txn failed"), 1);

        accept(SBUF("Peggy: Sent you PUSD!"), 0);
    }
    else
    {

        // NON-XRP incoming
        if (!vault_exists)
            rollback(SBUF("Peggy: Can only send PUSD back to an existing vault."), 1);

        uint8_t amount_buffer[48];
        if (slot(SBUF(amount_buffer), amt_slot) != 48)
            rollback(SBUF("Peggy: Could not dump sfAmount"), 1);

        // ensure the issuer is us
        if (!BUFFER_EQUAL_20(amount_buffer + 28, hook_accid))
            rollback(SBUF("Peggy: A currency we didn't issue was sent to us."), 1);

        // ensure the currency is PUSD
        if (!BUFFER_EQUAL_20(amount_buffer + 8, currency))
            rollback(SBUF("Peggy: A non USD currency was sent to us."), 1);

        TRACEVAR(vault_pusd);

        // decide whether the vault is liquidatable
        int64_t required_vault_xrp = float_divide(vault_pusd, exchange_rate);
        required_vault_xrp =
            float_mulratio(required_vault_xrp, 0, LIQ_COLLATERALIZATION_DENOMINATOR, LIQ_COLLATERALIZATION_NUMERATOR);
        uint8_t can_liq = (required_vault_xrp < vault_xrp);


        // compute new vault pusd by adding the pusd they just sent
        vault_pusd = float_sum(float_negate(amt), vault_pusd);

        // compute the maximum amount of pusd that can be out according to the collateralization
        int64_t max_vault_xrp = float_divide(vault_pusd, exchange_rate);
        max_vault_xrp =
            float_mulratio(max_vault_xrp, 0, NEW_COLLATERALIZATION_DENOMINATOR, NEW_COLLATERALIZATION_NUMERATOR);


        // compute the amount we can send them
        int64_t xrp_to_send =
            float_sum(float_negate(max_vault_xrp), vault_xrp);

        if (xrp_to_send < 0)
            rollback(SBUF("Peggy: Error computing xrp to send"), 1);

        // is the amount to send negative, that means the vault is undercollateralized
        if (float_compare(xrp_to_send, 0, COMPARE_LESS))
        {
            if (!is_vault_owner)
                rollback(SBUF("Peggy: Vault is undercollateralized and your deposit would not redeem it."), 1);
            else
            {
                if (float_sto(vault, 8, 0,0,0,0, vault_pusd, -1) != 8)
                    rollback(SBUF("Peggy: Internal error writing vault"), 1);

                if (state_set(SBUF(vault), SBUF(vault_key)) != 16)
                    rollback(SBUF("Peggy: Could not set state"), 1);

                accept(SBUF("Peggy: Vault is undercollateralized, absorbing without sending anything."), 0);
            }
        }

        if (!is_vault_owner && !can_liq)
            rollback(SBUF("Peggy: Vault is not sufficiently undercollateralized to take over yet."), 2);

        // execution to here means we will send out pusd

        // update the vault
        vault_xrp = float_sum(vault_xrp, xrp_to_send);

        // if this is a takeover we destroy the vault on the old key and recreate it on the new key
        if (!is_vault_owner)
        {
            // destroy 
            if (state_set(0,0,SBUF(vault_key)) < 0)
                rollback(SBUF("Peggy: Could not destroy old vault."), 1);

            // reset the key; bytes 24..31 are still zero, only the first 24 bytes are ever written
            BYTES20_TO_BUF(vault_key, otxn_accid);
            UINT32_TO_BUF(vault_key + 20, source_tag);
        }

        // set / update the vault
        if (float_sto(vault, 8, 0,0,0,0, vault_pusd, -1) != 8 ||
            float_sto(vault + 8, 8, 0,0,0,0, max_vault_xrp, -1) != 8)
            rollback(SBUF("Peggy: Internal error writing vault"), 1);

        if (state_set(SBUF(vault), SBUF(vault_key)) != 16)
            rollback(SBUF("Peggy: Could not set state"), 1);

        // fill in the XAH Payment; the amount is a whole number of drops
        int64_t drops = float_int(xrp_to_send, 6, 0);
        if (drops < 0)
            rollback(SBUF("Peggy: Could not convert XAH amount to drops"), 1);

        SET_NATIVE_AMOUNT(AMOUNT_OUT_XAH, drops);
        SET_ACCOUNT(DEST_OUT_XAH, otxn_accid);
        SET_UINT32(DTAG_OUT_XAH, source_tag);
        SET_UINT32(STAG_OUT_XAH, source_tag);

        PREPARE_TXN_XAH();

        uint8_t emithash[32];
        if (emit(SBUF(emithash), SBUF(txn_xah)) != 32)
            rollback(SBUF("Peggy: Emitting txn failed"), 1);

        accept(SBUF("Peggy: Sent you XAH!"), 0);
    }
    return 0;
}
