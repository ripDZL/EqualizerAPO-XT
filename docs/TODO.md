# TODO

- [x] Previous v2.57.3.0 beta push 435b4aa6: run38062138458 attempt2 verified green on 2026-11-09. AVX2's first-run theme timing spike did not block the unchanged retry; not proof for v2.63.0.
- [x] Integrate upstream v2.63.0 locally over beta, resolve conflicts, compile/test native and Qt. No commit/push/install or release authorized.
- [x] v2.63.0 local Editor gates pass: VST serialization/fill, native fixture panels, routing, power-toggle, and 132/132 skin switches at unchanged 5000 ms limit.
- [x] Complete v2.63.0 AVX-512 local package; verify 78 payload hashes, Win32 wrapper, system-wide MSI and canonical Program Files default. `artifacts/v2630-avx512-test/VERIFIED-MSI.json`.
- [x] User reports the installed v2.63.0 candidate works and authorizes beta push. Installed Editor/APO match the packaged build. Do not infer itemized plugin/app, latency, persistence or sustained coverage.
- [ ] Finish approved v2.63.0 beta push and exact-commit six-channel hosted v145 CI. Main and releases need separate approval.
- [ ] User's requested couple-of-days test: recording/chat continuity, live-preview stability, save/reopen persistence and errors. Keep working installation/configs unchanged; main/release need separate approval.
- [x] Post-install v2.57.3 diagnostic Mastering-only mic activity confirmed by user; no live changes. Pro-Q/full-chain/preview/latency and sustained stability remain unaccepted.
- [x] Pro-Q alone with original saved state reproduces live mic mute; Clear/Mastering alone passed.
- [x] Restore diagnostic copy to prior Mastering-only stage; original preset/backup/root/runtime/device unchanged.
- [x] Saved/fresh candidate-host offline probes both produce audio; correct suffix-based VST2 assumption (dual-export DLL, probed VST3). Installed audio-service fault still not reproduced offline.
- [x] User confirms actual mic recovery on restored Mastering-only diagnostic (`yes working again`); no live file changes this turn.
- [x] Fresh/default Pro-Q alone also mutes; exact previously accepted Mastering-only diagnostic restored via authorized guarded rollback. Original preset/backup/root/runtime/device/services unchanged.
- [x] Actual recovery after latest Mastering-only rollback confirmed by user (`yes mircrophone is back`); no live changes this turn.
- [x] Approved Pro-Q diagnostic build prepared: unsigned AVX-512 v2.57.3 MSI, numeric lifecycle/results/bounded level counters, no stored audio/per-buffer logs. Related tests, offline Pro-Q and rejection fixtures pass; 78 packaged payload hashes verified. Installed v2.51.0/config unchanged.
- [x] Installation approved; complete active runtime/config backed up and hashes verified. MSI not run: old-version uninstall would restart audio and remove current device APO assignments.
- [x] Explicit upgrade/audio/device-restoration approval received; v2.57.3 diagnostic MSI installed at canonical root, both assignments exact, all24 configs unchanged, both services Running. Upgrade/file repair exit0; 78 deployed payload hashes verified.
- [x] Approved real Pro-Q re-test/service-license correction: post-reboot fresh802/802 succeeds but emits zero; existing LicenseText copied to LOCAL SERVICE + Audiosrv restart restores fresh800/800 and saved801/801 nonzero,0 sampled silent-with-input. Mastering-only rollback/recovery pass;78 files/24 configs/four keys unchanged. Evidence: `artifacts/proq-diagnostic-20261010/license-retest-20261010-064316-f42d16a5/`.
- [x] Approved temporary original full-chain test/exact rollback:801/801 and1400/1400 Pro-Q successful calls,0 sampled silent-with-input/rejected/nonfinite, nonzero real IN1 output. Original preset untouched; Mastering-only restored. Evidence: `artifacts/proq-diagnostic-20261010/full-chain-retest-20261010T105742Z/`.
- [x] User approved activation for manual test; original enabled chain left active in diagnostic copy `E3242E6A...`; original preset/root unchanged. Receipt `artifacts/proq-diagnostic-20261010/full-chain-retest-20261010T105742Z/MANUAL-ACTIVATION.json`.
- [x] User reports `seemsgood` after requested full-chain microphone/live-preview test; record basic manual acceptance only. Working chain remains enabled; original/root/runtime hashes unchanged. Receipt `artifacts/proq-diagnostic-20261010/full-chain-retest-20261010T105742Z/USER-RESULT.json`.
- [x] Approved diagnostics cleanup and normal v2.57.3.0 local v143 AVX-512 build/tests/package complete; runtime semantics/pending fixes preserved. Five suites, nine synthetic plugin cases, VST/panel tests and scoped Standards/Spec reviews pass. `artifacts/v2573-clean-candidate-20261010T135658Z/`.
- [x] Approved clean-install/audio interruption complete at canonical Program Files current root; fresh106-file backup/same-product repair/source filename compatibility verified, exit0. All78 clean deployed hashes/24 configs/four keys/support files/license preserved. `artifacts/v2573-clean-candidate-20261010T135658Z/install-run-20261010-102552-31c67324/RESULT.md`.
- [x] CLEAN normal runtime: user reports microphone works and stays usable while disabling plugins individually (2026-10-10). Specific plugins/apps not named; do not infer RNNoise/ReaFIR coverage.
- [x] CLEAN normal runtime: user confirms "live preview still works" (2026-10-10, recorded14:52:40 UTC); no per-plugin/app coverage inferred.
- [ ] Confirm recording/chat on CLEAN normal runtime; use ACTIVE `IN1-plugin-isolation-20261010.txt` tab. Preserve newer edits to both that file and `New Simple 4-12-26.txt`, including current enable states; no auto Include restoration, commit/push/release.
- [ ] Separate remaining coverage: two unelevated ACL branches, hosted v145/ARM64, measured latency/sustained stability, disabled RNNoise/ReaFIR.
- [ ] Installer follow-up (not implemented): prevent major-upgrade device removal/legacy SFX-MFX resurrection and missing same-version support files. Current installation recovered via exact snapshots and same-product force-file MSI repair; retain logs/tests before any proposed source repair.

