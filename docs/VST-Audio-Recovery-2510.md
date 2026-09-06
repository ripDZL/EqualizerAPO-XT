# VST audio recovery test candidate

- Base: beta/main `fe0b2b37`, released v2.51.0; work branch `codex/v2510-vst-audio-recovery`.
- User report: some plugins appear inactive (RNNoise); microphone disappears in recording/chat apps, not just preview.
- Root cause of the user's intermittent live cutout remains unconfirmed; no live recording was taken.

## Reproduced host defects and fixes

- Bundle and inner-module paths created separate owners for the same VST3 DLL; releasing one could invalidate the other's factory. Cache by resolved absolute, case-normalized module path.
- Ignored `setupProcessing`/startup/process rejections left stale or unwritten output. Latch explicit failures, bypass the entire device block, and stop re-entering the failed processor until reinitialization. Defer process-error logging off the audio thread.
- `setProcessing(kNotImplemented)` incorrectly deactivated valid effects. Accept the optional no-op notification in audio startup, panel sessions, and parameter flushes; still reject failed setup and audio processing.
- SDK reference: [Steinberg AudioEffect default setProcessing](https://github.com/steinbergmedia/vst3_public_sdk/blob/master/source/vst/vstaudioeffect.cpp).
- Intentional silent output remains valid; never bypass a denoiser merely because it gates audio.

## Validation

- Initial regression: 12/140 VST3 host checks failed (path aliases and rejected setup/start/process).
- Optional-notification regression: 3/164 failed before compatibility repair.
- Final VST3 host suite: 170 checks pass, including mono float/double, rejection after a successful block, silent effects, and native-panel session startup.
- Local v143 AVX-512 build; HybridConvTests 1635, EngineOrchestrationTests 1320, EditorLogicTests 4585 pass. Exact CI toolchains remain a separate gate.
- Real RNNoise VST3 with saved state: mono, 48/192 kHz, 64/480/1024-frame blocks, 10 synthetic seconds per case; all six pass without bypass.
- Real RNNoise VST2, FabFilter Pro-Q 4 raw VST3, Clear OSS: mono 48 kHz, 480-frame blocks, 10 synthetic seconds; all pass without bypass.
- Extended VstPreviewProbe: `--engine --channels 1 --block-frames 480 --state <base64>` exercises the real filter; initialization or error bypass is a failure, not a successful plugin test.
- Packaged Editor VST round-trip and channel-fill self-tests pass; direct build-folder launch lacked runtime dependencies, resolved by testing the complete payload.

## Delivery and next gates

- Local test staging: `C:\Users\Admin\Documents\EAPOVST364bit\artifacts\v2510-vst-audio-test-20260906`.
- Build a per-machine AVX-512 MSI; do not use the per-user channel Setup executable for installation.
- No installation, audio-service restart, active-config edit, stable release, or main promotion is authorized by this candidate preparation.
- Run complete beta CI before publishing the next unused `v2.51.0-beta.N` prerelease with all six channels, universal Setup, source, checksums, and notes.
- Manual acceptance: RNNoise effect responds; record speech through the normal microphone chain with panel closed/open and repeated stop/start. Confirm no voice loss in the actual recording/chat app.
- If cutout persists: obtain timestamp and capture the failed host boundary; do not claim these synthetic tests prove live microphone recovery.
