#!/usr/bin/env bash
# check-refineps.sh - quick health and sync check for the RefinePS project (Lubuntu)
#
# Usage:
#   ./check-refineps.sh            quick check (about 5 seconds). Prints ONLY problems, then a verdict:
#                                  'All OK' or the list of items to look at
#   ./check-refineps.sh --verbose  also print every check that passed
#   ./check-refineps.sh --full     also checks the release files on GitHub match the
#                                  recorded hashes, and (if the ESP32 is plugged in)
#                                  verifies the chip matches the baseline image
#   ./check-refineps.sh --port /dev/ttyUSB1   use a different serial port with --full
#
# Reads only. It never commits, pushes, flashes or changes any file in the repo.
# Override the repo location with:  REFINEPS_REPO=/path/to/refineps ./check-refineps.sh

set -u

REPO="${REFINEPS_REPO:-$HOME/Downloads/refineps}"
REL_TAG="baseline-2026-11-20"
REL_URL="https://github.com/andrew1976fell/refineps/releases/download/$REL_TAG"
APK_SHA="2e2e64704c7d7c773d2cd478d51c1682d1be2d2e04b54337dfc87c5ff982c377"
BIN_SHA="8e3cb933a0ec1ca26794908d258efdc1f0c1306c5985dbc9c4e703ee6ab8b311"

FULL=0
VERBOSE=0
SEC=""
PORT=""
while [ $# -gt 0 ]; do
    case "$1" in
        --full) FULL=1 ;;
        -v|--verbose) VERBOSE=1 ;;
        --port) shift; PORT="${1:-}" ;;
        -h|--help) sed -n '2,16p' "$0"; exit 0 ;;
        *) echo "Unknown option: $1 (try --help)"; exit 2 ;;
    esac
    shift
done

WARNS=0
FAILS=0
ok()   { [ "$VERBOSE" -eq 1 ] && printf '  [ OK ] %s\n' "$*"; return 0; }
info() { [ "$VERBOSE" -eq 1 ] && printf '         %s\n' "$*"; return 0; }
skip() { [ "$VERBOSE" -eq 1 ] && printf '  [skip] %s\n' "$*"; return 0; }
warn() { printf '  [WARN] %s: %s\n' "$SEC" "$*"; WARNS=$((WARNS + 1)); }
fail() { printf '  [FAIL] %s: %s\n' "$SEC" "$*"; FAILS=$((FAILS + 1)); }
head_() { SEC="$*"; [ "$VERBOSE" -eq 1 ] && printf '\n== %s\n' "$*"; return 0; }

g() { git -C "$REPO" "$@"; }

echo "RefinePS check on $(hostname) - $(date '+%Y-%m-%d %H:%M')"

# ---------------------------------------------------------------- repo
head_ "Repository ($REPO)"
if [ ! -d "$REPO/.git" ]; then
    fail "no git repository at $REPO (set REFINEPS_REPO to the right folder)"
    echo; echo "Result: $FAILS fail, $WARNS warn"; exit 1
fi
ok "found"

branch="$(g rev-parse --abbrev-ref HEAD)"
[ "$branch" = "main" ] && ok "on branch main" || warn "on branch '$branch' (project policy is main only)"

if g branch -a | grep -q 'master'; then
    fail "a 'master' branch exists - repo policy is main only"
fi

# ---------------------------------------------------------------- sync
head_ "Sync with GitHub"
if timeout 30 git -C "$REPO" fetch --tags --quiet 2>/dev/null; then
    ok "fetched from GitHub"
else
    warn "could not reach GitHub (offline, or the fetch timed out) - results below may be stale"
fi

if g rev-parse --abbrev-ref '@{u}' >/dev/null 2>&1; then
    read -r ahead behind < <(g rev-list --left-right --count 'HEAD...@{u}')
    [ "$ahead" -eq 0 ] && ok "nothing unpushed" || fail "$ahead commit(s) NOT pushed to GitHub - push before relying on this machine"
    [ "$behind" -eq 0 ] && ok "up to date with GitHub" || warn "GitHub has $behind newer commit(s) - run: git pull"
else
    warn "branch has no upstream. Fix with: git branch --set-upstream-to=origin/main main"
fi

dirty="$(g status --porcelain | grep -v '^??' || true)"
untracked="$(g status --porcelain | grep -c '^??' || true)"
if [ -z "$dirty" ]; then
    ok "no uncommitted changes to tracked files"
else
    warn "uncommitted changes (the firmware may have been built from these):"
    printf '%s\n' "$dirty" | sed 's/^/           /'
fi
[ "$untracked" -gt 0 ] && info "($untracked untracked file(s), ignored)"