- [x] Actual IN1 pass-through acceptance: user reports mic no longer muted and shows speech activity while EAPO remains attached.
- [x] Approved individual saved-state isolation complete: Clear/Mastering work; Pro-Q alone mutes; Mastering restoration recovers mic.

- [x] Explicitly approved temporary live-chain bypass: root Include commented, other configs/settings and IN1 attachment unchanged. Offline pass-through check passes.
- [x] User accepted IN1 pass-through activity with APO attached; preserve guarded rollback, no new speaking/capture tests.

- [x] User-approved complete config backup verified (23 originals); live bypass and individual-copy testing were subsequently approved. Exact backup in `docs/Microphone-Regression-2573.md`.

- [x] October 10 pass-through isolation: user-confirmed IN1 activity with current installed v2.51.0 still attached. Saved-chain processing implicates a plug-in/state/interaction rather than universally broken APO pass-through; continue current stage above, no automatic device/service/install changes.

- [x] Current v2.57.3 candidate: finish local native/Qt, workflow/source/UI/VST regression gates and Standards/Spec review; document unverified native ARM64/v145 gates. Prepare and inspect local AVX-512 test package without installing.
- [x] 2026-10-09 verify the user-installed v2.57.3 payload at `C:\Program Files\EqualizerAPO-XT-x64-avx512\current`: three installed primary binaries match the local test payload. This agent did not install it.
- [x] Resolve the isolated main-IN1 mute: missing Pro-Q4 registration under LOCAL SERVICE corrected with explicit approval; normal v2.57.3 mic/individual-disable/live-preview now accepted by user. Multi-day acceptance/CI remain separate; see `docs/Microphone-Regression-2573.md`.

- [x] Push the 2026-09-30 `2.54.31.0` CI-only repair `300b1ef3` to beta normally and dispatch all six variants.
- [x] Diagnose both run `36730189033` failures; fix alias-aware routing test and provision matching v143 ARM64 ATL. Rebuilt local routing test is green; hosted ARM64 remains unverified.
- [x] Prior v2.54.31 CI follow-up was normally pushed as `1d14cbff`; run `36740403333` is green. Its v143 ARM workaround is superseded by the current tagged v2.57.3 v145 policy. Preserve the separate dirty v2.54.32/33 work.

