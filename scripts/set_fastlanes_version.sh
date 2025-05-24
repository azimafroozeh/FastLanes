#!/usr/bin/env bash
#
# set_fastlanes_version  TAG
# ---------------------------------------------------------------
# Tag the current Git HEAD and bump every `fls-rs = "…"` entry in
# Cargo.toml files to the given version.
#
set - euo
pipefail

if [[ $  # -ne 1 ]]; then
echo "Usage: $0 <tag>" > & 2
exit 1
fi

TAG="$1"
VERSION="${TAG#v}"  # strip leading 'v' (Cargo expects numbers)
ROOT="$(git rev-parse --show-toplevel)"

echo "▶ Tagging current commit as '${TAG}' …"
if git rev-parse "$TAG" > / dev / null 2 > & 1; then
git tag -d "$TAG"  # overwrite if it already exists
fi
git tag -a "$TAG" -m "FastLanes release ${TAG}"

echo "▶ Updating Cargo.toml dependencies to version '${VERSION}' …"
changed_files=()
while IFS= read -r -d '' toml; do
if grep - qE '^\s*fls-rs\s*=' "$toml"; then
sed - Ei.bak
"s/^\s*fls-rs\s*=.*$/fls-rs = \"${VERSION}\"/" "$toml"
rm
"${toml}.bak"
changed_files += ("$toml")
fi
done < < (git - C "$ROOT" ls-files -z | grep -z --null-data 'Cargo.toml')

if ((${  # changed_files[@]} == 0 )); then
echo "⚠️  No Cargo.toml with 'fls-rs' dependency found." > & 2
else
git add "${changed_files[@]}"
git commit -m "Bump fls-rs dependency to ${VERSION}"
echo "✓ Updated and committed: ${changed_files[*]}"
fi

echo "All done. Push with:  git push --follow-tags"
