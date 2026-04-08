# CLAUDE.md

## Meta

Claude should feel free to update this CLAUDE.md file as it learns patterns and preferences through prompts and responses in this project.

## Repo Overview

This is `github:danlangford/bmai`, a personal fork of the upstream `github:pappde/bmai`.

It is a C++ program that attempts to play the game **Button Men**.

## Skills & Rules Reference

The authoritative source for how buttons and skills work is: https://www.buttonweavers.com/ui/skills.html

A local cache is stored at `buttonweavers_skills_cache.json` (includes `dieSkills` and `dieTypes` with descriptions and interactions). The `fetched` field records when it was last updated.

**Before using the cache**, check the `fetched` date. If it is more than 7 days old, refresh it:
```bash
curl -s -X POST "https://www.buttonweavers.com/api/responder" --data '{"type":"loadDieSkillsData"}'
curl -s -X POST "https://www.buttonweavers.com/api/responder" --data '{"type":"loadDieTypesData"}'
```
Then update the `fetched` date and the `dieSkills`/`dieTypes` fields in the cache file.

## Build & Test

- Build system: **cmake**
- Test library: **googletest**
- GitHub Actions run builds and tests automatically on PRs

```
cmake --build .
ctest --output-on-failure
```

## Code Patterns

### Skill Interaction Testing
- Skills should work independently unless documented interaction exists in `docs/buttonweavers_skills_cache.json`
- ValidAttack checks attack-specific dice properties (via `_move.m_attackers.IsSet(i)`)
- GenerateValidAttacks uses player-wide checks for generation strategy, but ValidAttack is final arbiter
- When adding skill combinations: write tests showing both skills work together independently

### Property Checks
- **Player-wide**: `player->HasDieWithProperty(PROPERTY)` - checks if ANY die in player's pool has property
- **Attack-specific**: iterate `_move.m_attackers` and check each die - validates actual attack dice
- **Stack-specific**: `die_stack.CountDiceWithProperty(PROPERTY)` - checks dice in current generation stack

## Development Workflow

1. Do work on a feature branch
2. Open a PR from the feature branch into **`danlangford/bmai` main** to validate GitHub Actions (builds, tests, Copilot review) and confirm the PR looks good
3. **DO NOT MERGE** that PR — it is for validation only; close/abandon it
4. Open a new PR from the **same feature branch** into **`pappde/bmai` main** (the upstream)
5. `pappde` reviews and merges
6. Sync fork: **`danlangford/bmai` main** pulls from **`pappde/bmai` main**
7. Any other in-progress branches off the old base will likely need a **rebase** onto the updated main
