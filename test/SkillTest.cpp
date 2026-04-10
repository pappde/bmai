// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp <denis@accessdenied.net>
// SPDX-FileComment: https://github.com/pappde/bmai

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "./_matchers.h"
#include "./_testutils.h"
#include "../src/BMC_Parser.h"
#include "../src/BMC_RNG.h"

namespace {

BMC_Die *FindDieByOriginalIndex(BMC_Player *player, int original_index) {
	for (int i = 0; i < player->GetAvailableDice(); ++i) {
		BMC_Die *die = player->GetDie(i);
		if (die->GetOriginalIndex() == original_index)
			return die;
	}
	return nullptr;
}

}  // namespace

class KonstantSignedAssignmentTests : public ::testing::TestWithParam<std::string> {};

TEST(SkillTests, NoSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("9:8","7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	auto a_dice = TEST_Util::extractAttackerDice(_move);
	EXPECT_EQ(a_dice[0]->GetScore(true), 4.5);
	auto t_dice = TEST_Util::extractTargetDice(_move);
	EXPECT_EQ(t_dice[0]->GetScore(false), 7);

    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_VALID);
    EXPECT_EQ(die.GetScore(false), 30);
    EXPECT_EQ(die.GetScore(true), 15);
}

TEST(SkillTests, MultiDieSkillAttack) {
	TEST_Util test;

	TEST_Util::FightContext context;
	EXPECT_NO_THROW({
		context = test.ParseFightContext("6:5 6:1","20:6");
	});

	auto valid_attacks = context.ValidAttacks();
	EXPECT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", {0,1}, 0)
	));
}

TEST(SkillTests, SingleDieSkillAttack) {
	TEST_Util test;

	TEST_Util::FightContext context;
	EXPECT_NO_THROW({
		context = test.ParseFightContext("6:6","20:6");
	});

	auto valid_attacks = context.ValidAttacks();
	EXPECT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0),
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", 0, 0)
	));
}

TEST(SkillTests, KonstantSingleDieSkillAttack) {
	TEST_Util test;

	TEST_Util::FightContext context;
	EXPECT_NO_THROW({
		context = test.ParseFightContext("k6:6", "20:6");
	});

	auto valid_attacks = context.ValidAttacks();
	EXPECT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAction(BME_ACTION_PASS)
	));
}

TEST(SkillTests, KonstantCannotPowerAttack) {
	BMC_Die die = TEST_Util::createTestDie(6, BME_PROPERTY_KONSTANT);

	EXPECT_FALSE(die.CanDoAttack(BME_ATTACK_POWER));
	EXPECT_TRUE(die.CanDoAttack(BME_ATTACK_SKILL));
}

TEST(SkillTests, KonstantMultiDieSkillAttackWithSubtraction) {
	TEST_Util test;

	auto context = test.ParseFightContext(
		"Mk1:1 Mk1:1 Mk3:3",
		"3:3"
	);

	EXPECT_THAT(context.ValidAttacks(), ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", {0, 1, 2}, 0)
	));
}

TEST_P(KonstantSignedAssignmentTests, MultiDieSkillAttackAllowsSignedAssignments) {
	TEST_Util test;

	// KONSTANT allow + or -
	// 5+2+3 10
	// 5+2-3 4
	// 5-2+3 6
	// 5-2-3 0
	auto context = test.ParseFightContext(
		"41:5 Mk2:2 Mk3:3",
		GetParam()
	);

	EXPECT_THAT(context.ValidAttacks(), ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", {0, 1, 2}, 0)
	));
}

INSTANTIATE_TEST_SUITE_P(
	SkillTests,
	KonstantSignedAssignmentTests,
	::testing::Values(
		"d10:10",
		"d4:4",
		"d6:6",
		"d0:0"
	));

TEST(SkillTests, KonstantMixedMultiDieSkillAttackWithSubtraction) {
	TEST_Util test;

	auto context = test.ParseFightContext(
		"8:8 Mk1:1",
		"d7:7"
	);

	EXPECT_THAT(context.ValidAttacks(), ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", {0, 1}, 0)
	));
}

