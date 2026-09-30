Describe "ARM64 v143 ATL provisioning" {
    BeforeAll {
        $script = Join-Path $PSScriptRoot '..\Ensure-Arm64Atl.ps1'
    }
    BeforeEach {
        $vs = Join-Path $TestDrive 'Visual Studio'
        $tools = Join-Path $vs 'VC\Tools\MSVC\14.44.35207'
        $library = Join-Path $tools 'atlmfc\lib\arm64\atls.lib'
        $installer = Join-Path $TestDrive 'setup.exe'
        New-Item -ItemType Directory -Path $tools -Force | Out-Null
        Set-Content -LiteralPath $installer -Value ''
        Mock Start-Process { throw 'Installer must not run in a plan or ready environment' }
    }
    AfterEach {
        Remove-Item -LiteralPath $vs -Recurse -Force
    }

    It "does not modify a complete toolchain" {
        New-Item -ItemType Directory -Path (Split-Path $library) -Force | Out-Null
        Set-Content -LiteralPath $library -Value 'fixture'
        $plan = & $script -VisualStudioPath $vs -InstallerPath (Join-Path $TestDrive 'absent.exe')
        $plan.Action | Should -Be 'Ready'
        $plan.Library | Should -Be $library
        Should -Invoke Start-Process -Times 0
    }

    It "pins the v143 component even when v145 is also installed" {
        $v145Atl = Join-Path $vs 'VC\Tools\MSVC\14.50.35717\atlmfc\lib\arm64'
        New-Item -ItemType Directory -Path $v145Atl -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $v145Atl 'atls.lib') -Value 'wrong toolchain'
        $plan = & $script -VisualStudioPath $vs -InstallerPath $installer -PlanOnly
        $plan.Action | Should -Be 'Install'
        $plan.Component | Should -Be 'Microsoft.VisualStudio.Component.VC.14.44.17.14.ATL.ARM64'
        $plan.Library | Should -Be $library
        $plan.Arguments | Should -Contain ('"{0}"' -f $vs)
        $plan.Arguments | Should -Contain '--norestart'
        $plan.Arguments | Should -Not -Contain '--wait'
        Should -Invoke Start-Process -Times 0
    }

    It "fails before installation when the matching compiler is absent" {
        Remove-Item -LiteralPath $tools -Recurse -Force
        { & $script -VisualStudioPath $vs -InstallerPath $installer -PlanOnly } |
            Should -Throw '*v143 14.44*'
    }

    It "fails clearly when the installer is missing" {
        { & $script -VisualStudioPath $vs -InstallerPath (Join-Path $TestDrive 'absent.exe') } |
            Should -Throw '*installer not found*'
        Should -Invoke Start-Process -Times 0
    }

    It "rejects an installer error" {
        Mock Start-Process { [pscustomobject]@{ ExitCode = 1603 } }
        { & $script -VisualStudioPath $vs -InstallerPath $installer } | Should -Throw '*1603*'
        Should -Invoke Start-Process -Times 1 -ParameterFilter { $Wait -and $PassThru -and $WindowStyle -eq 'Hidden' }
    }

    It "does not trust a successful installer without the library" {
        Mock Start-Process { [pscustomobject]@{ ExitCode = 0 } }
        { & $script -VisualStudioPath $vs -InstallerPath $installer } | Should -Throw '*atls.lib still missing*'
    }

    It "verifies the installed library without restarting on exit 3010" {
        Mock Start-Process {
            New-Item -ItemType Directory -Path (Split-Path $library) -Force | Out-Null
            Set-Content -LiteralPath $library -Value 'fixture'
            [pscustomobject]@{ ExitCode = 3010 }
        }
        $plan = & $script -VisualStudioPath $vs -InstallerPath $installer
        $plan.Action | Should -Be 'Installed'
        Test-Path -LiteralPath $plan.Library | Should -BeTrue
    }

    It "accepts exit zero only after verifying the matching library" {
        Mock Start-Process {
            New-Item -ItemType Directory -Path (Split-Path $library) -Force | Out-Null
            Set-Content -LiteralPath $library -Value 'fixture'
            [pscustomobject]@{ ExitCode = 0 }
        }
        (& $script -VisualStudioPath $vs -InstallerPath $installer).Action | Should -Be 'Installed'
    }

    It "provisions only the ARM64 leg before the native solution build" {
        $workflow = Get-Content (Join-Path $PSScriptRoot '..\..\workflows\build.yml') -Raw
        $workflow | Should -Match '(?s)name: Ensure ARM64 v143 ATL\s+if: matrix.platform == ''ARM64''.*?Ensure-Arm64Atl.ps1'
        $workflow.IndexOf('name: Ensure ARM64 v143 ATL') | Should -BeLessThan $workflow.IndexOf('name: Build solution')
    }
}
