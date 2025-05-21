#!/usr/bin/env bash
set -euo pipefail

# 1) locate the repo root (one level up from this script, then verify with git)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && git rev-parse --show-toplevel)"

cd "$REPO_ROOT"

echo "➤ Step 1: package & commit fresh tarball"
make package-fastlanes
git add rust/fastlanes-src.tar.gz
git commit -m "embed FastLanes sources for crates.io"

echo
echo "➤ Step 2: build & dry-run publish"
make build-rust
make dry-run-rust

echo
echo "➤ Step 3: publish for real 🎉"
make publish-rust

echo
echo "✅ All done!"
