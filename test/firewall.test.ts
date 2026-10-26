import { hexNamespace } from "@xahau/hooks-toolkit";
import { afterAll, beforeAll, describe, expect, it } from "vitest";
import {
  accountIdHex,
  type Context,
  compileHook,
  installHook,
  setup,
  submit,
  teardown,
  txParams,
  uninstallHook,
} from "./helpers";

describe("firewall", () => {
  let ctx: Context;

  beforeAll(async () => {
    ctx = await setup();
    await installHook(
      ctx,
      ctx.hook4,
      await compileHook("firewall", "blacklist.c"),
      {
        hookOn: ["Invoke"],
        namespace: "blacklist",
        params: { admin: accountIdHex(ctx.dave.address) },
      },
    );
    await installHook(
      ctx,
      ctx.hook3,
      await compileHook("firewall", "firewall.c"),
      {
        hookOn: ["Payment"],
        namespace: "firewall",
        params: {
          blaccid: accountIdHex(ctx.hook4.address),
          blns: hexNamespace("blacklist"),
        },
      },
    );
  });

  afterAll(async () => {
    await uninstallHook(ctx, ctx.hook4, "blacklist");
    await uninstallHook(ctx, ctx.hook3, "firewall");
    await teardown(ctx);
  });

  const pay = (from: "carol" | "elsa") =>
    submit(ctx, ctx[from], {
      TransactionType: "Payment",
      Account: ctx[from].address,
      Destination: ctx.hook3.address,
      Amount: "1000000",
    });

  const invoke = (from: "dave" | "elsa", params: Record<string, string>) =>
    submit(ctx, ctx[from], {
      TransactionType: "Invoke",
      Account: ctx[from].address,
      Destination: ctx.hook4.address,
      ...txParams(params),
    });

  it("lets a normal sender pay", async () => {
    const r = await pay("carol");
    expect(r.engineResult).toBe("tesSUCCESS");
    expect(r.executions[0].result).toBe("accept");
  });

  it("admin blocks an account", async () => {
    const r = await invoke("dave", {
      acc: accountIdHex(ctx.elsa.address),
      op: "01",
    });
    expect(r.engineResult).toBe("tesSUCCESS");
    expect(r.executions[0].result).toBe("accept");
    expect(r.executions[0].message).toContain("blocked");
  });

  it("rejects a payment from the blocked account", async () => {
    const r = await pay("elsa");
    expect(r.engineResult).toBe("tecHOOK_REJECTED");
    expect(r.executions[0].result).toBe("rollback");
    expect(r.executions[0].message).toContain("blacklisted");
  });

  it("rejects an Invoke from a non-admin", async () => {
    const r = await invoke("elsa", {
      acc: accountIdHex(ctx.elsa.address),
      op: "00",
    });
    expect(r.engineResult).toBe("tecHOOK_REJECTED");
    expect(r.executions[0].message).toContain("admin");
  });

  it("admin unblocks, payments work again", async () => {
    const r = await invoke("dave", {
      acc: accountIdHex(ctx.elsa.address),
      op: "00",
    });
    expect(r.executions[0].result).toBe("accept");
    expect(r.executions[0].message).toContain("unblocked");
    expect((await pay("elsa")).engineResult).toBe("tesSUCCESS");
  });

  it("rolls back when op is missing", async () => {
    const r = await invoke("dave", { acc: accountIdHex(ctx.elsa.address) });
    expect(r.engineResult).toBe("tecHOOK_REJECTED");
    expect(r.executions[0].result).toBe("rollback");
    expect(r.executions[0].code).toBe(6);
  });
});