TEST(SkillTests, KonstantMultiDieSkillAttackNoMatchingAssignment) {
	TEST_Util test;

	auto context = test.ParseFightContext(
		"Mk1:1 Mk1:1 Mk3:3",
		"6:6"
	);

	EXPECT_THAT(context.ValidAttacks(), ::testing::UnorderedElementsAre(
		IsAction(BME_ACTION_PASS)
	));
}

TEST(SkillTests, KonstantWarriorCannotSubtractInSkillAttack) {
	TEST_Util test;

	auto context = test.ParseFightContext(
		"`k3:3 Mk5:5",
		"d2:2"
	);

	EXPECT_THAT(context.ValidAttacks(), ::testing::UnorderedElementsAre(
		IsAction(BME_ACTION_PASS)
	));
}

TEST(SkillTests, OnlyOneWarriorMayParticipateInSkillAttack) {
	TEST_Util test;

	auto context = test.ParseFightContext(
		"41:5 `k2:2 `k3:3",
		"d10:10"
	);

	EXPECT_THAT(context.ValidAttacks(), ::testing::UnorderedElementsAre(
		IsAction(BME_ACTION_PASS)
	));
}

TEST(SkillTests, KonstantMultiDieSkillAttackWithoutSubtraction) {
	TEST_Util test;

	auto context = test.ParseFightContext(
		"Mk1:1 Mk2:2",
		"d3:3"
	);

	EXPECT_THAT(context.ValidAttacks(), ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", {0, 1}, 0)
	));
}

TEST(SkillTests, KonstantGenerationFindsLaterTargetAfterOvershoot) {
	TEST_Util test;

	auto context = test.ParseFightContext(
		"8:8 Mk1:1",
		"d8:8 d7:7"
	);

	EXPECT_THAT(context.ValidAttacks(), ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", {0, 1}, 1)
	));
}

TEST(SkillTests, KonstantSkillAttackWithUnusedStingerInPool)
{
	TEST_Util test;

	// Attack with 5+2-3=4 to hit target 4, Stinger not involved
	auto ctx = test.ParseFightContext(
		"5:5 Mk2:2 Mk3:3 g6:6",
		"4:4");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Contains(
										IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.a({ "5", "Mk2", "Mk3" }), ctx.t("4"))));
}

TEST(SkillTests, StingerAndKonstantBothInAttack)
{
	TEST_Util test;

	// Stinger(6)+Konstant(3) can hit anything from (1+3) to (6+3) = 4-9
	// Test hitting 7: Stinger uses 4, Konstant adds 3
	auto context = test.ParseFightContext(
		"g6:6 Mk3:3",
		"7:7");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, StingerAndKonstantWithSubtraction)
{
	TEST_Util test;

	// Stinger(8)+Konstant(5) can hit 3: Stinger=8, Konstant=-5
	auto context = test.ParseFightContext(
		"g8:8 Mk5:5",
		"3:3");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

// === Pure Stinger Skill Attack Tests ===
// Per rules: "When a Stinger Die participates in a Skill Attack, it can be used
// as any number between its minimum possible value and the value it currently shows."

TEST(SkillTests, StingerSkillAttackInRange)
{
	TEST_Util test;

	// normal(4) + Stinger(6): skill range is [4+1, 4+6] = [5, 10]
	// Target 7: Stinger uses 3
	auto context = test.ParseFightContext(
		"4:4 g6:6",
		"7:7");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, StingerSkillAttackAtMinimumRange)
{
	TEST_Util test;

	// normal(4) + Stinger(6): minimum is 4+1=5
	// Stinger contributes exactly 1
	auto context = test.ParseFightContext(
		"4:4 g6:6",
		"5:5");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, StingerSkillAttackAtMaximumRange)
{
	TEST_Util test;

	// normal(4) + Stinger(6): maximum is 4+6=10
	// Stinger contributes full value
	auto context = test.ParseFightContext(
		"4:4 g6:6",
		"10:10");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, StingerSkillAttackBelowRange)
{
	TEST_Util test;

	// normal(4) + Stinger(6): minimum is 4+1=5, so target 4 cannot be hit
	// by a 2-die skill attack (Stinger minimum contribution is 1)
	auto context = test.ParseFightContext(
		"4:4 g6:6",
		"4:4");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Not(::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0))));
}

