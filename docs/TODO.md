# TODO

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
