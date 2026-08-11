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

	auto ctx = test.ParseFightContext(
		"5:5 Mk2:2 Mk3:3 g6:6",
		"4:4");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Contains(
										IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.AttackerIndex({ "5", "Mk2", "Mk3" }), ctx.TargetIndex("4"))));
}

TEST(SkillTests, StingerAndKonstantBothInAttack)
{
	TEST_Util test;

	auto context = test.ParseFightContext(
		"g6:6 Mk3:3",
		"7:7");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, StingerAndKonstantWithSubtraction)
{
	TEST_Util test;

	auto context = test.ParseFightContext(
		"g8:8 Mk5:5",
		"3:3");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}


TEST(SkillTests, StingerSkillAttackInRange)
{
	TEST_Util test;

	auto context = test.ParseFightContext(
		"4:4 g6:6",
		"7:7");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, StingerSkillAttackAtMinimumRange)
{
	TEST_Util test;

	auto context = test.ParseFightContext(
		"4:4 g6:6",
		"5:5");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, StingerSkillAttackAtMaximumRange)
{
	TEST_Util test;

	auto context = test.ParseFightContext(
		"4:4 g6:6",
		"10:10");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, StingerSkillAttackBelowRange)
{
	TEST_Util test;

	auto context = test.ParseFightContext(
		"4:4 g6:6",
		"4:4");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Not(::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0))));
}

TEST(SkillTests, TwoStingersSkillAttackRange)
{
	TEST_Util test;

	auto context = test.ParseFightContext(
		"g10:10 g10:10",
		"2:2");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, TwoStingersCannotHitBelowMinimum)
{
	TEST_Util test;

	auto context = test.ParseFightContext(
		"g10:10 g10:10",
		"1:1");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Not(::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0))));
}


TEST(SkillTests, StingerAtValueOneHasNoFlexibility)
{
	TEST_Util test;

	auto context = test.ParseFightContext(
		"4:4 g6:1",
		"5:5");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, StingerAtValueOneCannotHitLowerTarget)
{
	TEST_Util test;

	auto context = test.ParseFightContext(
		"4:4 g6:1",
		"4:4");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Not(::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0))));
}

TEST(SkillTests, NormalStingerKonstantThreeDieAttack)
{
	TEST_Util test;

	auto ctx = test.ParseFightContext(
		"4:4 g6:6 Mk3:3",
		"5:5");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Contains(
										IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.AttackerIndex({ "4", "g6", "Mk3" }), ctx.TargetIndex("5"))));
}

TEST(SkillTests, KonstantWarriorCanAddInSkillAttack)
{
	TEST_Util test;

	auto ctx = test.ParseFightContext(
		"5:5 `k3:3",
		"8:8");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Contains(
										IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.AttackerIndex({ "5", "`k3" }), ctx.TargetIndex("8"))));
}

TEST(SkillTests, StingerWarriorMustUseFullValue)
{
	TEST_Util test;

	auto ctx = test.ParseFightContext(
		"4:4 `g6:6",
		"7:7");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Not(::testing::Contains(
										IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.AttackerIndex({ "4", "`g6" }), ctx.TargetIndex("7")))));
}

TEST(SkillTests, StingerWarriorAtFullValueIsValid)
{
	TEST_Util test;

	auto ctx = test.ParseFightContext(
		"4:4 `g6:6",
		"10:10");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Contains(
										IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.AttackerIndex({ "4", "`g6" }), ctx.TargetIndex("10"))));
}

TEST(SkillTests, StingerAndKonstantCombinedFlexibility)
{
	TEST_Util test;

	auto context = test.ParseFightContext(
		"g8:8 Mk5:5",
		"2:2");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, StingerKonstantOnSameDieWithSubtraction)
{
	TEST_Util test;

	auto context = test.ParseFightContext(
		"4:4 gk5:5",
		"d2:2");

	EXPECT_THAT(context.ValidAttacks(), ::testing::Contains(
											IsAttack(BME_ATTACK_TYPE_N_1, "skill", { 0, 1 }, 0)));
}

TEST(SkillTests, StingerKonstantOnSameDieWithAddition)
{
	TEST_Util test;

	auto ctx = test.ParseFightContext(
		"4:4 gk5:5",
		"d6:6");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Contains(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.AttackerIndex({ "4", "gk5" }), ctx.TargetIndex("d6"))));
}