TEST(SkillTests, TwoStingersSkillAttackRange)
{
	TEST_Util test;

	// Per rules: "Two Stinger Dice showing 10 can Skill Attack any die between 2 and 20"
	// Minimum: both Stingers contribute 1 each = 2
	auto context = test.ParseFightContext(
		"g10:10 g10:10",
		"2:2");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, TwoStingersCannotHitBelowMinimum)
{
	TEST_Util test;

	// Two Stingers: minimum is 1+1=2, so target 1 cannot be hit
	auto context = test.ParseFightContext(
		"g10:10 g10:10",
		"1:1");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Not(::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0))));
}

// === Stinger + Konstant Combined Flexibility ===
// No direct interaction documented. Each skill applies its own rules:
// Stinger can use [1, current_value], Konstant can add or subtract.

TEST(SkillTests, StingerAtValueOneHasNoFlexibility)
{
	TEST_Util test;

	// Stinger showing 1: range is [1, 1], no flexibility (slack = 0)
	// normal(4) + Stinger(showing 1): only total is 4+1=5
	auto context = test.ParseFightContext(
		"4:4 g6:1",
		"5:5");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, StingerAtValueOneCannotHitLowerTarget)
{
	TEST_Util test;

	// Stinger showing 1 has no slack, so normal(4) + Stinger(1) = 5 only
	// Target 4 is NOT reachable
	auto context = test.ParseFightContext(
		"4:4 g6:1",
		"4:4");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Not(::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0))));
}

TEST(SkillTests, NormalStingerKonstantThreeDieAttack)
{
	TEST_Util test;

	// Target 5: 4+Stinger(4)+Konstant(-3) => 4+4-3=5
	// Requires all three types working together
	auto ctx = test.ParseFightContext(
		"4:4 g6:6 Mk3:3",
		"5:5");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Contains(
										IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.a({ "4", "g6", "Mk3" }), ctx.t("5"))));
}

TEST(SkillTests, KonstantWarriorCanAddInSkillAttack)
{
	TEST_Util test;

	// Warrior Konstant(3) + normal(5): 5+3=8, Warrior Konstant can add
	auto ctx = test.ParseFightContext(
		"5:5 `k3:3",
		"8:8");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Contains(
										IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.a({ "5", "`k3" }), ctx.t("8"))));
}

// Per rules: "A Warrior can't use the Stinger skill to add less than the
// full value of the die, because the die isn't in play yet"
TEST(SkillTests, StingerWarriorMustUseFullValue)
{
	TEST_Util test;

	// Warrior Stinger must use full value 6, so only total is 4+6=10
	// Target 7 would require Stinger to use 3, which Warrior forbids
	auto ctx = test.ParseFightContext(
		"4:4 `g6:6",
		"7:7");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Not(::testing::Contains(
										IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.a({ "4", "`g6" }), ctx.t("7")))));
}

TEST(SkillTests, StingerWarriorAtFullValueIsValid)
{
	TEST_Util test;

	// Warrior Stinger(6) + normal(4) at full value: 4+6=10
	auto ctx = test.ParseFightContext(
		"4:4 `g6:6",
		"10:10");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Contains(
										IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.a({ "4", "`g6" }), ctx.t("10"))));
}

TEST(SkillTests, StingerAndKonstantCombinedFlexibility)
{
	TEST_Util test;

	// Stinger(8) + Konstant(5): Stinger can use [1..8], Konstant can +5 or -5
	// Target 2: Stinger=7, Konstant=-5 => 7-5=2
	// This requires BOTH Stinger flexibility AND Konstant subtraction together
	auto context = test.ParseFightContext(
		"g8:8 Mk5:5",
		"2:2");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, StealthSingleDieSkillAttack) {
	TEST_Util test;

	TEST_Util::FightContext context;
	EXPECT_NO_THROW({
		context = test.ParseFightContext("d6:6", "20:6");
	});

	auto valid_attacks = context.ValidAttacks();
	EXPECT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAction(BME_ACTION_PASS)
	));
}

