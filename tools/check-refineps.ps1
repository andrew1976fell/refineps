# check-refineps.ps1 - quick health and sync check for the RefinePS project (Windows PC)
#
# Usage (PowerShell):
#   .\check-refineps.ps1                 quick check (about 5 seconds). Prints ONLY problems, then a
#                                        verdict: 'All OK' or the list of items to look at
#   .\check-refineps.ps1 -Detail       also print every check that passed
#   .\check-refineps.ps1 -Full           also checks the release files on GitHub match the recorded
#                                        hashes, and (if plugged in) the phone's installed APK and
#                                        the ESP32's flash against the baseline
#   .\check-refineps.ps1 -Full -Port COM5   choose the serial port yourself
#
# Reads only. It never commits, pushes, flashes or changes any file in the repo.
# If scripts are blocked, run once:  Set-ExecutionPolicy -Scope CurrentUser RemoteSigned

param(
    [switch]$Full,
    [switch]$Detail,
    [string]$Port = "",
    [string]$Repo = "C:\Users\battery.tech\AndroidStudioProjects\refineps"
)

$RelTag = "baseline-2026-11-20"
$RelUrl = "https://github.com/andrew1976fell/refineps/releases/download/$RelTag"
$ApkSha = "2e2e64704c7d7c773d2cd478d51c1682d1be2d2e04b54337dfc87c5ff982c377"
$BinSha = "8e3cb933a0ec1ca26794908d258efdc1f0c1306c5985dbc9c4e703ee6ab8b311"
$Adb    = Join-Path $env:LOCALAPPDATA "Android\Sdk\platform-tools\adb.exe"

$script:Warns = 0
$script:Fails = 0
$script:Sec = ""
function Ok($m)   { if ($Detail) { Write-Host "  [ OK ] $m" -ForegroundColor Green } }
function Info($m) { if ($Detail) { Write-Host "         $m" } }
function Skip($m) { if ($Detail) { Write-Host "  [skip] $m" } }
function Warn($m) { Write-Host "  [WARN] ${script:Sec}: $m" -ForegroundColor Yellow; $script:Warns++ }
function Fail($m) { Write-Host "  [FAIL] ${script:Sec}: $m" -ForegroundColor Red;    $script:Fails++ }
function Section($m) { $script:Sec = $m; if ($Detail) { Write-Host ""; Write-Host "== $m" } }
function Gitr { & git -C $Repo @args 2>$null }

Write-Host "RefinePS check on $env:COMPUTERNAME - $(Get-Date -Format 'yyyy-MM-dd HH:mm')"

# ---------------------------------------------------------------- repo
Section "Repository ($Repo)"
if (-not (Test-Path (Join-Path $Repo ".git"))) {
    Fail "no git repository at $Repo (use -Repo to point at the right folder)"
    Write-Host ""; Write-Host "NEEDS ATTENTION: repository not found"; exit 1
}
Ok "found"

$branch = (Gitr rev-parse --abbrev-ref HEAD | Out-String).Trim()
if ($branch -eq "main") { Ok "on branch main" } else { Warn "on branch '$branch' (project policy is main only)" }

if ((Gitr branch -a | Out-String) -match "master") { Fail "a 'master' branch exists - repo policy is main only" }

# ---------------------------------------------------------------- sync
Section "Sync with GitHub"
Gitr fetch --tags --quiet
if ($LASTEXITCODE -eq 0) { Ok "fetched from GitHub" } else { Warn "could not reach GitHub - results below may be stale" }

$up = (Gitr rev-parse --abbrev-ref '@{u}' | Out-String).Trim()
if ($up) {
    $counts = ((Gitr rev-list --left-right --count 'HEAD...@{u}' | Out-String).Trim()) -split "\s+"
    $ahead = [int]$counts[0]; $behind = [int]$counts[1]
    if ($ahead -eq 0)  { Ok "nothing unpushed" } else { Fail "$ahead commit(s) NOT pushed to GitHub - push before relying on this machine" }
    if ($behind -eq 0) { Ok "up to date with GitHub" } else { Warn "GitHub has $behind newer commit(s) - run: git pull" }
} else {
    Warn "branch has no upstream. Fix with: git branch --set-upstream-to=origin/main main"
}

$dirty = @(Gitr status --porcelain | Where-Object { $_ -notmatch '^\?\?' })
if ($dirty.Count -eq 0) { Ok "no uncommitted changes to tracked files" }
else {
    Warn "uncommitted changes:"
    $dirty | ForEach-Object { Write-Host "           $_" }
}

