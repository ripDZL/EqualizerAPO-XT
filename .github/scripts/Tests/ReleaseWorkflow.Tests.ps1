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
        $condition = "(?s)if:\s*always\(\)\s*&&\s*github\.event_name\s*==\s*'push'\s*&&\s*github\.ref\s*==\s*'refs/heads/main'\s*&&\s*needs\.build\.result\s*==\s*'success'"
        [regex]::IsMatch($releaseBlock.Value, $condition) | Should -BeTrue
    }
}