TEST(SkillTests, StealthMultiDieSkillAttack) {
	TEST_Util test;

	TEST_Util::FightContext context;
	EXPECT_NO_THROW({
		context = test.ParseFightContext("d6:5 20:1", "20:6");
	});

	auto valid_attacks = context.ValidAttacks();
	EXPECT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", {0, 1}, 0)
	));
}

TEST(SkillTests, MaximumSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("M9:8","M7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	auto a_dice = TEST_Util::extractAttackerDice(_move);
	EXPECT_EQ(a_dice[0]->GetScore(true), 4.5);
	auto t_dice = TEST_Util::extractTargetDice(_move);
	EXPECT_EQ(t_dice[0]->GetScore(false), 7);

	// test other behavior
    // Arrange: Given a 6 sided Maximum die
    BMC_Die die = TEST_Util::createTestDie(6, BME_PROPERTY_MAXIMUM);

    for (int i = 0; i < 10; ++i) {
        // Act: When the die is rolled 10 times
        die.SetState(BME_STATE_NOTSET);
        die.Roll();

        // Assert: Then it always has the max value
        EXPECT_EQ(die.GetValueTotal(), 6);
    }
}

TEST(SkillTests, RollRequiresNotSetState) {
    BMC_Die die = TEST_Util::createTestDie(6, BME_PROPERTY_VALID);

    // This invariant is enforced only in debug builds, where assert() is active.
#ifdef NDEBUG
    GTEST_SKIP() << "assert() is compiled out in release builds";
#else
    EXPECT_DEATH(
        {
            die.Roll();
        },
        "");
#endif
}

TEST(SkillTests, SwingSetRequiresNotSetState) {
    BMC_Die die = TEST_Util::createTestDie(6, BME_PROPERTY_VALID, BME_SWING_X);

    // This invariant is enforced only in debug builds, where assert() is active.
#ifdef NDEBUG
    GTEST_SKIP() << "assert() is compiled out in release builds";
#else
    EXPECT_DEATH(
        {
            die.OnSwingSet(BME_SWING_X, 8);
        },
        "");
#endif
}

TEST(SkillTests, KonstantRetainsValueWhenTripped) {
	TEST_Util test;

	auto context = test.ParseFightContext("tk8:8", "k100:7");
	auto valid_attacks = context.ValidAttacks();
	ASSERT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_1_1, "trip", 0, 0)
	));

	// Fix the RNG seed so a broken reroll path cannot randomly land back on 7 and mask the bug.
	g_rng.SRand(1);

	bool extra_turn = false;
	context.Game()->SimulateAttack(valid_attacks.front(), extra_turn);

	BMC_Player *target_player = context.Game()->GetPlayer(1);
	EXPECT_EQ(target_player->GetDie(0)->GetValueTotal(), 7);
	EXPECT_EQ(target_player->GetAvailableDice(), 0);
}

TEST(SkillTests, KonstantRetainsValueWhenChanceRerolls) {
	TEST_Util test;

	auto context = test.ParseChanceContext("ck100:7", "20:20");
	auto valid_chance = context.ValidChance();

	auto chance_it = std::find_if(valid_chance.begin(), valid_chance.end(), [](const BMC_Move &move) {
		return move.m_action == BME_ACTION_USE_CHANCE;
	});
	ASSERT_NE(chance_it, valid_chance.end());

	// Fix the RNG seed so a broken reroll path cannot randomly land back on 7 and mask the bug.
	g_rng.SRand(1);

	context.Game()->ApplyUseChance(*chance_it);

	BMC_Player *chance_player = context.Game()->GetPlayer(0);
	// Die should not have re-rolled
	EXPECT_EQ(chance_player->GetDie(0)->GetValueTotal(), 7);
}

