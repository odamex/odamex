# OdaHeretic Milestone 1 Kickoff

## Objective
Establish a modern Heretic compatibility branch by porting applicable historical `odaraven` work onto `protobreak` using current Odamex patterns and C++20.

## Working branch
- Base branch: `feature/odaheretic` (branched from `upstream/protobreak`)

## Team workflow

### Forge (implementation)
1. Diff `upstream/odaraven` vs `upstream/protobreak`.
2. Group portable work into small topical batches.
3. Port each batch on a short-lived branch off `feature/odaheretic`.
4. Open PRs targeting `feature/odaheretic`.
5. Keep PRs small and include:
   - what was ported
   - what was adapted to modern code patterns
   - any intentionally skipped legacy behavior

### Sentry (review)
- Review for style and architecture fit with modern Odamex.
- Validate C++20 usage and existing code patterns.
- Confirm CI checks are green before merge.

## Guardrails
- If a change impacts security/build/deployment posture, pause and escalate for decision.
- Prefer iterative, shippable PRs over giant rebases.

## Immediate next actions
- [ ] Forge: produce first commit-port map from `odaraven`.
- [ ] Forge: open PR #1 (smallest viable gameplay/system slice).
- [ ] Sentry/Lotus: review and merge when checks pass.
