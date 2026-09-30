Describe "release workflow target commit" {
    BeforeAll {
        $workflowPath = Join-Path $PSScriptRoot "..\..\workflows\build.yml"
        $workflow = Get-Content -LiteralPath $workflowPath -Raw
        $resolvedReleaseSha = [regex]::Escape('${{ needs.version-bump.outputs.bumped_sha || github.sha }}')
    }

    It "uses the bumped commit when creating the Velopack release" {
        $pattern = "(?s)New-VelopackRelease\.ps1.*?-TargetCommit\s+`"$resolvedReleaseSha`""
        [regex]::IsMatch($workflow, $pattern) | Should -BeTrue
    }

    It "uses the bumped commit when generating release notes" {
        $pattern = "(?s)New-ReleaseNotes\.ps1.*?-TargetCommit\s+`"$resolvedReleaseSha`""
        [regex]::IsMatch($workflow, $pattern) | Should -BeTrue
    }

    It "allows a manually versioned main build to publish after a skipped version bump" {
        $releaseBlock = [regex]::Match($workflow, "(?ms)^\s{2}create-release:.*?^\s{4}permissions:")
        $releaseBlock.Success | Should -BeTrue
        # YAML's folded scalar marker is formatting, not part of the condition.
        $condition = "(?s)if:\s*(?:>-\s*)?always\(\)\s*&&\s*github\.event_name\s*==\s*'push'\s*&&\s*github\.ref\s*==\s*'refs/heads/main'\s*&&\s*needs\.build\.result\s*==\s*'success'"
        [regex]::IsMatch($releaseBlock.Value, $condition) | Should -BeTrue
        $releaseBlock.Value | Should -Match "\(\s*needs\.version-bump\.result\s*==\s*'success'\s*\|\|\s*needs\.version-bump\.result\s*==\s*'skipped'\s*\)"
    }

    It "requires every blocking gate to pass before publishing" {
        $releaseBlock = [regex]::Match($workflow, "(?ms)^\s{2}create-release:.*?^\s{4}permissions:")
        $releaseBlock.Success | Should -BeTrue
        foreach ($gate in @("build", "memcheck", "capture-gate", "cppcheck", "pester")) {
            $releaseBlock.Value | Should -Match "&&\s*needs\.$gate\.result\s*==\s*'success'"
        }
    }

    It "builds a same-version prerelease promotion even when version.h is unchanged" {
        $workflow | Should -Match "id:\s*version-decision"
        $workflow | Should -Match "steps\.version-decision\.outputs\.release_required"
        $promotionBranch = '(?s)elseif\s*\(\s*\$releaseRequired\s*\)\s*\{.*?"bumped=true"\s*>>\s*\$env:GITHUB_OUTPUT'
        [regex]::IsMatch($workflow, $promotionBranch) | Should -BeTrue
    }
}
