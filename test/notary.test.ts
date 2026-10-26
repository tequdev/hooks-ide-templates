import { afterAll, beforeAll, describe, expect, it } from "vitest";
import { encode } from "xahau";
import {
  type Context,
  compileHook,
  installHook,
  setup,
  submit,
  teardown,
  txParams,
  uninstallHook,
  waitForEmitted,
} from "./helpers";

const NS = "notary";

describe("notary", () => {
  let ctx: Context;
  let ledger: number;

  /** Unsigned Payment hook5 -> frank, serialized the way propose.js does. */
  const proposal = (lastLedger: number, amount = "100") =>
    encode({
      TransactionType: "Payment",
      Account: ctx.hook5.address,
      Destination: ctx.frank.address,
      Amount: amount,
      LastLedgerSequence: lastLedger,
      Fee: "0",
      Sequence: 0,
    });

  const invoke = (from: typeof ctx.frank, extra: Record<string, unknown>) =>
    submit(ctx, from, {
      TransactionType: "Invoke",
      Account: from.address,
      Destination: ctx.hook5.address,
      ...extra,
    } as Parameters<typeof submit>[2]);

  beforeAll(async () => {
    ctx = await setup();
    const r = await submit(ctx, ctx.hook5, {
      TransactionType: "SignerListSet",
      Account: ctx.hook5.address,
      SignerQuorum: 2,
      SignerEntries: [
        { SignerEntry: { Account: ctx.frank.address, SignerWeight: 1 } },
        { SignerEntry: { Account: ctx.grace.address, SignerWeight: 1 } },
      ],
    });
    expect(r.engineResult).toBe("tesSUCCESS");
    await installHook(ctx, ctx.hook5, await compileHook("notary"), {
      hookOn: ["Invoke"],
      namespace: NS,
    });
    ledger = await ctx.client.getLedgerIndex();
  });

  afterAll(async () => {
    await uninstallHook(ctx, ctx.hook5, NS);
    await teardown(ctx);
  });

  it("collects two signatures and emits the payment", async () => {
    const p = await invoke(ctx.frank, { Blob: proposal(ledger + 500) });
    expect(p.engineResult).toBe("tesSUCCESS");
    expect(p.executions[0].result).toBe("accept");
    const id = /id: ([0-9A-F]{64})$/.exec(p.executions[0].message)?.[1];
    expect(id).toBeDefined();

    const e = await invoke(ctx.grace, txParams({ id: id as string }));
    expect(e.engineResult).toBe("tesSUCCESS");
    expect(e.executions[0].result).toBe("accept");
    expect(e.executions[0].message).toContain("emitted");

    const emitted = await waitForEmitted(ctx.client, ctx.frank.address, e.hash);
    expect(emitted.tx.TransactionType).toBe("Payment");
    expect(emitted.tx.Amount).toBe("100");
    expect(emitted.tx.Destination).toBe(ctx.frank.address);
  });

  it("rejects a proposal from an account that is not a signer", async () => {
    const r = await invoke(ctx.heidi, { Blob: proposal(ledger + 500, "101") });
    expect(r.engineResult).toBe("tecHOOK_REJECTED");
    expect(r.executions[0].message).toContain("signer list");
  });

  it("rejects an Invoke with both a Blob and an id", async () => {
    const r = await invoke(ctx.frank, {
      Blob: proposal(ledger + 500, "102"),
      ...txParams({ id: "AB".repeat(32) }),
    });
    expect(r.engineResult).toBe("tecHOOK_REJECTED");
    expect(r.executions[0].message).toContain("not both");
  });

  it("rejects an endorsement of an unknown id", async () => {
    const r = await invoke(ctx.grace, txParams({ id: "CD".repeat(32) }));
    expect(r.engineResult).toBe("tecHOOK_REJECTED");
    expect(r.executions[0].message).toContain("does not match");
  });

  it("rejects a proposal that expires too soon", async () => {
    const r = await invoke(ctx.frank, { Blob: proposal(ledger + 5, "103") });
    expect(r.engineResult).toBe("tecHOOK_REJECTED");
    expect(r.executions[0].message).toContain("LastLedgerSequence");
  });

  it("rejects a Blob over 256 bytes", async () => {
    const r = await invoke(ctx.frank, { Blob: "AB".repeat(300) });
    expect(r.engineResult).toBe("tecHOOK_REJECTED");
    expect(r.executions[0].message).toContain("256");
  });
});
