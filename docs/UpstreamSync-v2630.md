# Upstream v2.63.0 integration

- 2026-11-09: upstream [v2.63.0](https://github.com/115dkk/EqualizerAPO-XT/releases/tag/v2.63.0) and its default-branch head both resolve to `dd9e3111847949de6d32ec4645f1e6e9648b92d6`. Base is fork beta `435b4aa6de1414d1ef4ab0ae624cf251b3311e06`; common ancestor is upstream v2.57.3 `b1e9ed13`. This advances the upstream side by 117 commits.
- Worktree `C:/Users/Admin/.codex/worktrees/upstream-v2630-beta/EAPOVST364bit`, branch `codex/upstream-v2630-beta`, pending merge with all 37 conflict paths resolved. Nothing committed or pushed. Remote beta 435b4aa6 and main fe0b2b37 unchanged; older worktrees/untracked galleries preserved.

## Integration decisions

- Adopt the upstream VST host base and residency, loading, panel/DPI and idle-flush changes. Carry forward canonical bundle/inner-module cache identity, rejected-processing bypass and safe cleanup. Remove the obsolete VSTFormatInstance façade header replaced by upstream.
- For existing dual-export DLL configurations, keep a usable VST3 factory so saved Pro-Q 4 state stays on the same ABI. A DLL with an unusable VST3 factory and a VST2 entry falls back to VST2 after unwinding VST3 initialization; .vst3 packages do not get that fallback. Fixtures cover both choices.
- Preserve selected-endpoint live preview, the native Bertom safety guard, Legacy/Modern panel probes, file/bundle import validation, Clarity/Graphite themes and delegated slot-fill, file-icon and power-key styling. Keep atomic byte restore/slow-reader retry and Send publication safeguards; port retained tests to upstream's self-registering harness.
- Keep fork distribution/release asset identity and existing audit workflow. Do not adopt upstream's new scheduled audit push to main. No license-copy product feature or live audio/configuration mutation.

## Local validation

- MSVC v143 x64 AVX-512; all four pinned Qt 6.12.0 x64 archives hash-verified. SDK and Pester were unpacked only inside the candidate worktree, not installed system-wide. Editor and Device Selector file versions 2.63.0.0.
- Source/variant/skin/diff checks pass. Pester 202/202. EditorLogic 5,580; EngineOrchestration 11,421; ASIO 631; VST3 host 210/VST host 152; HybridConv suite and AudioRegression 32/32 pass. Two unelevated ACL branches and optional real-BRIR test skipped.
- Nine synthetic-input probes pass at 96 kHz mono: FabFilter Pro-Q 4 (VST3), RNNoise (VST2), Clear OSS (VST3); preview 480-frame and engine 480/1024-frame cases. No real microphone or endpoint capture; no actual user preset/registration changed.
- Editor VST round-trip/fill, routing and power-toggle gates pass. Windows native panel fixture opens/closes in both Legacy and Modern; uses synthetic module with live analyzer feed disabled by the existing safety policy. Skin-switch passes 132/132 over 228 rows, worst 2806 ms, unchanged 2500/5000 ms warning/failure limits.
- Local build runner generates missing translation .qm files with pinned lrelease without overwriting tracked .qm files. Offscreen gates must point QT_QPA_PLATFORM_PLUGIN_PATH at the Qt SDK's platforms folder; the deployed runtime includes the Windows plugin only. Initial offscreen startup failures were this test setup, not the Editor build.
- Logs and reproducible preflight runners in `artifacts/v2630-preflight/`. The earlier beta run [38062138458](https://github.com/ripDZL/EqualizerAPO-XT/actions/runs/38062138458) succeeded on attempt 2; that was v2.57.3, not hosted validation of this merge.

## Local installer

- `artifacts/v2630-avx512-test/EqualizerAPO-XT-2.63.0-AVX512-Test.msi`: unsigned, v2.63.0.0, x64 per-machine. Built Benchmark, VoicemeeterClient and Win32 ASIO; tested primary binaries retained byte-for-byte. Verified all 78 payload files in the embedded MSI cabinet and full.nupkg.
- Local MSI default directory explicitly uses ProgramFiles64Folder/EqualizerAPO-XT-x64-avx512/current. Existing explicit VELOPACK_INSTALLDIR override, upgrade identity and hooks retained; package metadata verified, installation not run. See `artifacts/v2630-avx512-test/{RESULT.md,VERIFIED-MSI.json,SHA256SUMS.txt}`.

## Next gate

2026-11-09: user reports the installed build works and approves pushing beta. Installed v2.63.0.0 Editor/APO hashes match the packaged payload. Exact tested source unchanged; complete the normal beta merge push and six-channel CI. Itemized audio/app/latency/persistence coverage is not inferred. No further install, main promotion or release authorized.