- [x] Integrate tagged upstream `v2.54.31` (`00f578cd`) into isolated local candidate `codex/upstream-v25431-beta` over `origin/beta` `26e2a0bb`, retaining fork VST recovery, selected-endpoint preview, themes, and distribution identity. Local native/UI/gallery validation is green.
- [x] Package and install-test the v2.54.31 candidate: the unsigned local AVX-512 MSI is hash-verified at `C:\Program Files\EqualizerAPO-XT-x64-avx512\current`; config and update-pause marker are preserved, and Windows Audio is Running.
- [ ] User manually test installed v2.54.31: real VST3 chain (including conflicting plug-ins), selected-microphone live preview, microphone continuity in recording/chat apps, and latency. Beta CI-only push/build is explicitly approved; do not promote `main` or create a release until accepted and separately approved.

- [x] Reproduce and repair the Minimal-derived Legacy Preamp striped-drum regression with a deterministic 100x66 knob specimen. The legacy host now requests the token-aware precision dial and rotary drag; 112 dark/light specimen renders and Theme Lab pass.
- [x] Install the v2.54.31 UI follow-up as an Editor-only overlay at the exact AVX-512 Program Files target, with a rollback copy and before/after hashes proving config, engine, VST3 routing, and Qt platform unchanged.
- [x] User manually verified the installed Preamp overlay: the Minimal-derived Legacy dial is clear, circular, and usable. The tested merge `a44d03f5` is pushed to `beta`.
- [ ] Retain the separate real VST3-chain, microphone-continuity, and latency acceptance before any `main` promotion or release.

- [x] Reproduce and repair VST3 path-alias lifetime, explicit processing rejection, and optional startup-notification bugs; 170 host checks and real RNNoise/FabFilter/Clear OSS synthetic mono tests pass.
- [x] Finish local per-machine AVX-512 test MSI and packaged Editor/panel checks; fix `26b23931` pushed to beta, no install.
- [ ] Full beta CI `34061852125`, then complete beta prerelease from exact green `26b23931`. Main stays unchanged.
- [ ] User-test actual microphone recording/chat continuity; synthetic tests do not establish the reported cutout's root cause. See `docs/VST-Audio-Recovery-2510.md`.

- [x] Integrate tagged upstream `v2.51.0` (`3e595dc8`) and its version/changelog stamp (`4c28c2cf`) into isolated beta candidate `codex/upstream-2510-integration`, excluding unreleased `74fd96ec`, and complete local preflight.
- [x] User-manual-test the installed isolated v2.51.0 AVX-512 candidate: VST/VST3 panels and live preview, Minimal register-drum vertical drag, and theme readability.
- [x] Diagnose both beta run `33916441078` AVX2 timing outliers and retain the `5000 ms` hard limit: failures shifted from Bronze light to Forest dark and did not repeat locally or in adjacent CI rounds.
- [x] Require an immediate exact replay before an over-limit switch fails; local AVX2 compile, persistent-slow probe, and normal 132-switch gate pass.
- [x] Commit/push the timing-gate fix to `beta`, pass fresh CI `33931747211`, and publish complete prerelease `v2.51.0-beta.1` from `e32b129d`. Do not force-push or touch `main`.
- [ ] Install-test released `v2.51.0-beta.1` before any explicit `main` promotion or stable release. Keep `main` unchanged; do not force-push.

- [x] Integrate upstream `v2.50.7` (`56c10764` / `c56a0484`) as a local candidate and complete targeted native, ASIO, Qt Editor, VST, VST3-panel, static, and audio-regression validation.
- [ ] Fast-forward the validated v2.50.7 candidate to `beta`, await Actions, then install-test before any explicit `main` promotion or stable release. Keep `main` unchanged; do not force-push.

- [x] Integrate upstream `v2.50.6` / `40fb6ccf` as local candidate `b45db4d6` and complete local native, Qt, VST, ASIO, and UI-gallery validation.
- [x] Install the 69-file local AVX-512 test payload with hashes verified; preserve `config`, updater metadata, and documentation shortcuts. Backup: `artifacts\install-backups\upstream-v2506-local-avx512-20260831-100454`.
- [x] User accepted the v2.50.6 local test and authorized only a normal `beta` fast-forward.
- [x] Fast-forwarded `beta` to `43926271`; full Actions run `33404008513` is green.
- [x] Published complete prerelease `v2.50.6-beta.1` from `43926271` (33 assets; release-completeness plan green).
- [ ] Install-test the released v2.50.6 AVX-512 beta before any explicit `main` promotion or stable release. Keep `main` unchanged; do not force-push.

