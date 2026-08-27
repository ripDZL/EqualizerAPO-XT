# Progress

- [x] 2026-08-27 fast-forwarded `beta` to `76204c` after the v2.42.5 stable release. User policy recorded: develop/test through `beta`, then promote it to `main` only with explicit approval for the stable-release workflow.

- [x] 2026-08-27 published stable `v2.42.5` from `4b723927514c3df28d1108fd9c22dc1c76cc9fca` with 33 release assets. GitHub Actions run `33029195723` passed cppcheck, version bump, matrix preparation, memcheck, Pester, six native variants, cross-variant comparison, and release creation/completeness. It ships the live VST-preview and Program Files installer repairs.

- [x] 2026-08-26 user reports the installed AVX-512 Editor candidate restores the missing normal live VST preview. This is the reported runtime acceptance result; Bertom Denoiser Classic's deliberate native-popup feed exception is unchanged and was not re-tested in this pass.

- [x] 2026-08-26 installed the user-approved live-preview AVX-512 Editor-only candidate into `C:\Program Files\EqualizerAPO-XT-x64-avx512\current\Editor.exe`. Previous SHA-256 `02AD1CB1840AA96E90EB25EA171B80E72577048FD7326C22E3093691FE727B1C` is backed up at `artifacts\install-backups\live-preview-popup-avx512-20260826-204754\Editor.exe`; installed SHA-256 `8185116EFFEEB26B9B472550A6FCFD914172AF73ED5E7630A0D92BDB8717CF54` matches the candidate. Engine, configuration, plug-ins, and audio services are unchanged. Manual runtime verification is next.

- [x] 2026-08-26 live VST-preview regression: `083d138e` narrowed popup preview to embedded panels while fixing Bertom Denoiser Classic, removing normal separate-panel microphone/analyzer updates. Added a narrow popup policy used by both LegacyRows and modern cards: normal native panels regain the feed, while only `Bertom_DenoiserClassic.vst3` remains protected. The focused policy test failed at 1/4318 before the repair and passes at 4319; AVX-512 Editor Release build 2.42.4.0 SHA-256 `8185116EFFEEB26B9B472550A6FCFD914172AF73ED5E7630A0D92BDB8717CF54` passes. No installed files changed; manual runtime verification remains.

- [x] 2026-08-26 local test candidate: rebuilt the repaired universal `EqualizerAPO-XT-Setup.exe` (2.42.4.0, SHA-256 `4B39DF838B213F639759C3767561B20DF3DB26C0D410F8BB8BC729E4E2C1CCBB`) and its no-install `--detect-only` check exits 0. It is ready for explicit user-approved Program Files install testing; no installed files changed.

- [x] 2026-08-26 universal Setup x86 Program Files repair: reproduced the shown pre-download failure as `FOLDERID_ProgramFilesX64 = 0x80070002` only under x86; x64 and x86 `/reg:64` both resolve `C:\Program Files`. The front door now falls back only to the protected native `ProgramFilesDir` registry value. Resolver regression coverage, Win32 Installer Release build, and EditorLogicTests 4313 pass. No installed files were touched; user package test remains.

- [x] 2026-08-26 installer-scope diagnosis: the local release-test helper downloaded the per-user channel Setup EXE rather than the per-machine MSI, so v2.42.4 was created under AppData. The helper now defaults to the MSI, preserves `-InstallerKind PerUser` for CI repro jobs, and resolves the machine target as `C:\Program Files\EqualizerAPO-XT-<channel>`. Published MSI metadata confirms `ALLUSERS=1` plus `VELOPACK_INSTALLDIR`; local release-asset selection/checksum checks, Installer build, and EditorLogicTests 4310 pass. Existing AppData files were not touched.

- [x] 2026-08-25 Denoiser Classic popup-panel repair: stopped the optional live analyzer worker for pop-out native panels, which shared the controller instance and reproduced an access violation. Embedded panels retain the feed. Exact raw-module probe covers both LegacyRows and modern cards; Release Editor build, EditorLogic 4310, and HybridConv 1635 pass. User-approved Editor-only test build installed; on 2026-08-26 the user confirmed open/close works in both UI modes.

- 2026-08-25: published user-approved `v2.42.3-beta.1` from `28d868ae` after green run `32857546525` (Pester 5, cppcheck, memcheck, six variants, UI gates, cross-variant comparison). The 27-asset prerelease has a verified beta-pinned universal Setup, six system-wide MSIs/update feeds/full packages, source, and SHA256SUMS; repository completeness passes. No runtime install occurred. Next gate: manually verify it installs only to `C:\Program Files\EqualizerAPO-XT\<channel>` while preserving legacy/per-user installs.