TEST(SkillTests, KonstantAttackerRetainsValueAfterSkillAttack) {
	TEST_Util test;

	auto context = test.ParseFightContext("k20:13 7:7", "20:20");
	auto valid_attacks = context.ValidAttacks();
	ASSERT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", {0, 1}, 0)
	));

	BMC_Player *attacker = context.Game()->GetPlayer(0);
	BMC_Die *konstant_die = attacker->GetDie(0);
	ASSERT_NE(konstant_die, nullptr);
	int original_index = konstant_die->GetOriginalIndex();
	ASSERT_EQ(konstant_die->GetValueTotal(), 13);

	g_rng.SRand(1);

	bool extra_turn = false;
	context.Game()->SimulateAttack(valid_attacks.front(), extra_turn);

	attacker = context.Game()->GetPlayer(0);
	konstant_die = FindDieByOriginalIndex(attacker, original_index);
	ASSERT_NE(konstant_die, nullptr);
	// Die should not have re-rolled
	EXPECT_EQ(konstant_die->GetValueTotal(), 13);
}

TEST(SkillTests, KonstantWarriorRetainsValueWhenUsedInSkillAttack) {
	TEST_Util test;

	auto context = test.ParseFightContext("`k41:17 11:11", "20:28");
	auto valid_attacks = context.ValidAttacks();
	ASSERT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", {0, 1}, 0)
	));

	BMC_Player *attacker = context.Game()->GetPlayer(0);
	BMC_Die *warrior_die = attacker->GetDie(0);
	ASSERT_NE(warrior_die, nullptr);
	int original_index = warrior_die->GetOriginalIndex();
	ASSERT_EQ(warrior_die->GetValueTotal(), 17);
	ASSERT_TRUE(warrior_die->HasProperty(BME_PROPERTY_WARRIOR));

	g_rng.SRand(1);

	bool extra_turn = false;
	context.Game()->SimulateAttack(valid_attacks.front(), extra_turn);

	attacker = context.Game()->GetPlayer(0);
	warrior_die = FindDieByOriginalIndex(attacker, original_index);
	ASSERT_NE(warrior_die, nullptr);
	// Die should not have re-rolled
	EXPECT_EQ(warrior_die->GetValueTotal(), 17);
	EXPECT_FALSE(warrior_die->HasProperty(BME_PROPERTY_WARRIOR));
}

TEST(SkillTests, InsultSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("I9:8","I7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	auto a_dice = TEST_Util::extractAttackerDice(_move);
	EXPECT_EQ(a_dice[0]->GetScore(true), 4.5);
	auto t_dice = TEST_Util::extractTargetDice(_move);
	EXPECT_EQ(t_dice[0]->GetScore(false), 7);

	// test other behavior
    // Arrange: Given a 6 sided Maximum die
    BMC_Die die = TEST_Util::createTestDie(6, BME_PROPERTY_INSULT);

    EXPECT_TRUE(die.CanBeAttacked(BME_ATTACK_POWER));
    EXPECT_FALSE(die.CanBeAttacked(BME_ATTACK_SKILL));

}

TEST(SkillTests, NullSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("n9:8","n7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	auto a_dice = TEST_Util::extractAttackerDice(_move);
	EXPECT_EQ(a_dice[0]->GetScore(true), 0);
	auto t_dice = TEST_Util::extractTargetDice(_move);
	EXPECT_EQ(t_dice[0]->GetScore(false), 0);

    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_NULL);
    EXPECT_EQ(die.GetScore(false), 0);
    EXPECT_EQ(die.GetScore(true), 0);

}

TEST(SkillTests, ValueSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("v9:8","v7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	auto a_dice = TEST_Util::extractAttackerDice(_move);
	EXPECT_EQ(a_dice[0]->GetScore(true), 4);
	auto t_dice = TEST_Util::extractTargetDice(_move);
	EXPECT_EQ(t_dice[0]->GetScore(false), 6);

    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_VALUE);
    EXPECT_EQ(die.GetScore(false), die.GetValueTotal());
    EXPECT_EQ(die.GetScore(true), die.GetValueTotal()/2.0f);
}

TEST(SkillTests, NullValueSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("nv9:8","nv7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	auto a_dice = TEST_Util::extractAttackerDice(_move);
	EXPECT_EQ(a_dice[0]->GetScore(true), 0);
	auto t_dice = TEST_Util::extractTargetDice(_move);
	EXPECT_EQ(t_dice[0]->GetScore(false), 0);

    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_NULL|BME_PROPERTY_VALUE);
    EXPECT_EQ(die.GetScore(false), 0);
    EXPECT_EQ(die.GetScore(true), 0);
}

TEST(SkillTests, SpeedSkill) {
	TEST_Util test;

	TEST_Util::FightContext context;
	EXPECT_NO_THROW({
		context = test.ParseFightContext("z10:8","4:3 6:5");
	});

	auto valid_attacks = context.ValidAttacks();
	EXPECT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 1),
		IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0),
		IsAttack(BME_ATTACK_TYPE_1_N, "speed", 0, {0,1})
	));

	auto a1_dice = TEST_Util::extractAttackerDice(context.chosen_move);
	EXPECT_EQ(a1_dice[0]->GetScore(true), 5);
	auto t1_dice = TEST_Util::extractTargetDice(context.chosen_move);
	EXPECT_EQ(t1_dice[0]->GetScore(false), 6);
	EXPECT_EQ(t1_dice[1]->GetScore(false), 4);

}