- [x] Integrate upstream `v2.50.2` as `d59705d4` and repair the x64 endpoint-ASIO Win32 registration gap in `320a9e84`.
- [x] Fast-forwarded beta to `2ea7bcbd`, verified green Actions run `33327886701`, installed the AVX-512 CI payload with hashes verified, and published prerelease `v2.50.2-beta.1` (33 assets; release-completeness plan green).
- [ ] User-test the installed v2.50.2 AVX-512 beta before any explicit `main` promotion or stable release. Keep `main` unchanged; do not force-push.

- [x] Integrate upstream `v2.49.0` locally and complete CI-equivalent validation.
- [x] Fast-forwarded the validated candidate to `beta`; green Actions run `33280780780` published complete prerelease `v2.49.0-beta.1` from `bd92a2de`.
- [ ] Manually install-test the released beta before any explicit `main` promotion or stable release.

- [x] Repaired the same-version beta-to-stable release gate: Pester 5 has 25 focused passes and the origin-only `v2.47.1-beta.1` dry run emits `release_required=true`.
- [x] Published stable `v2.47.1` from `eee86aa5` after green main run `33230775759`; release verification confirms all six channel sets, universal Setup, source, SHA256SUMS, and notes (33 assets). Keep `beta` fast-forward-aligned with `main`; do not force-push either release branch.

- [x] Published user-approved beta prerelease `v2.47.1-beta.1` from `4ce8efdf` after green run `33220187263`; all six channel packages, universal Setup, source, checksums, and notes pass the release completeness plan.
- [x] User-approved `beta` promotion and stable release are complete; post-release changes return to the normal beta-first workflow.

- [x] User verified the installed Clarity-menu follow-up; `origin/beta` is fast-forwarded to `f5f0ebde` with the grouped Interface entries.
- [ ] Verify beta CI for `f5f0ebde` before any release or `main` promotion. Do not create a release from this push without explicit approval.
- [ ] User-test the installed v2.47.1 AVX-512 beta at `C:\Program Files\EqualizerAPO-XT-x64-avx512\current`. Verify normal live VST preview in Modern + Legacy Rows, Bertom panel open/close, and Clarity/Graphite dark/light readability before deciding on release or `main` promotion. Full pre-install backup: `artifacts\install-backups\beta-v2471-avx512-20260828-182654`.
- [x] User approved beta promotion of the validated `upstream/main` `v2.47.1` integration: `origin/beta` fast-forwarded `0523964a..242a3f86`; all 18 conflicts are resolved. `main`, releases, installer execution, and installed files remain unchanged.
- [ ] Verify beta CI before any release or `main` promotion. Keep selected endpoint -> fork preview, no endpoint -> upstream panel monitor, Bertom native -> neither; manually recheck live preview/Bertom and Clarity/Graphite modes if CI reveals a regression.
- [ ] Resolve or restore the missing AudioRegression reference fixtures before treating the optional audio-regression suite as green; do not generate replacement goldens from this candidate. Script Pester tests remain a CI/Pester-5 gate because this host has Pester 3.4.
- [ ] With explicit approval, build the isolated Win32 Installer Release project only; do not run it, package it, or change any installed runtime.

- [ ] Complete the release-acceptance test for the installed Clarity/Graphite AVX-512 Editor candidate (`B528D825C13789A781ECE675AE7B642A6D1758765CCF4487919DE2828948165D`) at `C:\Program Files\EqualizerAPO-XT-x64-avx512\current\Editor.exe`. The user reports it seems to work; verify both themes in Modern and Legacy Rows, each in Dark and Light modes: labels, focus, disabled state, knob position/value, and VST rows (including wrapped VST3 channel fills) must remain unambiguous. The beta update is prepared locally; push its fast-forward only after explicit approval.

- [x] Fast-forwarded `beta` to the v2.42.5 mainline. Future policy: work/test through `beta`, then only user-approved promotion to `main` starts a stable release. The durable guide is `docs/BranchWorkflow.md`, linked from the README.

