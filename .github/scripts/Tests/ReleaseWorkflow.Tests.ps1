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
}
