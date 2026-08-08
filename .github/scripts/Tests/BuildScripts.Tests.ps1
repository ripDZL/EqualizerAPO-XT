Describe "extracted build script decisions" {
    BeforeAll {
        $root = Join-Path $TestDrive "repo"
    }

    It "selects native ARM64 toolchain and suppresses unsupported runtime tests" {
        $plan = & (Join-Path $PSScriptRoot "..\Build-Solution.ps1") `
            -WorkspaceRoot $root -Platform ARM64 -SimdVariant neon -CanExecute:$false -PlanOnly
        $plan.PlatformToolset | Should -Be "v143"
        $plan.ToolArchitecture | Should -Be "ARM64"
        # A runner that cannot execute the variant runs nothing: EditorLogicTests
        # links Common.lib whole-archive and so now carries the variant's /arch
        # into a static initializer.
        $plan.RuntimeTests | Should -BeNullOrEmpty
        # No x86 cross-build on the ARM64 leg; the avx2 leg owns the installer.
        $plan.InstallerProject | Should -BeNullOrEmpty
    }

    It "builds the auto-detect installer on the avx2 leg only" {
        $avx2 = & (Join-Path $PSScriptRoot "..\Build-Solution.ps1") `
            -WorkspaceRoot $root -Platform x64 -SimdVariant avx2 -ArchFlag AdvancedVectorExtensions2 -PlanOnly
        $avx2.InstallerProject | Should -Be "Installer\Installer.vcxproj"
        $sse2 = & (Join-Path $PSScriptRoot "..\Build-Solution.ps1") `
            -WorkspaceRoot $root -Platform x64 -SimdVariant sse2 -ArchFlag NotSet -PlanOnly
        $sse2.InstallerProject | Should -BeNullOrEmpty
    }

    It "derives the AVX10 and ARM64 update channels" {
        $script = Join-Path $PSScriptRoot "..\Build-QtApps.ps1"
        (& $script -WorkspaceRoot $root -Platform x64 -SimdVariant avx10_1 `
            -QtArchFlag "/arch:AVX10.1" -MsvcDevPlatform amd64 -PlanOnly).UpdateChannel |
            Should -Be "x64-avx10-1"
        (& $script -WorkspaceRoot $root -Platform ARM64 -SimdVariant neon `
            -QtArchFlag "" -MsvcDevPlatform arm64 -PlanOnly).UpdateChannel |
            Should -Be "arm64-neon"
    }

    It "normalizes Visual Studio environment variable casing" {
        . (Join-Path $PSScriptRoot "..\Import-VsDevEnvironment.ps1")
        $name = "EAPO_PATH_CASE_TEST"
        $lowerName = "eapo_path_case_test"
        try {
            Set-Item -Path "Env:$name" -Value "old"
            Set-VsDevEnvironmentVariable -Name $lowerName -Value "new"

            @((Get-ChildItem Env: | Where-Object { $_.Name -ieq $name })).Count | Should -Be 1
            (Get-Item -Path "Env:$lowerName").Value | Should -Be "new"
        }
        finally {
            Remove-Item -LiteralPath "Env:$name" -ErrorAction SilentlyContinue
            Remove-Item -LiteralPath "Env:$lowerName" -ErrorAction SilentlyContinue
        }
    }

    It "keeps symbols and object files out of user artifacts" {
        $plan = & (Join-Path $PSScriptRoot "..\Package-Artifacts.ps1") `
            -WorkspaceRoot $root -Platform x64 -SimdVariant avx2 -PlanOnly
        $plan.RequiredFiles | Should -Contain "EqualizerAPO\x64\Release\EqualizerAPO.dll"
        $plan.ExcludedExtensions | Should -Contain ".pdb"
        $plan.ExcludedExtensions | Should -Contain ".obj"
        $plan.ExcludedExtensions | Should -Contain ".cpp"
        $plan.ExcludedExtensions | Should -Contain ".pch"
        $plan.ExcludedExtensions | Should -Contain ".qrc"
    }

    It "packages from a clean directory and leaves generated build files behind" {
        $repo = Join-Path $TestDrive "package-repo"
        $artifact = Join-Path $repo "artifacts\EqualizerAPO-x64-avx2"
        New-Item -ItemType Directory -Force -Path $artifact | Out-Null
        Set-Content -Path (Join-Path $artifact "stale.dll") -Value "old"

        $required = @(
            "EqualizerAPO\x64\Release\EqualizerAPO.dll",
            "Benchmark\x64\Release\Benchmark.exe",
            "VoicemeeterClient\x64\Release\VoicemeeterClient.exe",
            "deps\fftw\Release\libfftw3.dll",
            "deps\libsndfile\build\Release\sndfile.dll",
            "deps\velopack_libc\lib\velopack_libc_win_x64_msvc.dll",
            "VST3\SubwooferRouting\x64\Release\EapoXtSubwooferRouting.vst3",
            "VST3\SubwooferRouting\LICENSE"
        )
        foreach ($relative in $required) {
            $target = Join-Path $repo $relative
            New-Item -ItemType Directory -Force -Path (Split-Path $target -Parent) | Out-Null
            Set-Content -Path $target -Value "binary"
        }

        foreach ($app in @("Editor", "DeviceSelector", "UpdateChecker")) {
            $buildDir = Join-Path $repo "build-$app-x64\release"
            New-Item -ItemType Directory -Force -Path (Join-Path $buildDir "platforms") | Out-Null
            Set-Content -Path (Join-Path $buildDir "$app.exe") -Value "exe"
            Set-Content -Path (Join-Path $buildDir "Qt6Core.dll") -Value "dll"
            Set-Content -Path (Join-Path $buildDir "platforms\qwindows.dll") -Value "dll"
            Set-Content -Path (Join-Path $buildDir "moc_$app.cpp") -Value "generated"
            Set-Content -Path (Join-Path $buildDir "ui_$app.h") -Value "generated"
            Set-Content -Path (Join-Path $buildDir "$app.pch") -Value "generated"
            Set-Content -Path (Join-Path $buildDir "$app.qrc") -Value "generated"
            Set-Content -Path (Join-Path $buildDir "$app.obj") -Value "generated"
        }

        & (Join-Path $PSScriptRoot "..\Package-Artifacts.ps1") `
            -WorkspaceRoot $repo -Platform x64 -SimdVariant avx2

        Test-Path -LiteralPath (Join-Path $artifact "stale.dll") |
            Should -BeFalse -Because "reruns must not keep deleted files"
        Test-Path -LiteralPath (Join-Path $artifact "Editor.exe") | Should -BeTrue
        Test-Path -LiteralPath (Join-Path $artifact "platforms\qwindows.dll") | Should -BeTrue
        Test-Path -LiteralPath (Join-Path $artifact "VST3\EapoXtSubwooferRouting.vst3\Contents\x86_64-win\EapoXtSubwooferRouting.vst3") |
            Should -BeTrue
        Test-Path -LiteralPath (Join-Path $artifact "VST3\EapoXtSubwooferRouting.vst3\LICENSE") |
            Should -BeTrue
        Test-Path -LiteralPath (Join-Path $artifact "moc_Editor.cpp") |
            Should -BeFalse -Because "generated source files are not install artifacts"
        Test-Path -LiteralPath (Join-Path $artifact "ui_Editor.h") |
            Should -BeFalse -Because "generated headers are not install artifacts"
        Test-Path -LiteralPath (Join-Path $artifact "Editor.pch") |
            Should -BeFalse -Because "precompiled headers are not install artifacts"
        Test-Path -LiteralPath (Join-Path $artifact "Editor.qrc") |
            Should -BeFalse -Because "resource compiler inputs are not install artifacts"
    }
}