- [x] Published stable `v2.42.5` from `4b723927` after all six variant builds, Pester, cppcheck, memcheck, cross-variant comparison, packaging, and release completeness passed in run `33029195723`.

- [x] User reports the installed AVX-512 preview Editor (`C:\Program Files\EqualizerAPO-XT-x64-avx512\current\Editor.exe`, SHA-256 `8185116EFFEEB26B9B472550A6FCFD914172AF73ED5E7630A0D92BDB8717CF54`) fixes the missing normal separate-panel live VST preview. This result does not newly re-test the deliberate Bertom Denoiser Classic popup exception.

- [x] Installed the approved Editor-only Denoiser test build; configuration, engine, and plug-ins are unchanged.
- [x] User confirmed Bertom Denoiser Classic opens/closes without crashing in both LegacyRows and modern cards.

- [ ] User-test the prepared repaired universal Setup (`Installer\Release\EqualizerAPO-XT-Setup.exe`, SHA-256 `4B39DF838B213F639759C3767561B20DF3DB26C0D410F8BB8BC729E4E2C1CCBB`). It must get past the prior Program Files lookup error and write only to `C:\Program Files\EqualizerAPO-XT-x64-avx512`; do not remove the existing per-user v2.42.4 copy or alter legacy `C:\Program Files\EqualizerAPO`.

- [x] FabFilter Pro-Q manual test works; removed temporary debug/probe instrumentation.
- [x] User confirmed the installed clean AVX-512 Editor `6B24EFD87C122F405F37DD31312D5681A4FB8FE7F0ADE89E869B21510C095210` shows an acceptable small arrow/separator beside Browse/Options.
- [ ] Manually verify analyzer movement with real VST2 and VST3 plug-ins before any integration-candidate install.
- [ ] Manually test Theme Lab save/apply/import/delete and persistence across an Editor restart.
- [ ] Manually test LegacyRows palette selection and restart persistence in an integration candidate.
- [x] Audited remaining fork-only behavior by feature group; VST3 layouts, card focus, and VST readability were already upstream, and VST3 bundle import is now ported.
- [ ] Manually import a representative VST3 bundle in an integration candidate and verify it loads after restart.
- [x] Corrected the skin-module CI gate so token variants reuse their `paintBaseId` module; local check passes for 5 modules across 20 themes.
- [x] Corrected the full skin-gallery overflow check to use the active painter base; a rebuilt 20-theme 5,840-shot gallery passes locally.
- [x] GitHub Actions run `32795860636` passed Pester 5, cppcheck, six native variants, UI gates, packaging, and cross-variant comparison; published portable prerelease `v2.42.2-beta.1` from its beta commit.
- [ ] Optionally install-test the released matching portable package before recommending it beyond beta users.
- [ ] Manually open the VST2 no-editor test plug-in's panel in an integration candidate; it must reject safely without crashing.
- [ ] Launch the repaired `EQ APO XT AVX-512` shortcut (not the Program Files Configuration Editor), then add/load/open FabFilter Pro-Q 4. It must not report "library could not be loaded" and must retain normal audio processing. The obsolete `C:\\Program Files\\VSTPlugins\\win-rnnoise\\rnnoise.vst3` remains access-denied and is outside this fix.
- [ ] In LegacyRows, remove a disposable row through the visible red Remove icon.
- [ ] Manually validate the rebuilt Theme Lab in both a LegacyRows palette and a modern theme: tooltip readability, light/dark visual separation, swatch picker, reset, text-contrast repair, preview-in-app, save/apply/import/export.
- [x] User approved beta and main promotion after manually testing the v2.42.2 candidate and reviewing the green beta build; the stable main pipeline is authorized.
- [x] Main run `32799586405` passed Pester 5, cppcheck, memcheck, full six-variant matrix, package/release completeness, and published stable `v2.42.2`.
- [x] Corrected public `v2.42.2` tag and release target to shipped commit `f0700ffe`; the durable workflow guard is green in run `32801779998`.
- [x] Installed the v2.42.2 AVX-512 two-file test candidate after user approval; backup `artifacts\\install-backups\\v2422-avx512-20260824-202257`.
- [x] User confirmed the installed v2.42.2 candidate works; the reconciled release is published to `origin/beta` at `76717818`.
