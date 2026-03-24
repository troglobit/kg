#!/bin/sh
# Extract and unwrap the latest ChangeLog entry for use as release notes.
# Usage: genrel.sh [ChangeLog.md]

changelog=${1:-doc/ChangeLog.md}

awk '
/^\[[^]]+\]:/ { refs[++nrefs] = $0 }
/^## \[v[0-9]/ && !found { found=1; next }
found && /^## / {
    if (hold != "") { lines[++n] = hold; hold = "" }
    found = 0; next
}
found && /^\[/ {
    if (hold != "") { lines[++n] = hold; hold = "" }
    found = 0; next
}
found {
    line = $0
    if (line == "") {
        if (hold != "") { lines[++n] = hold; hold = "" }
        lines[++n] = ""
    } else if (hold == "") {
        hold = line
    } else {
        c = substr(line, 1, 1)
        if (c == "#" || c == "-" || c == "[" || c == ">") {
            lines[++n] = hold
            hold = line
        } else {
            sub(/[[:space:]]+$/, "", hold)
            sub(/^[[:space:]]+/, "", line)
            hold = hold " " line
        }
    }
}
END {
    if (hold != "") lines[++n] = hold
    first = 1; last = n
    while (first <= last && lines[first] == "") first++
    while (last >= first && lines[last] == "") last--
    for (i = first; i <= last; i++) print lines[i]
    content = ""
    for (i = first; i <= last; i++) content = content "\n" lines[i]
    added = 0
    for (i = 1; i <= nrefs; i++) {
        label = refs[i]; sub(/^\[/, "", label); sub(/\].*$/, "", label)
        if (index(content, "[" label "]") > 0) {
            if (!added) { print ""; added = 1 }
            print refs[i]
        }
    }
}
' "$changelog"
