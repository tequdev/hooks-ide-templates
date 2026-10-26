#include "hookapi.h"

/**
 * Carbon: every payment the hook account sends also emits 1% to a beneficiary.
 *
 * @name Carbon
 * @on_outgoing ttPAYMENT
 * @emit ttPAYMENT
 * @param carbon_acc The beneficiary account (20-byte AccountID, hex)
 */

// clang-format off
uint8_t txn[265] =
{
/* size, upto, field name               */
/*    3,    0, tt = Payment             */   0x12U, 0x00U, 0x00U,
/*    5,    3, flags                    */   0x22U, 0x00U, 0x00U, 0x00U, 0x00U,
/*    5,    8, sequence                 */   0x24U, 0x00U, 0x00U, 0x00U, 0x00U,
/*    5,   13, destinationtag           */   0x2EU, 0x00U, 0x00U, 0x00U, 0x00U,
/*    6,   18, firstledgersequence      */   0x20U, 0x1AU, 0x00U, 0x00U, 0x00U, 0x00U,
/*    6,   24, lastledgersequence       */   0x20U, 0x1BU, 0x00U, 0x00U, 0x00U, 0x00U,
/*    9,   30, amount                   */   0x61U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U,
/*    9,   39, fee                      */   0x68U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/*   35,   48, signingpubkey            */   0x73U, 0x21U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*   22,   83, account                  */   0x81U, 0x14U, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*   22,  105, destination              */   0x83U, 0x14U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
/*  138,  127, emit details             */ 
/*    0,  265,                          */ 
};
// clang-format on

// TX BUILDER
#define FLAGS_OUT (txn + 4U)
#define DTAG_OUT (txn + 14U)
#define FLS_OUT (txn + 20U)
#define LLS_OUT (txn + 26U)
#define AMOUNT_OUT (txn + 31U)
#define FEE_OUT (txn + 40U)
#define ACCOUNT_OUT (txn + 85U)
#define DEST_OUT (txn + 107U)
#define EMIT_OUT (txn + 127U)

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

#define PREPARE_TXN()                                                          \
  do {                                                                         \
    etxn_reserve(1);                                                           \
    uint32_t fls = (uint32_t)ledger_seq() + 1;                                 \
    SET_UINT32(FLS_OUT, fls);                                                  \
    SET_UINT32(LLS_OUT, fls + 4);                                              \
    hook_account(ACCOUNT_OUT, 20);                                             \
    etxn_details(EMIT_OUT, 138U);                                              \
    int64_t fee = etxn_fee_base(SBUF(txn));                                    \
    SET_NATIVE_AMOUNT(FEE_OUT, fee);                                           \
    TRACEHEX(txn);                                                             \
  } while (0)

// Called when the emitted payment has been applied (or has failed).
int64_t cbak(uint32_t reserved)
{
    _g(1, 1); // every hook must call _g once, first, to declare the guard budget
    TRACESTR("Carbon: callback called");
    accept(SBUF("Carbon: callback"), 0);
    return 0;
}

int64_t hook(uint32_t reserved)
{
    _g(1, 1);
    TRACESTR("Carbon: started");

    // Only payments are interesting.
    if (otxn_type() != ttPAYMENT)
        accept(SBUF("Carbon: not a payment"), 0);

    // Hooks run for incoming and outgoing transactions: compare the sender with the hook account.
    uint8_t hook_acc[20];
    hook_account(SBUF(hook_acc));
    uint8_t otxn_acc[20];
    if (otxn_field(SBUF(otxn_acc), sfAccount) != 20)
        rollback(SBUF("Carbon: sfAccount missing"), 1);

    if (!BUFFER_EQUAL_20(hook_acc, otxn_acc))
        accept(SBUF("Carbon: incoming payment"), 0);

    // The beneficiary is an install parameter (HookParameter), set in the SetHook dialog.
    uint8_t carbon_acc[20];
    if (hook_param(SBUF(carbon_acc), "carbon_acc", 10) != 20)
        rollback(SBUF("Carbon: carbon_acc parameter missing or not 20 bytes"), 2);

    // sfAmount is 8 bytes for XAH (drops) and 48 bytes for an IOU.
    uint8_t amount[48];
    int64_t amount_len = otxn_field(SBUF(amount), sfAmount);
    if (amount_len < 0)
        rollback(SBUF("Carbon: sfAmount missing"), 3);

    int64_t drops_to_send = 1000; // default: small payments and IOUs send 1000 drops
    if (amount_len == 8)
    {
        int64_t drops = AMOUNT_TO_DROPS(amount);
        if (drops > 100000)
            drops_to_send = drops / 100; // 1%, integer arithmetic only
    }
    TRACEVAR(drops_to_send);

    // Fill in the Transaction Builder template and emit it.
    // PREPARE_TXN reserves the emit slot, sets ledger limits, fee and EmitDetails.
    SET_NATIVE_AMOUNT(AMOUNT_OUT, drops_to_send);
    SET_ACCOUNT(DEST_OUT, carbon_acc);
    PREPARE_TXN();

    uint8_t emithash[32];
    int64_t emit_result = emit(SBUF(emithash), SBUF(txn));
    TRACEVAR(emit_result);
    if (emit_result != 32)
        rollback(SBUF("Carbon: emit failed"), 4);

    // Let the original payment through.
    accept(SBUF("Carbon: emitted 1% to the beneficiary"), 0);
    return 0;
}
