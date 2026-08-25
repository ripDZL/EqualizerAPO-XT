param(
  [string]$VersionHeader = "version.h",
  [switch]$Check
)

$ErrorActionPreference = "Stop"

function Invoke-Git {
  $safeDirectory = (Get-Location).Path -replace "\\", "/"
  & git -c "safe.directory=$safeDirectory" @args
}

# Audit #250 F069: the version.h parser lives in Get-VersionPart.ps1 (the
# file whose header claims it is written exactly once - now true again).
. (Join-Path $PSScriptRoot "Get-VersionPart.ps1")

function Get-LastVersionTag {
  $lastTag = ""
  try {
    $lastTag = (& Invoke-Git describe --tags --match "v[0-9]*" --abbrev=0 2>$null)
  } catch {
    $lastTag = ""
  }
  return $lastTag
}

function Get-BumpKind {
  param([string]$LastTag)
  $range = if ([string]::IsNullOrWhiteSpace($lastTag)) { "HEAD" } else { "$lastTag..HEAD" }
  $messages = @(& Invoke-Git log --format=%B $range)
  $joined = ($messages -join "`n")

  # Conventional Commits → SemVer mapping matching CLAUDE.md policy:
  #   docs/ci/chore/style/refactor/test/perf/build do not bump
  #   fix:                                            → patch
  #   feat:                                           → minor
  #   BREAKING CHANGE or `<type>!:`                   → major
  # Match the breaking change as an actual Conventional Commits footer
  # (uppercase, at the start of a line, with the colon) - or "BREAKING-CHANGE:" -
  # not as a bare substring. A commit that merely mentions the words in prose
  # (for example release tooling or a changelog entry that documents this rule)
  # must not trip a major bump; matching the bare substring is exactly what once
  # cut a spurious 2.0.0 release from a ci: commit whose body discussed the rule.
  if ($joined -cmatch "(^|\r?\n)BREAKING[ -]CHANGE:" -or $joined -match "(^|\n)\w+(\([^)]+\))?!:") {
    return "major"
  }
  if ($joined -match "(^|\n)feat(\([^)]+\))?:") {
    return "minor"
  }
  if ($joined -match "(^|\n)fix(\([^)]+\))?:") {
    return "patch"
  }
  return "none"
}

if (-not (Test-Path $VersionHeader)) {
  throw "Version header not found: $VersionHeader"
}

$lines = Get-Content -Path $VersionHeader
$major = Get-VersionPart "MAJOR" $lines
$minor = Get-VersionPart "MINOR" $lines
$revision = Get-VersionPart "REVISION" $lines

$lastTag = Get-LastVersionTag
$bumpKind = Get-BumpKind -LastTag $lastTag
$currentVersion = "$major.$minor.$revision"

# A portable beta is tagged with the version that will become stable, while
# version.h remains at the preceding development version. If main contains
# only docs after that prerelease, the old range calculation saw no fix/feat
# and skipped the stable release entirely. Promote a newer prerelease base
# before applying any post-beta Conventional Commit bump.
$promotingPrerelease = $false
if ($lastTag -match '^v(?<tagMajor>\d+)\.(?<tagMinor>\d+)\.(?<tagRevision>\d+)-[0-9A-Za-z][0-9A-Za-z.-]*$') {
  $tagMajor = [int]$Matches.tagMajor
  $tagMinor = [int]$Matches.tagMinor
  $tagRevision = [int]$Matches.tagRevision
  $tagIsNewer = ($tagMajor -gt $major) -or (($tagMajor -eq $major) -and (($tagMinor -gt $minor) -or (($tagMinor -eq $minor) -and ($tagRevision -gt $revision))))
  if ($tagIsNewer) {
    $major = $tagMajor
    $minor = $tagMinor
    $revision = $tagRevision
    $promotingPrerelease = $true
  }
}

if ($bumpKind -eq "none" -and -not $promotingPrerelease) {
  if ($Check) {
    Write-Host "No version-affecting commits since the last release; version stays at $currentVersion"
    exit 0
  }
  Write-Host "No version-affecting commits since the last release; leaving $VersionHeader at $currentVersion"
  exit 0
}

switch ($bumpKind) {
  "major" {
    $major += 1
    $minor = 0
    $revision = 0
  }
  "minor" {
    $minor += 1
    $revision = 0
  }
  "patch" {
    $revision += 1
  }
}

$nextLines = foreach ($line in $lines) {
  if ($line -match "^\s*#define\s+MAJOR\s+\d+\s*$") {
    "#define MAJOR $major"
  } elseif ($line -match "^\s*#define\s+MINOR\s+\d+\s*$") {
    "#define MINOR $minor"
  } elseif ($line -match "^\s*#define\s+REVISION\s+\d+\s*$") {
    "#define REVISION $revision"
  } else {
    $line
  }
}

$nextVersion = "$major.$minor.$revision"
if ($Check) {
  if ($promotingPrerelease -and $bumpKind -eq "none") {
    Write-Host "Next prerelease promotion version would be $nextVersion"
  } else {
    Write-Host "Next $bumpKind version would be $nextVersion"
  }
  exit 0
}

Set-Content -Path $VersionHeader -Value $nextLines -Encoding ASCII
if ($promotingPrerelease -and $bumpKind -eq "none") {
  Write-Host "Promoted prerelease $lastTag to stable version $nextVersion"
} else {
  Write-Host "Bumped $VersionHeader to $nextVersion using $bumpKind rule"
}
