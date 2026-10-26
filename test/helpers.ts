import { readdirSync, readFileSync } from "node:fs";
import {
  clearAllHooksV3,
  clearHookStateV3,
  ExecutionUtility,
  hexNamespace,
  type iHook,
  ledgerAccept,
  serverUrl,
  setHooksV3,
  setupClient,
  teardownClient,
  testTransaction,
  type XrplIntegrationTestContext,
} from "@xahau/hooks-toolkit";
import { createCompiler } from "hooks-compiler";
import {
  type Client,
  calculateHookOn,
  decodeAccountID,
  type Transaction,
  type TransactionMetadata,
  type Wallet,
} from "xahau";
import type { TransactionMetadataBase } from "xahau/dist/npm/models/transactions/metadata";

export type Context = XrplIntegrationTestContext;

const headers = Object.fromEntries(
  readdirSync("headers").map((f) => [f, readFileSync(`headers/${f}`, "utf8")]),
);
const compiler = createCompiler();

/** Compile templates/<template>/<file> exactly as the IDE does (hooks-compiler, bundled headers); returns the CreateCode hex. */
export async function compileHook(
  template: string,
  file = `${template}.c`,
): Promise<string> {
  const source = readFileSync(`templates/${template}/${file}`, "utf8");
  const r = await compiler.compile({
    files: { ...headers, [file]: source },
    entry: file,
  });
  return Buffer.from(r.wasm).toString("hex").toUpperCase();
}

export const hex = (s: string) =>
  Buffer.from(s, "utf8").toString("hex").toUpperCase();
/** 20-byte AccountID as hex, the form the templates expect in hook parameters. */
export const accountIdHex = (address: string) =>
  Buffer.from(decodeAccountID(address)).toString("hex").toUpperCase();

export const HSF_OVERRIDE = 1;
export const HSF_NSDELETE = 2;

export interface InstallOptions {
  hookOn: string[];
  namespace: string;
  /** Install-time parameters; names are text, values hex (same as the IDE's SetHook dialog). */
  params?: Record<string, string>;
}

/** SetHook (hsfOverride) on `wallet` with the given CreateCode. Installing is the real validator: a guard or import violation fails here. */
export async function installHook(
  ctx: Context,
  wallet: Wallet,
  createCode: string,
  opts: InstallOptions,
) {
  const hook: iHook = {
    CreateCode: createCode,
    Flags: HSF_OVERRIDE,
    HookOn: calculateHookOn(
      opts.hookOn as Parameters<typeof calculateHookOn>[0],
    ),
    HookNamespace: hexNamespace(opts.namespace),
    HookApiVersion: 0,
    HookParameters: Object.entries(opts.params ?? {}).map(([name, value]) => ({
      HookParameter: {
        HookParameterName: hex(name),
        HookParameterValue: value,
      },
    })),
  };
  await setHooksV3({ client: ctx.client, wallet, hooks: [{ Hook: hook }] });
}

/** Remove the hook and its namespace state so a persisted local node does not leak into the next file. */
export async function uninstallHook(
  ctx: Context,
  wallet: Wallet,
  namespace: string,
) {
  await clearHookStateV3({
    client: ctx.client,
    wallet,
    hooks: [
      { Hook: { Flags: HSF_NSDELETE, HookNamespace: hexNamespace(namespace) } },
    ],
  }).catch(() => undefined); // an empty namespace is already gone
  await clearAllHooksV3({ client: ctx.client, wallet });
}

export async function setup(): Promise<Context> {
  return setupClient(serverUrl);
}

export async function teardown(ctx: Context) {
  await teardownClient(ctx);
}

export interface HookExecution {
  account: string;
  result: "accept" | "rollback" | "error";
  code: number;
  message: string;
}

const SIGN_BIT = 1n << 63n;
/** HookReturnCode is a sign-magnitude hex string, not two's complement. */
function returnCode(h: string): number {
  const v = BigInt(`0x${h}`);
  const magnitude = v & ~SIGN_BIT;
  return Number((v & SIGN_BIT) !== 0n ? -magnitude : magnitude);
}

export function hookExecutions(
  meta: TransactionMetadata | TransactionMetadataBase,
): HookExecution[] {
  const execs =
    (
      meta as {
        HookExecutions?: { HookExecution: Record<string, string | number> }[];
      }
    ).HookExecutions ?? [];
  return execs.map(({ HookExecution: e }) => ({
    account: String(e.HookAccount),
    result:
      (
        { 3: "accept", 2: "rollback" } as Record<
          string,
          HookExecution["result"]
        >
      )[String(Number(e.HookResult))] ?? "error",
    code: returnCode(String(e.HookReturnCode)),
    message: Buffer.from(String(e.HookReturnString ?? ""), "hex")
      .toString("utf8")
      .replace(/\0+$/, ""),
  }));
}

export interface SubmitResult {
  hash: string;
  engineResult: string;
  meta: TransactionMetadata;
  executions: HookExecution[];
}

/** Sign, submit and close a ledger. Never throws on tecHOOK_REJECTED so one helper serves happy-path and rollback tests. */
export async function submit(
  ctx: Context,
  wallet: Wallet,
  tx: Transaction,
): Promise<SubmitResult> {
  const res = await testTransaction(
    ctx.client,
    tx as Parameters<typeof testTransaction>[1],
    wallet,
  );
  const meta = res.result.meta as TransactionMetadata;
  return {
    hash: res.result.hash,
    engineResult: (meta as { TransactionResult: string }).TransactionResult,
    meta,
    executions: hookExecutions(meta),
  };
}

/** Hook parameters on a transaction: names text, values hex. */
export function txParams(params: Record<string, string>) {
  return {
    HookParameters: Object.entries(params).map(([name, value]) => ({
      HookParameter: {
        HookParameterName: hex(name),
        HookParameterValue: value,
      },
    })),
  };
}

/** Close ledgers until the transaction emitted by `parentHash` has been applied on `account`, then return it. */
export async function waitForEmitted(
  client: Client,
  account: string,
  parentHash: string,
  closes = 8,
) {
  for (let i = 0; i < closes; i++) {
    await ledgerAccept(client);
    const r = await client.request({
      command: "account_tx",
      account,
      limit: 20,
    });
    const hit = (
      r.result.transactions as {
        tx?: Record<string, unknown>;
        tx_json?: Record<string, unknown>;
        meta?: unknown;
      }[]
    ).find((t) => {
      const tx = (t.tx ?? t.tx_json) as
        | { EmitDetails?: { EmitParentTxnID?: string } }
        | undefined;
      return tx?.EmitDetails?.EmitParentTxnID === parentHash;
    });
    if (hit)
      return {
        tx: (hit.tx ?? hit.tx_json) as Record<string, unknown>,
        meta: hit.meta as TransactionMetadata,
      };
  }
  throw new Error(
    `emitted transaction of ${parentHash} did not land on ${account}`,
  );
}

export { ExecutionUtility };
