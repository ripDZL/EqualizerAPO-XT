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

## Safety

- Do not force-push either release branch.
- Do not promote a failed or untested candidate.
- Documentation-only pushes to `main` do not create a release; the release jobs are skipped when no version bump is needed.
