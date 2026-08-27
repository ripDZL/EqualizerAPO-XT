# TODO

- [ ] User-test the installed `Clarity High Contrast` AVX-512 Editor-only candidate (`D28EEB7F098A2C16A66E6FF5383484E6C6AF9CFD83641D9B40DC771AE7FFD287`) in Modern and Legacy Rows, each in Dark and Light modes. Verify labels, control state, knob position/value, focus, disabled state, and VST rows are unambiguous before beta push.

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
