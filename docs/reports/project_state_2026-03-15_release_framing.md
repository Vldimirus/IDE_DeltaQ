# Release Framing Review After Maturity Recovery

Audit date: `2026-03-15`

This document is the current release-framing review after closing `Stage 1-5` of
`docs/plan/strategy_2026/32_product_maturity_recovery_v1.md`.

It supersedes `docs/reports/project_state_2026-03-09_post_priorities.md` as the
current internal view for release wording.

## Decision

Current honest public framing:

- **Linux release candidate**

This label is intentionally narrow:

- it applies to the Linux-first scope only;
- it does not claim Windows/macOS delivery readiness;
- it does not erase explicit v1 limits such as `C ABI`-only imported-pack intake.

## Why This Is No Longer Just A Technical Preview

Calling the repository merely a `technical preview` would now understate the
actual verified product surface.

The repo already has:

- stable checked-in starter templates for `console` and `desktop`;
- strong checked-in example flows for console, desktop/UI, reusable composition,
  file-backed settings, process/timer, UDP probe, serial probe, and imported packs;
- release artifacts with real Linux CI, tarball packaging, AppImage packaging,
  checksum verification, first-run smoke, and `open example -> build -> run` smoke;
- user-project Linux export with runnable directory/archive handoff;
- current screenshots, onboarding docs, example catalog, and release checklists;
- a cleaned blocker-shortlist with no open top-level `crash / data-loss /
  editor-runtime mismatch` trust blockers.

That is beyond a pure engineering preview. The user-facing Linux path is already
deliberate, reproducible, and test-backed.

## Why `Early Usable Linux Release` Is Too Conservative

`Early usable` would be a safe label if the repo only had promising workflows.
At this point it has stronger proof than that:

- Stage 1 closed the desktop editor/runtime parity tail with save/reopen/build/run proof;
- Stage 2 replaced weak desktop fixtures with explicit user-facing starters;
- Stage 4 turned `core` into a useful checked-in baseline instead of a thin primitive set;
- Stage 5 turned imported-pack adaptation into a reproducible and honestly bounded flow;
- Linux release artifacts are not only built, but also verified through runtime smoke;
- public release surface already shows the real IDE/export flow rather than aspirational mockups.

Within Linux-only scope, the remaining caveats are now explicit boundaries rather
than hidden blockers.

## Why This Is Still Not A Cross-Platform Release

The release-candidate decision is intentionally scoped.

Open boundaries remain:

- Windows delivery is not a finished public path;
- macOS delivery is not a finished public path;
- imported-pack v1 only claims Linux-first `C ABI` intake;
- complex callback-heavy or arbitrary `C++` ABI wrapping is still outside the current claim.

So the right wording is not `general release candidate`, but specifically
`Linux release candidate`.

## Comparison Against Phase 2 / Phase 3 Acceptance

### Phase 2: Module Ecosystem

Release-relevant acceptance is now satisfied:

- useful `core` baseline exists across multiple meaningful scenarios;
- verification and curation are user-visible in IDE and docs;
- imported packs are documented as a real extension path, not a one-off showcase;
- reuse reads as a practical advantage, not just an implementation detail.

There are still non-blocking cleanup tails, such as further legacy pruning inside
`core`, but they no longer block the Phase 2 acceptance gate.

### Phase 3: Showcase And Adoption

Release-relevant acceptance is also satisfied:

- a new user can understand the product through README, onboarding, examples, and release docs;
- there are multiple strong demo flows rather than a single contrived showcase;
- screenshots and handoff docs match the current Linux-first product reality.

This means the repo now clears the threshold "DeltaQ can be shown as a coherent
open-source tool" for Linux scope.

## Required Public Wording

The public-facing wording should stay aligned with this review:

- `Linux release candidate` for the current platform claim;
- `Linux-first` for build, packaging, and onboarding language;
- explicit reminders that Windows/macOS are roadmap items, not current delivery claims;
- explicit reminders that imported-pack v1 is Linux-first `C ABI` intake, not universal binary wrapping.

## Summary

After closing the maturity-recovery stages, DeltaQ no longer reads honestly as
just a technical preview.

The current repo state supports a stronger but still disciplined label:

- **Linux release candidate**

That is the label Stage 6 should propagate through strategy, README, release
docs, and future public-facing messaging.