if g rev-parse -q --verify "refs/tags/$REL_TAG" >/dev/null; then
    ok "baseline tag $REL_TAG at $(g rev-parse --short "$REL_TAG^{commit}")"
else
    warn "baseline tag $REL_TAG is not in this clone - run: git fetch --tags"
fi

# ---------------------------------------------------------------- toolchain
head_ "Firmware toolchain"
recorded=""
[ -f "$REPO/firmware/TOOLCHAIN.md" ] && \
    recorded="$(grep -o 'ESP-IDF v[^ ]*' "$REPO/firmware/TOOLCHAIN.md" | head -1 | sed 's/ESP-IDF //')"
[ -n "$recorded" ] && ok "recorded in repo: $recorded" || warn "firmware/TOOLCHAIN.md missing or has no version (pull the latest?)"

idf=""
for d in "${IDF_PATH:-}" "$HOME"/esp-idf* "$HOME"/esp/esp-idf*; do
    [ -n "$d" ] && [ -d "$d/.git" ] && { idf="$d"; break; }
done
if [ -n "$idf" ]; then
    installed="$(git -C "$idf" describe --tags --always 2>/dev/null)"
    info "installed: $installed   ($idf)"
    if [ -n "$recorded" ] && [ "$installed" = "$recorded" ]; then
        ok "installed ESP-IDF matches the recorded one"
    elif [ -n "$recorded" ]; then
        fail "installed ESP-IDF differs from the recorded toolchain - a rebuild will not match the baseline"
    fi
else
    warn "no ESP-IDF checkout found under ~ (checked IDF_PATH, ~/esp-idf*, ~/esp/esp-idf*)"
fi

if [ -f "$REPO/firmware/sdkconfig" ]; then
    cfg="$(sed -n 's/.*(ESP-IDF) \([0-9.]*\) Project.*/\1/p' "$REPO/firmware/sdkconfig" | head -1)"
    [ -n "$cfg" ] && info "sdkconfig generated by ESP-IDF $cfg"
    if [ -n "$recorded" ] && [ -n "$cfg" ]; then
        case "$recorded" in
            "v${cfg%.*}"*) ok "sdkconfig version matches the recorded toolchain" ;;
            *) warn "sdkconfig was generated by ESP-IDF $cfg but the toolchain note says $recorded" ;;
        esac
    fi
fi

# ---------------------------------------------------------------- full checks
if [ "$FULL" -eq 1 ]; then
    head_ "Release files on GitHub (hash check)"
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' EXIT
    for pair in "refineps-v1.0-working.apk:$APK_SHA" "refineps_v1.0_full.bin:$BIN_SHA"; do
        f="${pair%%:*}"; want="${pair##*:}"
        if curl -fsSL -o "$tmp/$f" "$REL_URL/$f"; then
            got="$(sha256sum "$tmp/$f" | cut -d' ' -f1)"
            [ "$got" = "$want" ] && ok "$f hash matches" || fail "$f hash DIFFERS from the recorded one"
        else
            warn "could not download $f"
        fi
    done

    head_ "ESP32 against the baseline image"
    [ -z "$PORT" ] && PORT="$(ls /dev/ttyUSB* 2>/dev/null | head -1)"
    if [ -z "$PORT" ]; then
        skip "no /dev/ttyUSB* port - ESP32 not plugged in"
    elif [ ! -f "$tmp/refineps_v1.0_full.bin" ]; then
        warn "baseline image not downloaded, skipping"
    elif ! python3 -m esptool version >/dev/null 2>&1; then
        warn "esptool not available (python3 -m esptool)"
    else
        info "port $PORT - comparing, takes about 30 seconds"
        if out="$(python3 -m esptool --port "$PORT" --baud 115200 verify-flash 0x0 "$tmp/refineps_v1.0_full.bin" 2>&1)"; then
            ok "chip matches the baseline image exactly"
        elif printf '%s' "$out" | grep -qi 'differ\|mismatch\|verification failed'; then
            warn "chip differs from the baseline image - it is running different firmware"
        else
            warn "could not verify (port busy or chip not responding): $(printf '%s' "$out" | tail -1)"
        fi
    fi
fi

# ---------------------------------------------------------------- result
echo
if [ "$FAILS" -gt 0 ]; then
    echo "NEEDS ATTENTION: $FAILS problem(s), $WARNS note(s) - see above"; exit 1
elif [ "$WARNS" -gt 0 ]; then
    echo "REVIEW: $WARNS item(s) above need a look, nothing is broken"; exit 0
else
    echo "All OK - nothing to review"; exit 0
fi
