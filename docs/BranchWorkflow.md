# Branch and Release Workflow

## Rule

Develop and test ordinary changes through `beta`. Do not send ordinary work directly to `main`.

## Flow

1. Start from `beta`, or from a short-lived work branch based on it.
2. Build, test, and obtain the required manual/runtime confirmation.
3. Push or merge the accepted change into `beta`.
4. After explicit user approval, fast-forward `beta` into `main`.
5. Let the version-bearing `main` push run the stable build and release workflow.
6. Once the release is green, fast-forward `beta` to the post-release `main` tip so both branches remain aligned.

## Beta prerelease

Beta build-only requests: push the accepted candidate normally, then explicitly dispatch `build.yml` on `beta` with `regenerate_references=false`; beta pushes alone do not start this workflow. Confirm its `head_sha` is the pushed commit and all six variants are included. Do not tag/publish/promote from a build-only request; retain the user's requested testing period before further approval.

After a green `beta` run and explicit release approval:

1. Use the next unused `vX.Y.Z-beta.N` tag and the exact green commit.
2. Publish all six channel Setup/MSI/feed/full/delta sets, the exact source archive, and a universal Setup built with `InstallerReleaseTag=<tag>`.
3. Immediately mark the release prerelease-only. `vpk upload` can initially expose a beta tag as a normal public release.
4. Generate `SHA256SUMS.txt` from the uploaded installer bytes and generate release notes after every asset is present.
5. Require `Publish-Release.ps1` to report complete, verify 33 assets and all 13 installer hashes, then record the result in docs and push documentation to `beta` only.

## Safety

- Do not force-push either release branch.
- Do not promote a failed or untested candidate.
- Documentation-only pushes to `main` do not create a release; the release jobs are skipped when no version bump is needed.
