#!/bin/sh
# Extract and unwrap the latest ChangeLog entry for use as release notes.
# Usage: genrel.sh [ChangeLog.md]

changelog=${1:-ChangeLog.md}

awk '
/^\[v[0-9]/ && !found { found=1; skip_hr=1; next }
found && skip_hr && /^-{3,}/ { skip_hr=0; next }
found && /^\[/ {
    if (hold != "") { lines[++n] = hold; hold = "" }
    exit
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
}
' "$changelog"
