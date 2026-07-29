# TODO

- [ ] Obtain a Codex environment that provides the bundled workspace dependency runtime, or explicitly approve a revised workflow.
- [ ] Prepare the Matrix hatch-pet run.
- [ ] Generate and approve the canonical base image.
- [ ] Generate and validate animation rows 0-8.
- [ ] Generate and validate cardinal anchors and look rows 9-10.
- [ ] Assemble, QA, package, and install the v2 pet.
- [ ] Decide whether to close/supersede old EqualizerAPO-XT PR #1.
- [x] Build/test synced `main` before manual overlay smoke test.
- [ ] Overlay `artifacts/EqualizerAPO-XT-x64-avx2-a19f777-dragdrop` onto local install and test real VST3 GUI.
- [x] Build a fresh drag/drop bundle from `beta` at/after `7eca352` for install testing.
- [x] If approved, delete superseded remote branches; do not delete without explicit user approval.
- [ ] Decide whether `codex/upstream-main-review-fixes-20260726` should be merged into stable `main` now or kept beta-only.
- [ ] Overlay `artifacts/github-run-30209856962/EqualizerAPO-x64-avx2` onto local install and test real VST3 GUI.
- [ ] Install a build containing the live analyzer preview feed and test ReaFIR panel animation with live audio.
- [x] Extend live analyzer preview beyond default render loopback for mic/capture-device analyzers.
- [ ] If the mic still stays static, add selected-device-aware capture instead of default console/communications capture.
- [ ] Revoke temporary GitHub token when the user ends the session.