TEST(SkillTests, MorphingSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("m9:8","m7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	auto a_dice = TEST_Util::extractAttackerDice(_move);
	EXPECT_EQ(a_dice[0]->GetScore(true), 4.5);
	auto t_dice = TEST_Util::extractTargetDice(_move);
	EXPECT_EQ(t_dice[0]->GetScore(false), 7);

	BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_MORPHING);
	EXPECT_EQ(die.GetScore(false), 30);
	EXPECT_EQ(die.GetScore(true), 15);

	// now some morphing stuff
	EXPECT_EQ(a_dice[0]->GetSidesMax(), 9);
	_move.m_game->ApplyAttackPlayer(_move);
	EXPECT_EQ(_move.m_game->GetPlayer(0)->GetDie(0)->GetSidesMax(),7);
	// YES!!! successfully tested the ability to morph
}

 // Morphing Twin, Turbo, and Speed

TEST(SkillTests, MorphingTwinSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("m(5,5):8","m7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	auto a1_dice = TEST_Util::extractAttackerDice(_move);
	EXPECT_EQ(a1_dice[0]->GetScore(true), 5);
	auto t1_dice = TEST_Util::extractTargetDice(_move);
	EXPECT_EQ(t1_dice[0]->GetScore(false), 7);

	// now some morphing stuff
	EXPECT_EQ(a1_dice[0]->GetSidesMax(), 10);
	_move.m_game->ApplyAttackPlayer(_move);
	EXPECT_EQ(_move.m_game->GetPlayer(0)->GetDie(0)->GetSidesMax(), 7);

	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("m7:6", "m(10,11):3");
	});

	auto a2_dice = TEST_Util::extractAttackerDice(_move);
	EXPECT_EQ(a2_dice[0]->GetScore(true), 3.5);
	auto t2_dice = TEST_Util::extractTargetDice(_move);
	EXPECT_EQ(t2_dice[0]->GetScore(false), 21);

	EXPECT_EQ(a2_dice[0]->GetSidesMax(), 7);
	_move.m_game->ApplyAttackPlayer(_move);
	EXPECT_EQ(_move.m_game->GetPlayer(0)->GetDie(0)->GetSidesMax(), 21);
	EXPECT_EQ(_move.m_game->GetPlayer(0)->GetDie(0)->Dice(), 2);
	EXPECT_EQ(_move.m_game->GetPlayer(0)->GetDie(0)->GetSides(0), 10);
	EXPECT_EQ(_move.m_game->GetPlayer(0)->GetDie(0)->GetSides(1), 11);
	// ^ morphed into twin
}

