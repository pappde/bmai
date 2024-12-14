// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp <denis@accessdenied.net>

#include <gtest/gtest.h>
#include "./testutils.h"
#include "../src/BMC_Game.h"
#include "../src/BMC_Parser.h"


TEST(SkillTests, NoSkill) {
    BMC_Die die = TestUtils::createTestDie(30, BME_PROPERTY_VALID);
    die.Roll(); // triggers a Recompute
    EXPECT_EQ(die.GetScore(false), 30);
    EXPECT_EQ(die.GetScore(true), 15);
}

TEST(SkillTests, MaximumSkill) {
    // Arrange: Given a 6 sided Maximum die
    BMC_Die die = TestUtils::createTestDie(6, BME_PROPERTY_MAXIMUM);

    EXPECT_EQ(die.GetValueTotal(), 0);

    for (int i = 0; i < 10; ++i) {
        // Act: When the die is rolled 10 times
        die.Roll();

        // Assert: Then it always has the max value
        EXPECT_EQ(die.GetValueTotal(), 6);
    }
}

TEST(SkillTests, InsultSkill) {
    // Arrange: Given a 6 sided Maximum die
    BMC_Die die = TestUtils::createTestDie(6, BME_PROPERTY_INSULT);
    die.Roll(); // triggers a Recompute

    BMC_Move power = BMC_Move();
    power.m_attack = BME_ATTACK_POWER;
    EXPECT_TRUE(die.CanBeAttacked(power));

    BMC_Move skill = BMC_Move();
    skill.m_attack = BME_ATTACK_SKILL;
    EXPECT_FALSE(die.CanBeAttacked(skill));

}

TEST(SkillTests, NullSkill) {
    BMC_Die die = TestUtils::createTestDie(30, BME_PROPERTY_NULL);
    die.Roll(); // triggers a Recompute

    EXPECT_EQ(die.GetScore(false), 0);
    EXPECT_EQ(die.GetScore(true), 0);

    // TEST_Parser parser = TestUtils::createTestParser("fight", {"n9:9"},{"2:2"});
    //
    // TEST_Game game = parser.GetGame();
    // BMC_Player* p0 = game.GetPlayer(0);
    // BMC_Player* p1 = game.GetPlayer(1);
    //
    // EXPECT_EQ(p0->GetScore(), 0);
    // EXPECT_EQ(p1->GetScore(), 1);

}

TEST(SkillTests, ValueSkill) {
    BMC_Die die = TestUtils::createTestDie(30, BME_PROPERTY_VALUE);
    die.Roll(); // triggers a Recompute
    EXPECT_EQ(die.GetScore(false), die.GetValueTotal());
    EXPECT_EQ(die.GetScore(true), die.GetValueTotal()/2.0f);
}

TEST(SkillTests, NullValueSkill) {
    BMC_Die die = TestUtils::createTestDie(30, BME_PROPERTY_NULL|BME_PROPERTY_VALUE);
    die.Roll(); // triggers a Recompute
    EXPECT_EQ(die.GetScore(false), 0);
    EXPECT_EQ(die.GetScore(true), 0);
}

TEST(SkillTests, PoisonSkill) {
    BMC_Die die = TestUtils::createTestDie(30, BME_PROPERTY_POISON);
    die.Roll(); // triggers a Recompute
    EXPECT_EQ(die.GetScore(false), -15);
    EXPECT_EQ(die.GetScore(true), -30);
}

TEST(SkillTests, PoisonValueSkill) {
    BMC_Die die = TestUtils::createTestDie(30, BME_PROPERTY_POISON|BME_PROPERTY_VALUE);
    die.Roll(); // triggers a Recompute
    EXPECT_EQ(die.GetScore(false), die.GetValueTotal()*-1/2.0f);
    EXPECT_EQ(die.GetScore(true), die.GetValueTotal()*-1);
}

TEST(SkillTests, PoisonNullSkill) {
    BMC_Die die = TestUtils::createTestDie(30, BME_PROPERTY_POISON|BME_PROPERTY_NULL);
    die.Roll(); // triggers a Recompute
    EXPECT_EQ(die.GetScore(false), 0);
    EXPECT_EQ(die.GetScore(true), 0);
}
