#!/bin/bash
# Start a standalone xahaud in Docker via xrpld-lab (pinned: 6.x dropped the xahau protocol).
set -euo pipefail
V="${XAHAUD_VERSION:-2026.10.13-release+4119}"
if curl -sf -m 3 -X POST http://0.0.0.0:5005 -d '{"method":"server_info"}' | grep -q build_version; then
  echo "standalone node already running"; exit 0
fi
pip install -q 'xrpld-lab==3.1.2' 'anyio<4.15'
xrpld-lab up:standalone --protocol xahau --version "$V"
curl -fsSL -o "workspace/xahau-$V/xahaud.$V" "https://build.xahau.tech/$V"
chmod +x "workspace/xahau-$V/xahaud.$V"
# 3.1.2 renders an unset quorum as the literal "None", which xahaud rejects
perl -pi -e 's/"None"/""/' "workspace/xahau-$V/Dockerfile"
bash "workspace/xahau-$V/start.sh"
for _ in $(seq 1 120); do
  if curl -sf -X POST http://0.0.0.0:5005 -d '{"method":"server_info"}' | grep -q build_version; then
    echo "node is up"; exit 0
  fi
  sleep 2
done
echo "node did not become ready" >&2; docker logs xahau --tail 200 || true; exit 1