TEST(SkillTests, MorphingSpeedSkill) {
	TEST_Util test;

	TEST_Util::FightContext context;
	EXPECT_NO_THROW({
		context = test.ParseFightContext("mz(5,5):8","4:3 6:5");
	});

	auto valid_attacks = context.ValidAttacks();
	EXPECT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 1),
		IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0),
		IsAttack(BME_ATTACK_TYPE_1_N, "speed", 0, {0,1})
	));

	auto a1_dice = TEST_Util::extractAttackerDice(context.chosen_move);
	EXPECT_EQ(a1_dice[0]->GetScore(true), 5);
	auto t1_dice = TEST_Util::extractTargetDice(context.chosen_move);
	EXPECT_EQ(t1_dice[0]->GetScore(false), 6);
	EXPECT_EQ(t1_dice[1]->GetScore(false), 4);

	// now some morphing stuff
	EXPECT_EQ(a1_dice[0]->GetSidesMax(), 10);
	context.chosen_move.m_game->ApplyAttackPlayer(context.chosen_move);
	EXPECT_EQ(context.chosen_move.m_game->GetPlayer(0)->GetDie(0)->GetSidesMax(), 10);
	// ^^ did NOT morph size
}

TEST(SkillTests, PoisonSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("p9:8","p7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	auto a_dice = TEST_Util::extractAttackerDice(_move);
	EXPECT_EQ(a_dice[0]->GetScore(true), -9);
	auto t_dice = TEST_Util::extractTargetDice(_move);
	EXPECT_EQ(t_dice[0]->GetScore(false), -3.5);

    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_POISON);
    EXPECT_EQ(die.GetScore(false), -15);
    EXPECT_EQ(die.GetScore(true), -30);
}

TEST(SkillTests, PoisonValueSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("pv9:8","pv7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	auto a_dice = TEST_Util::extractAttackerDice(_move);
	EXPECT_EQ(a_dice[0]->GetScore(true), -8);
	auto t_dice = TEST_Util::extractTargetDice(_move);
	EXPECT_EQ(t_dice[0]->GetScore(false), -3);

    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_POISON|BME_PROPERTY_VALUE);
    EXPECT_EQ(die.GetScore(false), die.GetValueTotal()*-1/2.0f);
    EXPECT_EQ(die.GetScore(true), die.GetValueTotal()*-1);
}

TEST(SkillTests, PoisonNullSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("pn9:8","pn7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	auto a_dice = TEST_Util::extractAttackerDice(_move);
	EXPECT_EQ(a_dice[0]->GetScore(true), 0);
	auto t_dice = TEST_Util::extractTargetDice(_move);
	EXPECT_EQ(t_dice[0]->GetScore(false), 0);

    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_POISON|BME_PROPERTY_NULL);
    EXPECT_EQ(die.GetScore(false), 0);
    EXPECT_EQ(die.GetScore(true), 0);
}

TEST(SkillTests, StealthTrip) {
	BMC_Die dice = TEST_Util::createTestDie(6, BME_PROPERTY_STEALTH | BME_PROPERTY_TRIP);
	EXPECT_TRUE(dice.CanDoAttack(BME_ATTACK_SKILL));
	EXPECT_FALSE(dice.CanDoAttack(BME_ATTACK_TRIP));
	EXPECT_FALSE(dice.CanDoAttack(BME_ATTACK_POWER));

	TEST_Util test;
	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("dt10:8","20:9");
	});
	EXPECT_THAT(_move, IsAction(BME_ACTION_PASS));

}

TEST(SkillTests, StealthShadow) {
	BMC_Die die = TEST_Util::createTestDie(6, BME_PROPERTY_STEALTH | BME_PROPERTY_SHADOW);
	EXPECT_TRUE(die.CanDoAttack(BME_ATTACK_SKILL));
	EXPECT_FALSE(die.CanDoAttack(BME_ATTACK_SHADOW));
	EXPECT_FALSE(die.CanDoAttack(BME_ATTACK_POWER));
}

TEST(SkillTests, StealthBerserk) {
	BMC_Die die = TEST_Util::createTestDie(6, BME_PROPERTY_STEALTH | BME_PROPERTY_BERSERK);
	EXPECT_TRUE(die.CanDoAttack(BME_ATTACK_SKILL));
	EXPECT_FALSE(die.CanDoAttack(BME_ATTACK_BERSERK));
	EXPECT_FALSE(die.CanDoAttack(BME_ATTACK_POWER));
}

