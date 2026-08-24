# TODO

- [ ] Manually verify analyzer movement with real VST2 and VST3 plug-ins before any integration-candidate install.
- [ ] Manually test Theme Lab save/apply/import/delete and persistence across an Editor restart.
- [ ] Manually test LegacyRows palette selection and restart persistence in an integration candidate.
- [x] Audited remaining fork-only behavior by feature group; VST3 layouts, card focus, and VST readability were already upstream, and VST3 bundle import is now ported.
- [ ] Manually import a representative VST3 bundle in an integration candidate and verify it loads after restart.
- [x] Corrected the skin-module CI gate so token variants reuse their `paintBaseId` module; local check passes for 5 modules across 20 themes.
- [x] Corrected the full skin-gallery overflow check to use the active painter base; a rebuilt 20-theme 5,840-shot gallery passes locally.
- [ ] Complete the manually dispatched CI test build, including Pester 5 build-script tests, then manually test its packaged integration candidate.
- [ ] Manually open the VST2 no-editor test plug-in's panel in an integration candidate; it must reject safely without crashing.
- [ ] In the installed AVX-512 VST3 factory-context test overlay, add/load/open a previously failing FabFilter or iZotope VST3. It must not report "library could not be loaded". The obsolete `C:\\Program Files\\VSTPlugins\\win-rnnoise\\rnnoise.vst3` remains access-denied and is outside this fix.
- [ ] Launch the repaired `EQ APO XT AVX-512` shortcut (not the Program Files Configuration Editor), then verify FabFilter Pro-Q 4 loads and LegacyRows exposes a working red Remove icon for a disposable row.
- [ ] Manually validate the rebuilt Theme Lab in both a LegacyRows palette and a modern theme: tooltip readability, light/dark visual separation, swatch picker, reset, text-contrast repair, preview-in-app, save/apply/import/export.
- [ ] Do not merge, install, or promote this port without user approval.
