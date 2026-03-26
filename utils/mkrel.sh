#!/bin/sh
# Release helper: verify version bumps, step debian/changelog, build artifacts.
# Usage: mkrel.sh (run from repo root via 'make release')
#
# Pre-tag mode  (HEAD not yet tagged with v<ver>):
#   Verifies the version is bumped and the ChangeLog is updated, steps
#   debian/changelog, then prints the suggested commit/tag/push commands.
#
# Release mode  (HEAD is tagged with v<ver>, e.g. on CI):
#   Ensures debian/changelog is current, runs the test suite, and builds
#   all release artifacts into artifacts/.

set -e

# Check for uncommitted changes early; warn at the end so it's not missed.
dirty_files=$(git status --short)
if [ -n "$dirty_files" ]; then
    dirty=1
else
    dirty=0
fi

ver=$(sed -n 's/^#define KG_VERSION "\(.*\)"/\1/p' src/def.h)
if [ -z "$ver" ]; then
    echo "error: cannot extract KG_VERSION from src/def.h" >&2
    exit 1
fi

# Determine mode based on whether HEAD carries an exact tag matching the
# version.  KG_VERSION in src/def.h holds the GA version (e.g. 1.1.0)
# even for pre-release tags (e.g. v1.1.0-rc1) — both ship as "the same
# release" semantically — so strip any -alpha/-beta/-rc suffix from the
# tag before comparing.  Fall back to GITHUB_REF when git describe can't
# see the tag (e.g. CI checkout).
cur_tag=$(git describe --exact-match HEAD 2>/dev/null || true)
[ -z "$cur_tag" ] && cur_tag=${GITHUB_REF#refs/tags/}
basetag=$(echo "${cur_tag}" | sed 's/-\(alpha\|beta\|rc\)[0-9]*$//')
if [ "v${ver}" = "${basetag}" ]; then
    mode=release
    # In release mode the tag is the source of truth for what's
    # actually being shipped — use it for the .deb version and the
    # tarball name so v1.1.0-rc1 produces kg~rc1 artifacts.
    relver=${cur_tag#v}
else
    last_tag=$(git describe --tags --abbrev=0 2>/dev/null || true)
    if [ "v${ver}" = "${last_tag}" ]; then
        echo "error: KG_VERSION ${ver} matches last tag ${last_tag} -- bump the version first" >&2
        exit 1
    fi
    mode=prerelease
    relver=$ver
fi

# ChangeLog must have an entry for the GA version (beta/rc tags share the GA entry).
basever=$(echo "$ver" | sed 's/-\(alpha\|beta\|rc\)[0-9]*$//')
if ! grep -qF "[v${basever}]" doc/ChangeLog.md; then
    echo "error: no entry for [v${basever}] found in doc/ChangeLog.md" >&2
    exit 1
fi

# Extract bullet points from the GA ChangeLog entry, formatted as Debian entries.
changes=$(awk -v ver="v${basever}" '
    /^## \[/  { in_ver = (index($0, "[" ver "]") > 0); next }
    !in_ver   { next }
    /^#/      { next }
    /^- /     { print "  * " substr($0, 3) }
    /^  /     { line = $0; sub(/^[[:space:]]+/, "", line); print "    " line }
' doc/ChangeLog.md)

# Step debian/changelog if not already at this version.
# Use ~ for pre-release suffixes so dpkg sorts them before the final release.
debver=$(echo "$relver" | sed 's/-\(alpha\|beta\|rc\)/~\1/')
debver="${debver}-1"
if head -1 debian/changelog | grep -qF "kg (${debver})"; then
    echo "debian/changelog already up to date for ${debver}"
else
    tmpfile=$(mktemp)
    cat > "$tmpfile" << EOF
kg (${debver}) unstable; urgency=low

${changes}

 -- $(git config user.name) <$(git config user.email)>  $(date -R)

EOF
    cat debian/changelog >> "$tmpfile"
    mv "$tmpfile" debian/changelog
    echo "Stepped debian/changelog to ${debver}"
fi

if [ "$mode" = "prerelease" ]; then
    echo ""
    echo "Ready to release v${ver}. Suggested next steps:"
    echo "  git add src/def.h doc/ChangeLog.md debian/changelog"
    echo "  git commit -m 'Prepare release v${ver}'"
    echo "  git tag v${ver}"
    echo "  git push && git push --tags"
    if [ "$dirty" -eq 1 ]; then
        echo ""
        echo "WARNING: uncommitted changes in working tree:"
        echo "$dirty_files"
	exit 1
    fi
    exit 0
fi

# Release mode: run tests then build all artifacts.
make check

name=kg-v${relver}
mkdir -p artifacts
git archive --format=tar.gz --prefix=${name}/ HEAD -o artifacts/${name}.tar.gz
echo "Created artifacts/${name}.tar.gz"

make deb
mv ../kg_*.deb artifacts/
echo "Moved .deb to artifacts/"

sh utils/genrel.sh doc/ChangeLog.md > artifacts/release.md
echo "Created artifacts/release.md"

echo ""
echo "Artifacts ready in artifacts/:"
ls -1 artifacts/
if [ "$dirty" -eq 1 ]; then
    echo ""
    echo "WARNING: uncommitted changes in working tree:"
    echo "$dirty_files"
fi
