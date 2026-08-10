<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: Copyright (c) 2026 Daniel Langford
-->

# Skill Implementation Status

This file tracks BMAI behavior against the [Button Weavers skill rules](skill-rules.md). The rules reference is generated from Button Weavers; this status is maintained by the BMAI project.

- **Implemented:** behavior is present and passing its listed regression coverage.
- **Known bug:** the listed regression test intentionally fails until the behavior is fixed.

## Konstant Interactions

| Interaction | Status | Regression coverage |
|---|---|---|
| Skill attack signed values | Implemented | `KonstantSignedAssignmentTests` |
| Stinger and Warrior skill attacks | Implemented | `StingerKonstant*`, `KonstantWarrior*` |
| Trip value retention | Implemented | `KonstantRetainsValueWhenTripped` |
| Trip with Mighty and Weak | Implemented | `KonstantMightyTripTargetRetainsValueAndGrows`, `KonstantWeakTripTargetRetainsValueAndShrinks` |
| Chance value retention | Implemented | `KonstantRetainsValueWhenChanceRerolls` |
| Chance with Mighty and Weak | Implemented | `KonstantChanceMightyRetainsValueAndGrows`, `KonstantChanceWeakRetainsValueAndShrinks` |
| Ornery value retention | Implemented | `KonstantOrneryMightyRetainsValueAndGrows`, `KonstantOrneryWeakRetainsValueAndShrinks` |
| Ornery with Mighty and Weak | Implemented | `KonstantOrneryMightyRetainsValueAndGrows`, `KonstantOrneryWeakRetainsValueAndShrinks` |
| TimeAndSpace | Implemented | `KonstantTimeAndSpaceTripDoesNotGrantExtraTurn`, `KonstantTimeAndSpaceSkillAttackDoesNotGrantExtraTurn` |
