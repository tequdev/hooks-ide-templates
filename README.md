# Hooks IDE templates

Source of the example Hooks offered as templates in the
[Xahau Hooks Builder](https://github.com/Xahau/xrpl-hooks-ide) (Starter,
Firewall, Notary, Carbon, Peggy).

## Layout

- `templates/<name>/` – one directory per template: the Hook source, its
  README and any helper scripts, exactly as the IDE presents them.
- `headers/` – the shared Hook API headers the IDE adds to every template.

## Origin

This initial commit imports the files verbatim from the GitHub Gists the IDE
used until now:

| Template | Gist |
|---|---|
| starter | https://gist.github.com/1f8109c80f504e6326db2735df2f0ad6 |
| firewall | https://gist.github.com/1cc30f39c8a0b9c55b88c312669ca45e |
| notary | https://gist.github.com/87b6f5a8c2f5038fb0f20b8b510efa10 |
| carbon | https://gist.github.com/953662b22d065449f8ab6f69bc2afe41 |
| peggy | https://gist.github.com/049784a83fa068faf7912f663f7b6471 |
| headers | https://gist.github.com/028e8ce6d6d674776970caf8acc77ecc |