$tagCommit = (Gitr rev-parse --short "refs/tags/$RelTag^{commit}" | Out-String).Trim()
if ($tagCommit) { Ok "baseline tag $RelTag at $tagCommit" } else { Warn "baseline tag $RelTag is not in this clone - run: git fetch --tags" }

# ---------------------------------------------------------------- toolchain note
Section "Firmware toolchain (builds happen on the Lubuntu machine)"
$tc = Join-Path $Repo "firmware\TOOLCHAIN.md"
if (Test-Path $tc) {
    $m = Select-String -Path $tc -Pattern 'ESP-IDF (v\S+)' | Select-Object -First 1
    if ($m) { Ok "recorded in repo: $($m.Matches[0].Groups[1].Value)" } else { Warn "TOOLCHAIN.md has no version line" }
} else {
    Warn "firmware\TOOLCHAIN.md missing (run: git pull)"
}

# ---------------------------------------------------------------- full checks
if ($Full) {
    Section "Release files on GitHub (hash check)"
    $tmp = Join-Path $env:TEMP "refineps-check"
    New-Item -ItemType Directory -Force $tmp | Out-Null
    $files = @(@("refineps-v1.0-working.apk", $ApkSha), @("refineps_v1.0_full.bin", $BinSha))
    foreach ($f in $files) {
        $dest = Join-Path $tmp $f[0]
        try {
            Invoke-WebRequest "$RelUrl/$($f[0])" -OutFile $dest -UseBasicParsing
            $got = (Get-FileHash $dest -Algorithm SHA256).Hash.ToLower()
            if ($got -eq $f[1]) { Ok "$($f[0]) hash matches" } else { Fail "$($f[0]) hash DIFFERS from the recorded one" }
        } catch {
            Warn "could not download $($f[0])"
        }
    }

    Section "Phone (installed APK against the baseline)"
    if (-not (Test-Path $Adb)) {
        Warn "adb not found at $Adb - skipping"
    } else {
        $dev = @(& $Adb devices | Select-String "`tdevice$")
        if ($dev.Count -eq 0) {
            Skip "no phone connected over USB"
        } else {
            $path = ((& $Adb shell pm path com.andrewscrap.refineps | Out-String).Trim() -replace '^package:', '')
            if (-not $path) {
                Warn "RefinePS app is not installed on the phone"
            } else {
                $pulled = Join-Path $tmp "installed.apk"
                & $Adb pull $path $pulled 2>$null | Out-Null
                $got = (Get-FileHash $pulled -Algorithm SHA256).Hash.ToLower()
                if ($got -eq $ApkSha) { Ok "installed APK is the baseline APK" }
                else { Warn "installed APK differs from the baseline (newer build, or a different one)" }
            }
        }
    }

    Section "ESP32 against the baseline image"
    if (-not $Port) {
        $ports = @([System.IO.Ports.SerialPort]::GetPortNames())
        if ($ports.Count -eq 1) { $Port = $ports[0] }
        elseif ($ports.Count -gt 1) { Warn "several serial ports ($($ports -join ', ')) - rerun with -Port COMx" }
        else { Skip "no serial port - ESP32 not plugged in" }
    }
    $img = Join-Path $tmp "refineps_v1.0_full.bin"
    if ($Port -and (Test-Path $img)) {
        Info "port $Port - comparing, takes about 30 seconds"
        $out = & python -m esptool --port $Port --baud 115200 verify-flash 0x0 $img 2>&1 | Out-String
        if ($LASTEXITCODE -eq 0) { Ok "chip matches the baseline image exactly" }
        elseif ($out -match "(?i)differ|mismatch|verification failed") { Warn "chip differs from the baseline image - it is running different firmware" }
        else { Warn "could not verify (port busy or chip not responding)" }
    }
}

# ---------------------------------------------------------------- result
Write-Host ""
if ($script:Fails -gt 0) { Write-Host "NEEDS ATTENTION: $($script:Fails) problem(s), $($script:Warns) note(s) - see above" -ForegroundColor Red; exit 1 }
elseif ($script:Warns -gt 0) { Write-Host "REVIEW: $($script:Warns) item(s) above need a look, nothing is broken" -ForegroundColor Yellow; exit 0 }
else { Write-Host "All OK - nothing to review" -ForegroundColor Green; exit 0 }
