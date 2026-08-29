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

    It "runs the ASIO suite where the variant executes and lists the ASIO projects" {
        $avx2 = & (Join-Path $PSScriptRoot "..\Build-Solution.ps1") `
            -WorkspaceRoot $root -Platform x64 -SimdVariant avx2 -ArchFlag AdvancedVectorExtensions2 -PlanOnly
        $avx2.RuntimeTests | Should -Contain "AsioTests"
        $avx2.Projects | Should -Contain "EqualizerAPOAsio\EqualizerAPOAsio.vcxproj"
        $avx2.Projects | Should -Contain "Tests\FakeAsioDriver\FakeAsioDriver.vcxproj"
        $avx2.Projects | Should -Contain "Tests\AsioProbe\AsioProbe.vcxproj"
    }

    It "plans the ASIO probe gate over the fake driver with an in-process and a DLL shape" {
        $plan = & (Join-Path $PSScriptRoot "..\Invoke-AsioProbeGate.ps1") -WorkspaceRoot $root -Platform x64 -PlanOnly
        $plan.Probe | Should -Be (Join-Path $root "Tests\AsioProbe\x64\Release\AsioProbe.exe")
        $plan.Config | Should -Be (Join-Path $root "Tests\AsioProbe\probe-config.txt")
        @($plan.Runs | Where-Object { $_.Arguments -contains "inproc" }).Count | Should -BeGreaterThan 0
        @($plan.Runs | Where-Object { $_.Arguments -contains "passthrough" }).Count | Should -BeGreaterThan 0
        foreach ($run in $plan.Runs) {
            if ($run.Arguments -contains "inproc") {
                $run.Arguments | Should -Contain "--max-late" -Because "$($run.Name) must refuse late blocks"
            }
        }
        # The pipelined run over the real host process is timing-bound on a
        # shared runner and gets more than one attempt; the deterministic
        # runs get exactly one, so a regression in them cannot hide.
        $pipelinedDll = $plan.Runs | Where-Object { $_.Name -eq "dll-daemon-exe-pipelined-float32-32" }
        $pipelinedDll.Attempts | Should -BeGreaterThan 1
        foreach ($run in $plan.Runs) {
            if ($run.Arguments -contains "--max-late") {
                ($run.PSObject.Properties["Attempts"]) | Should -BeNullOrEmpty -Because "$($run.Name) is deterministic"
            }
        }
    }

    It "lists the capture probes so every leg builds them" {
        $avx2 = & (Join-Path $PSScriptRoot "..\Build-Solution.ps1") `
            -WorkspaceRoot $root -Platform x64 -SimdVariant avx2 -ArchFlag AdvancedVectorExtensions2 -PlanOnly
        $avx2.Projects | Should -Contain "Tests\ApoHostProbe\ApoHostProbe.vcxproj"
        $avx2.Projects | Should -Contain "Tests\CaptureProbe\CaptureProbe.vcxproj"
    }

    It "plans the capture gate over a pinned virtual cable with the preamp measured three ways" {
        $plan = & (Join-Path $PSScriptRoot "..\Invoke-CaptureGate.ps1") -WorkspaceRoot $root -PlanOnly
        $plan.VbCableSha256 | Should -Match "^[0-9A-F]{64}$"
        $plan.VbCableUrl | Should -Match "^https://download\.vb-audio\.com/"
        $plan.RenderConnection | Should -Be "CABLE Input"
        $plan.CaptureConnection | Should -Be "CABLE Output"
        $plan.PreampDb | Should -BeLessThan 0
        $names = @($plan.Measurements | ForEach-Object { $_.Name })
        $names | Should -Be @("baseline", "apo-default", "apo-comms", "apo-raw", "after-uninstall")
        # The two that say "the APO processes a recording endpoint": a plain
        # recorder and a voice-chat stream, both expected at the preamp.
        foreach ($name in @("apo-default", "apo-comms")) {
            $m = $plan.Measurements | Where-Object { $_.Name -eq $name }
            $m.ExpectGainDb | Should -Be $plan.PreampDb
            $m.Required | Should -BeTrue
        }
        # Raw mode bypasses stream effects by design, so it is noted, not gated.
        ($plan.Measurements | Where-Object { $_.Name -eq "apo-raw" }).Required | Should -BeFalse
        ($plan.Measurements | Where-Object { $_.Name -eq "after-uninstall" }).ExpectGainDb | Should -Be 0
        # One install/measure/uninstall round per install mode: the one the
        # product picks on its own first, then each slot pair by name.
        @($plan.InstallModes | ForEach-Object { $_.Name }) | Should -Be @("default", "sfx-efx", "sfx-mfx", "lfx-gfx")
        ($plan.InstallModes | Where-Object { $_.Name -eq "default" }).Arguments.Count | Should -Be 0
        # The product's own choice is the gate; the named slots are the
        # evidence (a legacy driver is fed through LFX only, so a named SFX
        # round legitimately reads unity).
        ($plan.InstallModes | Where-Object { $_.Name -eq "default" }).Required | Should -BeTrue
        foreach ($named in @("sfx-efx", "sfx-mfx", "lfx-gfx")) {
            ($plan.InstallModes | Where-Object { $_.Name -eq $named }).Required | Should -BeFalse
        }
        # The low-latency round: the playback side with a convolution in the
        # config, the small period fresh and after a switch, both at the
        # preamp and both gated (the script itself skips them, and says so,
        # on a driver that declares no small period).
        @($plan.LowLatency | ForEach-Object { $_.Name }) | Should -Be @("ll-default", "ll-min", "ll-min-switch", "ll-after-uninstall")
        foreach ($name in @("ll-min", "ll-min-switch")) {
            $m = $plan.LowLatency | Where-Object { $_.Name -eq $name }
            $m.Period | Should -Be "min"
            $m.ExpectGainDb | Should -Be $plan.PreampDb
            $m.Required | Should -BeTrue
        }
        ($plan.LowLatency | Where-Object { $_.Name -eq "ll-min" }).HoldDefault | Should -BeFalse
        ($plan.LowLatency | Where-Object { $_.Name -eq "ll-min-switch" }).HoldDefault | Should -BeTrue
        ($plan.LowLatency | Where-Object { $_.Name -eq "ll-default" }).Period | Should -Be "default"
        ($plan.LowLatency | Where-Object { $_.Name -eq "ll-after-uninstall" }).ExpectGainDb | Should -Be 0
    }

    It "builds only the two 32-bit ASIO DLLs for Win32" {
        $plan = & (Join-Path $PSScriptRoot "..\Build-AsioWin32.ps1") -WorkspaceRoot $root -PlanOnly
        $plan.Projects | Should -Be @("EqualizerAPOAsio\EqualizerAPOAsio.vcxproj", "Tests\FakeAsioDriver\FakeAsioDriver.vcxproj")
        $plan.BuildParams | Should -Contain "/p:Platform=Win32"
        $plan.Outputs | Should -Contain (Join-Path $root "EqualizerAPOAsio\Release\EqualizerAPOAsio.dll")
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
            "EqualizerAPOAsio\x64\Release\EqualizerAPOAsio.dll",
            "EqualizerAPOHost\x64\Release\EqualizerAPOHost.exe",
            "EqualizerAPOAsio\Release\EqualizerAPOAsio.dll",
            "deps\fftw\Release\libfftw3.dll",
            "deps\libsndfile\build\Release\sndfile.dll",
            "deps\velopack_libc\lib\velopack_libc_win_x64_msvc.dll",
            "VST3\SubwooferRouting\x64\Release\EapoXtSubwooferRouting.vst3",
            "VST3\SubwooferRouting\LICENSE",
            "License.txt",
            "License-gpl-3.0.txt"
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
        Test-Path -LiteralPath (Join-Path $artifact "x86\EqualizerAPOAsio.dll") |
            Should -BeTrue -Because "the x64 channel ships its Win32 ASIO wrapper"
        Test-Path -LiteralPath (Join-Path $artifact "License-gpl-3.0.txt") |
            Should -BeTrue -Because "the ASIO wrapper is distributed under GPLv3"
        Test-Path -LiteralPath (Join-Path $artifact "VST3\EapoXtSubwooferRouting.vst3\Contents\x86_64-win\EapoXtSubwooferRouting.vst3") |
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

    It "keeps the Qt build's precompiled headers and generated sources out of user artifacts" {
        # v2.38.0 to v2.48.0 shipped them: 887 MB of .pch and 119 MB of
        # moc_/qrc_ sources unpacked, a ~250 MB installer instead of ~65 MB.
        $plan = & (Join-Path $PSScriptRoot "..\Package-Artifacts.ps1") `
            -WorkspaceRoot $root -Platform x64 -SimdVariant avx2 -PlanOnly
        foreach ($extension in @(".pch", ".cpp", ".h")) {
            $plan.ExcludedExtensions | Should -Contain $extension
        }
    }
}
