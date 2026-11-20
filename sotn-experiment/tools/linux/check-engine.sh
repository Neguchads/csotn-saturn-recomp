#!/bin/bash
# Testes do motor vendorizado; não clona a raiz do port como se fosse outro repositório.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$ROOT/engine"
tools/check.sh
