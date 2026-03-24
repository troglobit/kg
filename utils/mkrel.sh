#!/bin/sh
# Pre-release helper: verify version bumps and step debian/changelog.
# Usage: mkrel.sh (run from repo root via 'make release')

set -e

# Extract KG_VERSION from def.h
ver=$(sed -n 's/^#define KG_VERSION "\(.*\)"/\1/p' src/def.h)
if [ -z "$ver" ]; then
    echo "error: cannot extract KG_VERSION from src/def.h" >&2
    exit 1
fi

# Verify version was bumped since last tag
last_tag=$(git describe --tags --abbrev=0 2>/dev/null || true)
if [ "v${ver}" = "${last_tag}" ]; then
    echo "error: KG_VERSION ${ver} matches last tag ${last_tag} -- bump the version first" >&2
    exit 1
fi

# Verify ChangeLog has an entry for this version
if ! grep -qF "[v${ver}]" doc/ChangeLog.md; then
    echo "error: no entry for [v${ver}] found in doc/ChangeLog.md" >&2
    exit 1
fi

# Step debian/changelog if not already at this version
# Use ~ for pre-release suffixes so dpkg sorts them before the final release.
debver=$(echo "$ver" | sed 's/-\(alpha\|beta\|rc\)/~\1/')
debver="${debver}-1"
if head -1 debian/changelog | grep -qF "kg (${debver})"; then
    echo "debian/changelog already up to date for ${debver}"
else
    tmpfile=$(mktemp)
    cat > "$tmpfile" << EOF
kg (${debver}) unstable; urgency=low

  * New upstream release.

 -- $(git config user.name) <$(git config user.email)>  $(date -R)

EOF
    cat debian/changelog >> "$tmpfile"
    mv "$tmpfile" debian/changelog
    echo "Stepped debian/changelog to ${debver}"
fi

echo ""
echo "Ready to release v${ver}. Suggested next steps:"
echo "  git add src/def.h doc/ChangeLog.md debian/changelog"
echo "  git commit -m 'Prepare release v${ver}'"
echo "  git tag v${ver}"
echo "  git push && git push --tags"
