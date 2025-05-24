#!/usr/bin/env python3
"""
set_fastlanes_version.py TAG
────────────────────────────────────────────────────────────────────
• Creates / updates an annotated Git tag pointing at HEAD
• Finds every Cargo.toml that depends on `fls-rs` and rewrites the
  version string to the new tag (leading “v” is stripped).
"""
import argparse
import re
import subprocess
from pathlib import Path
from typing import List

# ── helpers ──────────────────────────────────────────────────────
def run(cmd: List[str], check=True, capture_output=False, **kw):
    """Thin wrapper around subprocess.run"""
    return subprocess.run(cmd, check=check, text=True,
                          capture_output=capture_output, **kw)

def git_root() -> Path:
    out = run(["git", "rev-parse", "--show-toplevel"],
              capture_output=True).stdout.strip()
    return Path(out)

def cargo_toml_files(root: Path) -> List[Path]:
    out = run(["git", "-C", str(root), "ls-files"],
              capture_output=True).stdout.splitlines()
    return [root / p for p in out if p.endswith("Cargo.toml")]

# ── main ─────────────────────────────────────────────────────────
def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("tag", help="Tag to create (e.g. v0.1.4)")
    args = ap.parse_args()

    tag      = args.tag
    version  = tag.lstrip("v")   # strip leading 'v' for Cargo
    root_dir = git_root()

    print(f"▶ Tagging current commit as '{tag}' …")
    # delete tag if it exists (overwrite)
    try:
        run(["git", "rev-parse", tag], check=True, capture_output=True)
        run(["git", "tag", "-d", tag])
    except subprocess.CalledProcessError:
        pass  # tag didn’t exist yet

    run(["git", "tag", "-a", tag, "-m", f"FastLanes release {tag}"])

    print(f"▶ Updating Cargo.toml dependencies to '{version}' …")
    changed: List[Path] = []
    pattern = re.compile(r'^\s*fls-rs\s*=')

    for toml in cargo_toml_files(root_dir):
        text = toml.read_text()
        if pattern.search(text):
            new_text = re.sub(
                r'^\s*fls-rs\s*=.*$',
                f'fls-rs = "{version}"',
                text,
                flags=re.MULTILINE)
            if new_text != text:
                toml.write_text(new_text)
                changed.append(toml)

    if not changed:
        print("⚠️  No Cargo.toml with 'fls-rs' dependency found.", flush=True)
    else:
        run(["git", "add", *map(str, changed)])
        run(["git", "commit", "-m", f"Bump fls-rs dependency to {version}"])
        print("✓ Updated and committed:", ", ".join(map(str, changed)))

    print("\nAll done. Push with:  git push --follow-tags")

if __name__ == "__main__":
    main()