- [x] Diagnosed and corrected the modern/Legacy split-menu theme regression: the initial 18px menu face/12px chevron was too large in the live UI, so generated token QSS now uses an 8px transparent face with a 6px chevron. The focused test failed with 120 geometry assertions before the correction and passes with EditorLogicTests 4283. Legacy VST status text uses active skin tokens. Clean AVX-512 Editor rebuilt; installed visual acceptance remains open because forced-offscreen Qt crashes locally.
- [x] Backed up the prior test Editor (`F13921F032A5BCE8DA90A197944E1F1900E4EC20E65EF0023E219502219FE22E`) and installed the compact-arrow Editor into the active per-user AVX-512 runtime; the installed hash is `6B24EFD87C122F405F37DD31312D5681A4FB8FE7F0ADE89E869B21510C095210`.
- [x] Fetched upstream v2.39.0 (`ff9c174`).
- [x] Evaluated a direct `beta` merge; 149 conflicts made it unsuitable.
- [x] Created local upstream-first feature-port branch.
- [x] Ported VST live analyzer preview and microphone-source support.
- [x] Ported selected EAPO-endpoint preview selection and regression coverage.
- [x] Ported row-scoped `Device:` resolution for VST preview capture.
- [x] Ported VST2 transport-time/process-level safety with a red/green host test.
- [x] Ported block-paced VST preview processing with join-before-stop lifecycle ordering.
- [x] Ported saved custom-theme persistence, token derivation, and JSON round-trips.
- [x] Ported saved-theme application, Theme Lab UI, live token preview, and current-skin display names.
- [x] Kept temporary previews out of persisted settings and retained base-skin chrome for saved custom themes.
- [x] Ported Midnight Console, Arctic Bloom, Ember Rack, Violet Pulse, and Solar Paper with their upstream painter/QSS bases.
- [x] Ported Obsidian Glass, Aurora Veil, Copper Forge, Neon Nebula, and Noir Chrome with the same scoped painter/QSS mapping.
- [x] Kept all token variants on their matching DeviceSelector base painters; 90 headless shots passed.
- [x] Ported DeviceSelector saved custom-theme application and a temporary-settings probe; 93 headless shots passed.
- [x] Ported LegacyRows token chrome and five legacy palettes while retaining the legacy row factory chain; 30 headless gallery shots passed.
- [x] Kept the expanded 20-theme shared roster on the Minimal DeviceSelector painter where appropriate; 123 headless shots passed.
- [x] Audited the remaining fork-only feature groups against current upstream; only direct VST3 bundle import remained substantive.
- [x] Ported direct VST3 bundle import with a dedicated folder picker, rejection that preserves the active reference, staged replacement, host-module/reparse/traversal rejection, UI warnings, and 3415 EditorLogic checks.
- [x] Included VST2 no-native-editor panel safety; the forced native rebuild passed VST2/VST3 host coverage.
- [x] Restored signed/leading-decimal legacy VST parameter parsing with a regression test.
- [x] Restored Visual Studio environment casing and clean-artifact packaging safeguards with Pester 5 coverage.
- [x] Repaired upstream's duplicate `AudioEngineAccess.cpp` entry in `EditorLogicTests.vcxproj`; configured local build and 3415 checks passed.
- [x] Pushed validated integration branch to `origin/codex/upstream-289-port`; no upstream merge or installation is included.
- [x] Corrected the skin-module CI gate to distinguish token variants from concrete `paintBaseId` modules; the local gate passes for 5 modules across 20 themes.
- [x] Corrected the skin-gallery expanded-dialog assertion to use the active painter base; a rebuilt full gallery passes 5,840 shots across 20 themes and its CI harness now captures diagnostics.
- [x] Built Editor and passed offscreen `Editor.exe --selftest-vst`.
- [x] Passed offscreen `Editor.exe --skin-switch-test` (120 switches).
- [x] Built and passed `EditorLogicTests.exe` (3371 checks).
- [x] Built and passed `HybridConvTests.exe` (1635 checks).
- [x] Reproduced real VST3 factory-host rejection: strict `IPluginFactory3::setHostContext` handling rejected FabFilter/iZotope `kNotImplemented`; a focused red/green host test now passes (103 VST3 host checks; 1635 native checks total).
- [x] Swept installed production VST3 libraries with the repaired loader: 79/80 pass. The old `win-rnnoise\\rnnoise.vst3` bundle is an ACL-level access-denied failure; the configured newer RNNoise bundle passes.
- [x] Built and hash-verified the AVX-512 `Editor.exe` and `EqualizerAPO.dll`, then installed only those two files into the registered test runtime with a workspace backup. The fresh Editor VST self-test timed out at 30 seconds and is not passing evidence.
- [x] Probed the exact FabFilter Pro-Q 4 raw VST3 module through library and component/controller initialization: it passes with 2 inputs and 2 outputs. Repaired the two AVX-512 test shortcuts that pointed at a stale sandbox profile; the Program Files shortcut remains the baseline build.
- [x] Reproduced Qt LegacyRows toolbar clipping: only Add was visible at 33px. `CompactToolBar` now reserves its full base-toolbar height; the red/green probe shows Add, Remove, and Edit visible. Installed a hash-verified Editor-only test overlay with this repair.
- [ ] Manually validate real VST2/VST3 analyzer movement before an integration-candidate install.
- [ ] Manually validate that a previously failing FabFilter or iZotope VST3 adds, loads, and opens without "library could not be loaded" in the installed test overlay.
- [ ] Manually validate FabFilter Pro-Q 4 and LegacyRows removal through the repaired `EQ APO XT AVX-512` shortcut, not the Program Files Configuration Editor.
- [ ] Manually validate Theme Lab save/apply/import/delete and restart persistence.
- [x] Audited 20 themes × two modes: 4.5:1 text/muted/selection, distinct light/dark grounds, modern + LegacyRows tooltips; EditorLogic 4043 and `--theme-lab-test` passed.
- [x] Rebuilt Theme Lab with picker/reset/repair, neutral control preview, audit, and safe custom-theme fallback.
- [x] Installed hash-verified `9f12df1c` Editor-only Theme Lab overlay in the registered AVX-512 test runtime; VST DLL + Program Files untouched.
- [x] Reproduced the post-overlay FabFilter failure from `Editor.log`; later PID attribution shows the observed rejection came from the stale Editor-side loader, not the active v2.42.2 engine.
- [x] Rebuilt the affected `Common` VST-host code and `EqualizerAPO.dll` at AVX-512; candidate SHA-256 `3A11AB586EE7D5A2C3A6A3D87FC3744F1E501AEC82F48369FC9FA5B04A8949B7`. AVX-512 `HybridConvTests.exe` passes 1,635 checks, including VST3 factory-host-context coverage (103 checks).
- [x] Backed up the preflight v2.42.2 engine (`52E96A...`) at `artifacts/install-backups/fabfilter-engine-avx512-20260824-144041`, swapped only the registered runtime's `EqualizerAPO.dll`, and briefly restarted Windows Audio. The new `audiodg` process confirms the AVX-512 candidate SHA-256 is loaded.
- [x] Identified the remaining FabFilter error as the stale Editor-side VST loader: failure PID 49932 was `Editor.exe`, while active `audiodg` was PID 39572. Rebuilt and hash-verified AVX-512 `Editor.exe` (`C54ECB...FF8A8`) is installed with a backup at `artifacts/install-backups/fabfilter-editor-avx512-20260824-151810`; its `--diagnose` check exits 0. The exact Pro-Q 4 factory probe passes. Agent-session Qt graphical self-tests crash for both old and new builds and are excluded from evidence.
- [x] Corrected the log interpretation: its leading decimal field is a thread ID. Exact library load/unload passes 12 cycles, and a temporary Editor FilterEngine probe loads the real config and Pro-Q 4 successfully. Installed the temporary tagged diagnostic Editor (`BCB4A1...EE26B`) with backup `artifacts/install-backups/fabfilter-editor-diagnostic-avx512-20260824-165026`; manual UI retry must classify or clear the live error.
- [ ] Manually retry the same FabFilter Pro-Q 4 row; it must load without the library error and retain normal audio processing.
- [ ] Manually validate the new Theme Lab readability workflow and tooltip treatment in both a LegacyRows palette and a modern theme.
- [ ] Manually validate LegacyRows palette selection and restart persistence in an integration candidate.
- [ ] Manually validate a VST3 bundle import and plug-in load in an integration candidate.
- [x] GitHub Actions run `32795860636` passed Pester 5, cppcheck, six native variants, UI gates, packaging, and cross-variant comparison; portable prerelease `v2.42.2-beta.1` is published from tested beta commit `7b2b349c`.
- [x] Integrated exact upstream v2.42.2 locally, retaining fork VST preview/safety and theme changes alongside upstream VST3 slot-fill controls; corrected the resulting SkinGallery VST constructor call.
- [x] v2.42.2 validation: Release Editor build, EditorLogicTests (4306), HybridConvTests (1635), and EngineOrchestrationTests (1259) pass.
- [x] User approved and completed a v2.42.2 runtime test install, validated the green beta build, and authorized promotion to main.
- [x] Built the v2.42.2 AVX-512 Editor (`38E60F...622CB`) and engine (`F5A474...974CE`); a fresh AVX-512 HybridConvTests run passes 1635 checks.
- [x] User-approved test install overlaid only the active AVX-512 runtime's Editor and engine; backup `artifacts\\install-backups\\v2422-avx512-20260824-202257`; hashes verified and Windows Audio restored.
- [x] User reported the installed v2.42.2 candidate works; the reconciled release is published to `origin/beta` at `76717818`.
- [x] Reconciled beta-only VST3 bus, legacy-parser, and capture channel-mask safeguards; AVX-512 HybridConvTests (1,635) and EngineOrchestrationTests (1,259) pass.
- [x] Repaired the release gate: a newer nearest prerelease base now promotes `version.h` even when only docs commits follow it. An origin-only replay changes `2.42.1` to `2.42.2`; subsequent Pester 5/full-release CI passed.
- [x] Main run `32799586405` passed Pester 5, cppcheck, memcheck, all six native variants, cross-variant comparison, packaging, and release completeness; stable `v2.42.2` is published with 27 assets.
- [x] Corrected `v2.42.2` tag and stored release target to shipped commit `f0700ffe`; `d6123c32` hardens both release metadata call sites and run `32801779998` is green.
