// Compile every templates/<name>/*.c with the IDE's toolchain (hooks-compiler) against headers/.
import { mkdirSync, readdirSync, readFileSync, writeFileSync } from "node:fs";
import { createCompiler } from "hooks-compiler";

const headers = Object.fromEntries(
  readdirSync("headers").map((f) => [f, readFileSync(`headers/${f}`, "utf8")]),
);
const compiler = createCompiler();
let failed = false;
mkdirSync("build", { recursive: true });
for (const template of readdirSync("templates")) {
  for (const file of readdirSync(`templates/${template}`).filter((f) =>
    f.endsWith(".c"),
  )) {
    const source = readFileSync(`templates/${template}/${file}`, "utf8");
    try {
      const r = await compiler.compile({
        files: { ...headers, [file]: source },
        entry: file,
      });
      writeFileSync(`build/${file.replace(/\.c$/, ".wasm")}`, r.wasm);
      console.log(
        `${template}/${file}: ${r.wasm.length} bytes sha256 ${r.sha256}`,
      );
    } catch (e) {
      failed = true;
      console.error(
        `${template}/${file}: FAILED\n${e instanceof Error ? e.message : e}`,
      );
    }
  }
}
process.exit(failed ? 1 : 0);
