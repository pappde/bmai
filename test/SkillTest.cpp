// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp <denis@accessdenied.net>
// SPDX-FileComment: https://github.com/pappde/bmai

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "./_matchers.h"
#include "./_testutils.h"
#include "../src/BMC_Parser.h"

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
        die.Roll();

        // Assert: Then it always has the max value
        EXPECT_EQ(die.GetValueTotal(), 6);
    }
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
