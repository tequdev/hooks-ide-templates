#!/bin/bash
set -uo pipefail
V="${XAHAUD_VERSION:-2026.10.13-release+4119}"
xrpld-lab down:standalone --protocol xahau --version "$V" || true
