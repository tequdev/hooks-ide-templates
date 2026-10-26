# Hooks IDE templates

Source of the example Hooks offered as templates in the
[Xahau Hooks Builder](https://github.com/Xahau/xrpl-hooks-ide): Starter,
Firewall, Notary, Carbon and Peggy.

Every template is compiled with the IDE's own toolchain
([hooks-compiler](https://github.com/tequdev/hooks-compiler)) against the
headers the IDE bundles, installed on a standalone Xahau node and exercised
end to end, locally and in CI.

## Layout

- `templates/<name>/` – one directory per template: the Hook source, its
  README and any helper scripts, exactly as the IDE presents them.
- `headers/` – the Hook API headers the IDE bundles with every template
  (xahaud's `hook/*.h`).
- `test/` – one Vitest file per template plus `helpers.ts` (compile, install,
  submit, decode `HookExecutions`).
- `scripts/` – `build-all.mjs` compiles every template; `node-up.sh` /
  `node-down.sh` run a standalone xahaud in Docker through `xrpld-lab`.

## Develop

Requires Node 22, pnpm, Docker and Python 3 (for `xrpld-lab`).

```sh
pnpm install
pnpm build        # compile templates/*/*.c into build/*.wasm
pnpm node:up      # start the standalone node (first run downloads xahaud)
pnpm test         # install each template on the node and run its scenarios
pnpm node:down
```

`.env` sets `XAHAU_ENV=standalone` so `@xahau/hooks-toolkit` closes ledgers
itself; without it every submit waits for a consensus that never comes.

## History

The first commit imports the files verbatim from the GitHub Gists the IDE
used until now, so the refactor to the current Hooks conventions can be read
commit by commit:

| Template | Gist |
|---|---|
| starter | https://gist.github.com/1f8109c80f504e6326db2735df2f0ad6 |
| firewall | https://gist.github.com/1cc30f39c8a0b9c55b88c312669ca45e |
| notary | https://gist.github.com/87b6f5a8c2f5038fb0f20b8b510efa10 |
| carbon | https://gist.github.com/953662b22d065449f8ab6f69bc2afe41 |
| peggy | https://gist.github.com/049784a83fa068faf7912f663f7b6471 |
| headers | https://gist.github.com/028e8ce6d6d674776970caf8acc77ecc |
