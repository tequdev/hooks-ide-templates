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

const NS = "carbon";

describe("carbon", () => {
  let ctx: Context;
  let code: string;

  const install = (carbon_acc: string) =>
    installHook(ctx, ctx.hook2, code, {
      hookOn: ["Payment"],
      namespace: NS,
      params: { carbon_acc },
    });

  const payBob = (Amount: string) =>
    submit(ctx, ctx.hook2, {
      TransactionType: "Payment",
      Account: ctx.hook2.address,
      Destination: ctx.bob.address,
      Amount,
    });

  beforeAll(async () => {
    ctx = await setup();
    code = await compileHook("carbon");
    await install(accountIdHex(ctx.carol.address));
  });

  afterAll(async () => {
    await uninstallHook(ctx, ctx.hook2, NS);
    await teardown(ctx);
  });

  it("emits 1% to the beneficiary", async () => {
    const r = await payBob("10000000");
    expect(r.engineResult).toBe("tesSUCCESS");
    expect(r.executions).toHaveLength(1);
    expect(r.executions[0].result).toBe("accept");
    const e = await waitForEmitted(ctx.client, ctx.carol.address, r.hash);
    expect(e.tx.TransactionType).toBe("Payment");
    expect(e.tx.Amount).toBe("100000");
    expect(e.tx.Destination).toBe(ctx.carol.address);
  });

  it("emits the default 1000 drops for small payments", async () => {
    const r = await payBob("50000");
    expect(r.executions[0].result).toBe("accept");
    const e = await waitForEmitted(ctx.client, ctx.carol.address, r.hash);
    expect(e.tx.Amount).toBe("1000");
  });

  it("ignores incoming payments", async () => {
    const r = await submit(ctx, ctx.alice, {
      TransactionType: "Payment",
      Account: ctx.alice.address,
      Destination: ctx.hook2.address,
      Amount: "1000000",
    });
    expect(r.engineResult).toBe("tesSUCCESS");
    expect(r.executions[0].result).toBe("accept");
    expect(r.executions[0].message).toContain("incoming");
    const created = r.meta.AffectedNodes.filter(
      (n) =>
        "CreatedNode" in n && n.CreatedNode.LedgerEntryType === "EmittedTxn",
    );
    expect(created).toHaveLength(0);
  });

  it("rolls back when carbon_acc is empty", async () => {
    await install("");
    const r = await payBob("10000000");
    expect(r.engineResult).toBe("tecHOOK_REJECTED");
    expect(r.executions[0].result).toBe("rollback");
    expect(r.executions[0].message).toContain("carbon_acc");
  });
});
