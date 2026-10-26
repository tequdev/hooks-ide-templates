import { afterAll, beforeAll, describe, expect, it } from "vitest";
import {
  accountIdHex,
  type Context,
  compileHook,
  installHook,
  setup,
  submit,
  teardown,
  uninstallHook,
  waitForEmitted,
} from "./helpers";

const NS = "peggy";

describe("peggy", () => {
  let ctx: Context;
  let code: string;
  let lo: string;
  let hi: string;

  /** A Payment from `from` to the hook account. */
  const pay = (from: typeof ctx.gw, Amount: unknown) =>
    submit(ctx, from, {
      TransactionType: "Payment",
      Account: from.address,
      Destination: ctx.hook1.address,
      Amount,
    } as Parameters<typeof submit>[2]);

  beforeAll(async () => {
    ctx = await setup();
    const sorted = [ctx.ivan, ctx.judy].sort((a, b) =>
      accountIdHex(a.address).localeCompare(accountIdHex(b.address)),
    );
    const [loW, hiW] = sorted;
    lo = loW.address;
    hi = hiW.address;

    // the oracle: a USD trust line from lo to hi whose limit is the price
    await submit(ctx, loW, {
      TransactionType: "TrustSet",
      Account: lo,
      LimitAmount: { currency: "USD", issuer: hi, value: "37" },
      Flags: 131072,
    } as Parameters<typeof submit>[2]);
    // the stablecoin user must trust the issuer for at least 10B
    await submit(ctx, ctx.gw, {
      TransactionType: "TrustSet",
      Account: ctx.gw.address,
      LimitAmount: {
        currency: "USD",
        issuer: ctx.hook1.address,
        value: "10000000000",
      },
    } as Parameters<typeof submit>[2]);

    code = await compileHook("peggy");
    await installHook(ctx, ctx.hook1, code, {
      hookOn: ["Payment"],
      namespace: NS,
      params: { oracle_lo: accountIdHex(lo), oracle_hi: accountIdHex(hi) },
    });
  });

  afterAll(async () => {
    await uninstallHook(ctx, ctx.hook1, NS);
    await teardown(ctx);
  });

  it("issues PUSD against XAH at 150% collateralization", async () => {
    const r = await pay(ctx.gw, "100000000");
    expect(r.engineResult).toBe("tesSUCCESS");
    expect(r.executions[0].result).toBe("accept");
    expect(r.executions[0].message).toContain("PUSD");

    const { tx } = await waitForEmitted(ctx.client, ctx.gw.address, r.hash);
    const amount = tx.Amount as {
      currency: string;
      issuer: string;
      value: string;
    };
    expect(amount.currency).toBe("USD");
    expect(amount.issuer).toBe(ctx.hook1.address);
    expect(amount.value.startsWith("2466.66")).toBe(true);
  });

  it("redeems XAH when PUSD is sent back", async () => {
    const r = await pay(ctx.gw, {
      currency: "USD",
      issuer: ctx.hook1.address,
      value: "1000",
    });
    expect(r.engineResult).toBe("tesSUCCESS");
    expect(r.executions[0].result).toBe("accept");
    expect(r.executions[0].message).toMatch(/XRP|XAH/);

    const { tx } = await waitForEmitted(ctx.client, ctx.gw.address, r.hash);
    expect(typeof tx.Amount).toBe("string");
    const drops = Number(tx.Amount);
    expect(drops).toBeGreaterThan(40000000);
    expect(drops).toBeLessThan(41000000);
  });

  it("rolls back when the sender has no USD trust line", async () => {
    const r = await pay(ctx.bob, "1000000");
    expect(r.engineResult).toBe("tecHOOK_REJECTED");
    expect(r.executions[0].result).toBe("rollback");
    expect(r.executions[0].message.toLowerCase()).toContain("trustline");
  });

  it("rolls back when oracle_lo is empty", async () => {
    await installHook(ctx, ctx.hook1, code, {
      hookOn: ["Payment"],
      namespace: NS,
      params: { oracle_lo: "", oracle_hi: accountIdHex(hi) },
    });
    const r = await pay(ctx.gw, "1000000");
    expect(r.engineResult).toBe("tecHOOK_REJECTED");
    expect(r.executions[0].message).toContain("oracle_lo");
  });
});
