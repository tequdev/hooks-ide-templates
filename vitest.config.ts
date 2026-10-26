import { defineConfig } from "vitest/config";

// One standalone node and fixed toolkit wallets: test files must not run concurrently.
export default defineConfig({
  test: {
    fileParallelism: false,
    testTimeout: 120_000,
    hookTimeout: 300_000,
    include: ["test/**/*.test.ts"],
  },
});