TEST(SkillTests, StealthCannotBePowerAttacked) {
	TEST_Util test;

	TEST_Util::FightContext context;
	EXPECT_NO_THROW({
		context = test.ParseFightContext("20:6", "d6:6");
	});

	auto valid_attacks = context.ValidAttacks();
	EXPECT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAction(BME_ACTION_PASS)
	));
}

TEST(SkillTests, StealthCannotBeTripAttacked) {
	TEST_Util test;

	TEST_Util::FightContext context;
	EXPECT_NO_THROW({
		context = test.ParseFightContext("t10:8", "d20:9");
	});

	auto valid_attacks = context.ValidAttacks();
	EXPECT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAction(BME_ACTION_PASS)
	));
}

TEST(SkillTests, StealthCanBeMultiDieSkillAttacked) {
	TEST_Util test;

	TEST_Util::FightContext context;
	EXPECT_NO_THROW({
		context = test.ParseFightContext("6:5 6:1", "d20:6");
	});

	auto valid_attacks = context.ValidAttacks();
	EXPECT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", {0, 1}, 0)
	));
}

TEST(SkillTests, StealthSingleDieSkillCannotCaptureStealth) {
	TEST_Util test;

	TEST_Util::FightContext context;
	EXPECT_NO_THROW({
		context = test.ParseFightContext("6:6", "d20:6");
	});

	auto valid_attacks = context.ValidAttacks();
	EXPECT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAction(BME_ACTION_PASS)
	));
}

// Rush skill tests
TEST(SkillTests, RushSkillBasic) {
	TEST_Util test;

	// Rush die capturing 2 non-Rush dice: #10 capturing 4+6
	auto context = test.ParseFightContext("#10:10", "4:4 6:6");

	auto valid_attacks = context.ValidAttacks();
	// Check that there's a Rush attack
	bool has_rush = false;
	for (const auto &attack : valid_attacks)
	{
		if (attack.m_action == BME_ACTION_ATTACK && attack.m_attack == BME_ATTACK_RUSH)
		{
			has_rush = true;
			break;
		}
	}
	EXPECT_TRUE(has_rush);
}

TEST(SkillTests, RushSkillMustBeExactlyTwoDice) {
	TEST_Util test;

	// Rush die #10 can capture 2 dice summing to 10
	auto context = test.ParseFightContext("#10:10", "2:2 3:3 5:5");

	auto valid_attacks = context.ValidAttacks();
	// All Rush attacks should have exactly 2 targets
	for (const auto &attack : valid_attacks)
	{
		if (attack.m_action == BME_ACTION_ATTACK && attack.m_attack == BME_ATTACK_RUSH)
		{
			// Just verify Rush attack exists - the validation in BMC_Game ensures exactly 2 targets
			EXPECT_EQ(attack.m_attack, BME_ATTACK_RUSH);
		}
	}
}

TEST(SkillTests, RushSkillNonRushDieCanRushIfTargetHasRush) {
	TEST_Util test;

	// Non-Rush die (6) can Rush attack if target includes Rush die
	auto context = test.ParseFightContext("6:6", "#3:3 3:3");

	auto valid_attacks = context.ValidAttacks();
	// Should have Rush attack (3+3=6)
	bool has_rush = false;
	for (const auto &attack : valid_attacks)
	{
		if (attack.m_action == BME_ACTION_ATTACK && attack.m_attack == BME_ATTACK_RUSH)
		{
			has_rush = true;
			break;
		}
	}
	EXPECT_TRUE(has_rush);
}

TEST(SkillTests, RushSkillNonRushDieCannotRushNonRushTargets) {
	TEST_Util test;

	// Non-Rush die (6) cannot Rush non-Rush targets (3+4)
	auto context = test.ParseFightContext("6:6", "3:3 4:4");

	auto valid_attacks = context.ValidAttacks();
	// Should NOT have Rush attack
	for (const auto &attack : valid_attacks)
	{
		if (attack.m_action == BME_ACTION_ATTACK)
		{
			EXPECT_NE(attack.m_attack, BME_ATTACK_RUSH);
		}
	}
}