TEST(SkillTests, StingerKonstantOnSameDieCannotHitGapBetweenSigns)
{
	TEST_Util test;

	auto ctx = test.ParseFightContext(
		"4:4 gk5:5",
		"d4:4");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Not(::testing::Contains(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.AttackerIndex({ "4", "gk5" }), ctx.TargetIndex("d4")))));
}

TEST(SkillTests, StingerWithKonstantWarriorUsesStingerFlexibility)
{
	TEST_Util test;

	auto ctx = test.ParseFightContext(
		"g6:6 `k3:3",
		"d7:7");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::ElementsAre(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.AttackerIndex({ "g6", "`k3" }), ctx.TargetIndex("d7"))));
}

TEST(SkillTests, StingerWarriorWithKonstantUsesKonstantSubtraction)
{
	TEST_Util test;

	auto ctx = test.ParseFightContext(
		"`g6:6 Mk3:3",
		"d3:3");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Contains(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.AttackerIndex({ "`g6", "Mk3" }), ctx.TargetIndex("d3"))));
}

TEST(SkillTests, StingerKonstantWarriorUsesFullPositiveValue)
{
	TEST_Util test;

	auto ctx = test.ParseFightContext(
		"4:4 `gk5:5",
		"d9:9");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Contains(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.AttackerIndex({ "4", "`gk5" }), ctx.TargetIndex("d9"))));
}

TEST(SkillTests, StingerKonstantWarriorCannotUsePartialValue)
{
	TEST_Util test;

	auto ctx = test.ParseFightContext(
		"4:4 `gk5:5",
		"d6:6");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Not(::testing::Contains(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.AttackerIndex({ "4", "`gk5" }), ctx.TargetIndex("d6")))));
}

