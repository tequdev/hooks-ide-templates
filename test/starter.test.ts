import { afterAll, beforeAll, describe, expect, it } from "vitest";
import {
  type Context,
  compileHook,
  installHook,
  setup,
  submit,
  teardown,
  uninstallHook,
} from "./helpers";

const NS = "starter";

describe("starter", () => {
  let ctx: Context;

  beforeAll(async () => {
    ctx = await setup();
    await installHook(ctx, ctx.hook1, await compileHook("starter"), {
      hookOn: ["Payment"],
      namespace: NS,
    });
  });

  afterAll(async () => {
    await uninstallHook(ctx, ctx.hook1, NS);
    await teardown(ctx);
  });

  it("accepts an incoming payment", async () => {
    const r = await submit(ctx, ctx.alice, {
      TransactionType: "Payment",
      Account: ctx.alice.address,
      Destination: ctx.hook1.address,
      Amount: "1000000",
    });
    expect(r.engineResult).toBe("tesSUCCESS");
    expect(r.executions).toHaveLength(1);
    expect(r.executions[0].result).toBe("accept");
    expect(r.executions[0].message).toBe("Starter: accepted");
  });
});
