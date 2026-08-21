# TODO

- [ ] Manually verify analyzer movement with real VST2 and VST3 plug-ins before any integration-candidate install.
- [ ] Manually test Theme Lab save/apply/import/delete and persistence across an Editor restart.
- [ ] Manually test LegacyRows palette selection and restart persistence in an integration candidate.
- [x] Audited remaining fork-only behavior by feature group; VST3 layouts, card focus, and VST readability were already upstream, and VST3 bundle import is now ported.
- [ ] Manually import a representative VST3 bundle in an integration candidate and verify it loads after restart.
- [ ] Run CI's Pester 5 build-script tests and manually test a packaged integration candidate.
- [ ] Manually open the VST2 no-editor test plug-in's panel in an integration candidate; it must reject safely without crashing.
- [ ] Do not merge, push, install, or promote this port without user approval.