TEST(SkillTests, StingerKonstantWarriorCannotSubtract)
{
	TEST_Util test;

	auto ctx = test.ParseFightContext(
		"4:4 `gk5:5",
		"d2:2");

	EXPECT_THAT(ctx.ValidAttacks(), ::testing::Not(::testing::Contains(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", ctx.AttackerIndex({ "4", "`gk5" }), ctx.TargetIndex("d2")))));
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

	// Death tests require active assertions.
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

	// Death tests require active assertions.
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

	// A fixed seed prevents a coincidental value match.
	g_rng.SRand(1);

	bool extra_turn = false;
	context.Game()->SimulateAttack(valid_attacks.front(), extra_turn);

	BMC_Player *target_player = context.Game()->GetPlayer(1);
	EXPECT_EQ(target_player->GetDie(0)->GetValueTotal(), 7);
	EXPECT_EQ(target_player->GetAvailableDice(), 0);
}

TEST(SkillTests, TripTargetMightyTriggersOnce) {
	TEST_Util test;

	auto context = test.ParseFightContext("t6:6", "H6:1");
	auto valid_attacks = context.ValidAttacks();
	auto trip_it = std::find_if(valid_attacks.begin(), valid_attacks.end(), [](const BMC_Move &move) {
		return move.m_attack == BME_ATTACK_TRIP;
	});
	ASSERT_NE(trip_it, valid_attacks.end());

	g_rng.SRand(1);
	bool extra_turn = false;
	context.Game()->SimulateAttack(*trip_it, extra_turn);

	EXPECT_EQ(context.Game()->GetPlayer(1)->GetDie(0)->GetSidesMax(), 8);
}

TEST(SkillTests, KonstantMightyTripTargetRetainsValueAndGrows) {
	TEST_Util test;

	auto context = test.ParseFightContext("t6:6", "Hk6:5");
	auto valid_attacks = context.ValidAttacks();
	auto trip_it = std::find_if(valid_attacks.begin(), valid_attacks.end(), [](const BMC_Move &move) {
		return move.m_attack == BME_ATTACK_TRIP;
	});
	ASSERT_NE(trip_it, valid_attacks.end());

	g_rng.SRand(1);
	bool extra_turn = false;
	context.Game()->SimulateAttack(*trip_it, extra_turn);

	BMC_Die *target = context.Game()->GetPlayer(1)->GetDie(0);
	EXPECT_EQ(target->GetValueTotal(), 5);
	EXPECT_EQ(target->GetSidesMax(), 8);
}

TEST(SkillTests, TripTargetWeakTriggersOnce) {
	TEST_Util test;

	auto context = test.ParseFightContext("t6:6", "h6:1");
	auto valid_attacks = context.ValidAttacks();
	auto trip_it = std::find_if(valid_attacks.begin(), valid_attacks.end(), [](const BMC_Move &move) {
		return move.m_attack == BME_ATTACK_TRIP;
	});
	ASSERT_NE(trip_it, valid_attacks.end());

	g_rng.SRand(1);
	bool extra_turn = false;
	context.Game()->SimulateAttack(*trip_it, extra_turn);

	EXPECT_EQ(context.Game()->GetPlayer(1)->GetDie(0)->GetSidesMax(), 4);
}

TEST(SkillTests, KonstantWeakTripTargetRetainsValueAndShrinks) {
	TEST_Util test;

	auto context = test.ParseFightContext("t6:6", "hk6:3");
	auto valid_attacks = context.ValidAttacks();
	auto trip_it = std::find_if(valid_attacks.begin(), valid_attacks.end(), [](const BMC_Move &move) {
		return move.m_attack == BME_ATTACK_TRIP;
	});
	ASSERT_NE(trip_it, valid_attacks.end());

	g_rng.SRand(1);
	bool extra_turn = false;
	context.Game()->SimulateAttack(*trip_it, extra_turn);

	BMC_Die *target = context.Game()->GetPlayer(1)->GetDie(0);
	EXPECT_EQ(target->GetValueTotal(), 3);
	EXPECT_EQ(target->GetSidesMax(), 4);
}

TEST(SkillTests, KonstantRetainsValueWhenChanceRerolls) {
	TEST_Util test;

	auto context = test.ParseChanceContext("ck100:7", "20:20");
	auto valid_chance = context.ValidChance();

	auto chance_it = std::find_if(valid_chance.begin(), valid_chance.end(), [](const BMC_Move &move) {
		return move.m_action == BME_ACTION_USE_CHANCE;
	});
	ASSERT_NE(chance_it, valid_chance.end());

	// A fixed seed prevents a coincidental value match.
	g_rng.SRand(1);

	context.Game()->ApplyUseChance(*chance_it);

	BMC_Player *chance_player = context.Game()->GetPlayer(0);
	EXPECT_EQ(chance_player->GetDie(0)->GetValueTotal(), 7);
}

TEST(SkillTests, ChanceMightyGrowsBeforeReroll) {
	TEST_Util test;

	auto context = test.ParseChanceContext("cH6:1", "20:20");
	auto valid_chance = context.ValidChance();
	auto chance_it = std::find_if(valid_chance.begin(), valid_chance.end(), [](const BMC_Move &move) {
		return move.m_action == BME_ACTION_USE_CHANCE;
	});
	ASSERT_NE(chance_it, valid_chance.end());

	g_rng.SRand(1);
	context.Game()->ApplyUseChance(*chance_it);

	EXPECT_EQ(context.Game()->GetPlayer(0)->GetDie(0)->GetSidesMax(), 8);
}

TEST(SkillTests, KonstantChanceMightyRetainsValueAndGrows) {
	TEST_Util test;

	auto context = test.ParseChanceContext("cHk6:3", "20:20");
	auto valid_chance = context.ValidChance();
	auto chance_it = std::find_if(valid_chance.begin(), valid_chance.end(), [](const BMC_Move &move) {
		return move.m_action == BME_ACTION_USE_CHANCE;
	});
	ASSERT_NE(chance_it, valid_chance.end());

	g_rng.SRand(1);
	context.Game()->ApplyUseChance(*chance_it);

	BMC_Die *chance_die = context.Game()->GetPlayer(0)->GetDie(0);
	EXPECT_EQ(chance_die->GetValueTotal(), 3);
	EXPECT_EQ(chance_die->GetSidesMax(), 8);
}

TEST(SkillTests, ChanceWeakShrinksBeforeReroll) {
	TEST_Util test;

	auto context = test.ParseChanceContext("ch6:1", "20:20");
	auto valid_chance = context.ValidChance();
	auto chance_it = std::find_if(valid_chance.begin(), valid_chance.end(), [](const BMC_Move &move) {
		return move.m_action == BME_ACTION_USE_CHANCE;
	});
	ASSERT_NE(chance_it, valid_chance.end());

	g_rng.SRand(1);
	context.Game()->ApplyUseChance(*chance_it);

	EXPECT_EQ(context.Game()->GetPlayer(0)->GetDie(0)->GetSidesMax(), 4);
}

TEST(SkillTests, KonstantChanceWeakRetainsValueAndShrinks) {
	TEST_Util test;

	auto context = test.ParseChanceContext("chk6:3", "20:20");
	auto valid_chance = context.ValidChance();
	auto chance_it = std::find_if(valid_chance.begin(), valid_chance.end(), [](const BMC_Move &move) {
		return move.m_action == BME_ACTION_USE_CHANCE;
	});
	ASSERT_NE(chance_it, valid_chance.end());

	g_rng.SRand(1);
	context.Game()->ApplyUseChance(*chance_it);

	BMC_Die *chance_die = context.Game()->GetPlayer(0)->GetDie(0);
	EXPECT_EQ(chance_die->GetValueTotal(), 3);
	EXPECT_EQ(chance_die->GetSidesMax(), 4);
}

TEST(SkillTests, KonstantOrneryMightyRetainsValueAndGrows) {
	TEST_Util test;

	auto context = test.ParseFightContext("6:6 oHk6:3", "1:1");
	auto valid_attacks = context.ValidAttacks();
	auto attack_it = std::find_if(valid_attacks.begin(), valid_attacks.end(), [&context](const BMC_Move &move) {
		return move.m_attack == BME_ATTACK_POWER && move.m_attacker == context.AttackerIndex("6");
	});
	ASSERT_NE(attack_it, valid_attacks.end());

	int original_index = context.AttackerIndex("oHk6");
	BMC_Die *ornery_die = FindDieByOriginalIndex(context.Game()->GetPlayer(0), original_index);
	ASSERT_NE(ornery_die, nullptr);
	g_rng.SRand(1);
	bool extra_turn = false;
	context.Game()->SimulateAttack(*attack_it, extra_turn);

	ornery_die = FindDieByOriginalIndex(context.Game()->GetPlayer(0), original_index);
	ASSERT_NE(ornery_die, nullptr);
	EXPECT_EQ(ornery_die->GetValueTotal(), 3);
	EXPECT_EQ(ornery_die->GetSidesMax(), 8);
}

TEST(SkillTests, KonstantOrneryWeakRetainsValueAndShrinks) {
	TEST_Util test;

	auto context = test.ParseFightContext("6:6 ohk6:3", "1:1");
	auto valid_attacks = context.ValidAttacks();
	auto attack_it = std::find_if(valid_attacks.begin(), valid_attacks.end(), [&context](const BMC_Move &move) {
		return move.m_attack == BME_ATTACK_POWER && move.m_attacker == context.AttackerIndex("6");
	});
	ASSERT_NE(attack_it, valid_attacks.end());

	int original_index = context.AttackerIndex("ohk6");
	BMC_Die *ornery_die = FindDieByOriginalIndex(context.Game()->GetPlayer(0), original_index);
	ASSERT_NE(ornery_die, nullptr);
	g_rng.SRand(1);
	bool extra_turn = false;
	context.Game()->SimulateAttack(*attack_it, extra_turn);

	ornery_die = FindDieByOriginalIndex(context.Game()->GetPlayer(0), original_index);
	ASSERT_NE(ornery_die, nullptr);
	EXPECT_EQ(ornery_die->GetValueTotal(), 3);
	EXPECT_EQ(ornery_die->GetSidesMax(), 4);
}

TEST(SkillTests, NonparticipatingOrneryDieRerolls) {
	TEST_Util test;

	auto context = test.ParseFightContext("6:6 o100:100", "1:1");
	auto valid_attacks = context.ValidAttacks();
	auto attack_it = std::find_if(valid_attacks.begin(), valid_attacks.end(), [&context](const BMC_Move &move) {
		return move.m_attack == BME_ATTACK_POWER && move.m_attacker == context.AttackerIndex("6");
	});
	ASSERT_NE(attack_it, valid_attacks.end());

	int original_index = context.AttackerIndex("o100");
	g_rng.SRand(1);
	bool extra_turn = false;
	context.Game()->SimulateAttack(*attack_it, extra_turn);

	BMC_Die *ornery_die = FindDieByOriginalIndex(context.Game()->GetPlayer(0), original_index);
	ASSERT_NE(ornery_die, nullptr);
	EXPECT_NE(ornery_die->GetValueTotal(), 100);
}

TEST(SkillTests, ParticipatingOrneryMightyTriggersOnce) {
	TEST_Util test;

	auto context = test.ParseFightContext("oH6:6", "1:1");
	auto valid_attacks = context.ValidAttacks();
	ASSERT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0)
	));

	g_rng.SRand(1);
	bool extra_turn = false;
	context.Game()->SimulateAttack(valid_attacks.front(), extra_turn);

	EXPECT_EQ(context.Game()->GetPlayer(0)->GetDie(0)->GetSidesMax(), 8);
}

TEST(SkillTests, OrneryMoodDoesNotChangeOnPass) {
	TEST_Util test;

	auto context = test.ParseFightContext("oX?-6:3", "20:20");
	auto valid_attacks = context.ValidAttacks();
	auto pass_it = std::find_if(valid_attacks.begin(), valid_attacks.end(), [](const BMC_Move &move) {
		return move.m_action == BME_ACTION_PASS;
	});
	ASSERT_NE(pass_it, valid_attacks.end());

	BMC_Die *ornery_die = context.Game()->GetPlayer(0)->GetDie(0);
	ASSERT_NE(ornery_die, nullptr);
	int sides = ornery_die->GetSidesMax();
	int value = ornery_die->GetValueTotal();

	g_rng.SRand(1);
	bool extra_turn = false;
	context.Game()->SimulateAttack(*pass_it, extra_turn);

	EXPECT_EQ(ornery_die->GetSidesMax(), sides);
	EXPECT_EQ(ornery_die->GetValueTotal(), value);
}

TEST(SkillTests, OrdinarySideChangeInvalidatesValue) {
	TEST_Util test;

	auto context = test.ParseFightContext("6:3", "20:20");
	BMC_Player *player = context.Game()->GetPlayer(0);
	BMC_Die *die = player->GetDie(0);
	ASSERT_EQ(die->GetState(), BME_STATE_READY);

	player->OnDieSidesChanging(die);

	EXPECT_EQ(die->GetState(), BME_STATE_NOTSET);
}

TEST(SkillTests, KonstantTimeAndSpaceTripDoesNotGrantExtraTurn) {
	TEST_Util test;

	auto context = test.ParseFightContext("t^k6:3", "1:1");
	auto valid_attacks = context.ValidAttacks();
	auto trip_it = std::find_if(valid_attacks.begin(), valid_attacks.end(), [](const BMC_Move &move) {
		return move.m_attack == BME_ATTACK_TRIP;
	});
	ASSERT_NE(trip_it, valid_attacks.end());

	g_rng.SRand(1);
	bool extra_turn = false;
	context.Game()->SimulateAttack(*trip_it, extra_turn);

	EXPECT_FALSE(extra_turn);
}

TEST(SkillTests, KonstantTimeAndSpaceSkillAttackDoesNotGrantExtraTurn) {
	TEST_Util test;

	auto context = test.ParseFightContext("^k6:3 2:2", "5:5");
	auto valid_attacks = context.ValidAttacks();
	ASSERT_THAT(valid_attacks, ::testing::UnorderedElementsAre(
		IsAttack(BME_ATTACK_TYPE_N_1, "skill", {0, 1}, 0)
	));

	g_rng.SRand(1);
	bool extra_turn = false;
	context.Game()->SimulateAttack(valid_attacks.front(), extra_turn);

	EXPECT_FALSE(extra_turn);
}

TEST(SkillTests, TimeAndSpaceOddRerollGrantsExtraTurn) {
	TEST_Util test;

	auto context = test.ParseFightContext("^6:1", "1:1");
	auto valid_attacks = context.ValidAttacks();
	auto attack_it = std::find_if(valid_attacks.begin(), valid_attacks.end(), [](const BMC_Move &move) {
		return move.m_attack == BME_ATTACK_POWER;
	});
	ASSERT_NE(attack_it, valid_attacks.end());

	g_rng.SRand(3);
	bool extra_turn = false;
	context.Game()->SimulateAttack(*attack_it, extra_turn);

	ASSERT_EQ(context.Game()->GetPlayer(0)->GetDie(0)->GetValueTotal()%2, 1);
	EXPECT_TRUE(extra_turn);
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
