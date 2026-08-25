# TODO

- [ ] After approval, install the Editor-only Denoiser test build and open Bertom Denoiser Classic from both LegacyRows and a modern card. It must open/close without crashing; live analyzer feed is intentionally available only when the panel is embedded.

- [ ] Manually verify published `v2.42.3-beta.1` auto-detect Setup writes only under `C:\Program Files\EqualizerAPO-XT\<channel>`. Do not auto-remove the existing per-user XT install or alter legacy `C:\Program Files\EqualizerAPO`.

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
