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

	// test other behavior
    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_VALID);
    die.Roll(); // triggers a Recompute
    EXPECT_EQ(die.GetScore(false), 30);
    EXPECT_EQ(die.GetScore(true), 15);
}

TEST(SkillTests, MaximumSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("M9:8","M7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	// test other behavior
    // Arrange: Given a 6 sided Maximum die
    BMC_Die die = TEST_Util::createTestDie(6, BME_PROPERTY_MAXIMUM);

    EXPECT_EQ(die.GetValueTotal(), 0);

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

	// test other behavior
    // Arrange: Given a 6 sided Maximum die
    BMC_Die die = TEST_Util::createTestDie(6, BME_PROPERTY_INSULT);
    die.Roll(); // triggers a Recompute

    BMC_Move power = BMC_Move();
    power.m_attack = BME_ATTACK_POWER;
    EXPECT_TRUE(die.CanBeAttacked(power));

    BMC_Move skill = BMC_Move();
    skill.m_attack = BME_ATTACK_SKILL;
    EXPECT_FALSE(die.CanBeAttacked(skill));

}

TEST(SkillTests, NullSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("n9:8","n7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	// test other behavior
    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_NULL);
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
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("v9:8","v7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	// test other behavior
    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_VALUE);
    die.Roll(); // triggers a Recompute
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

	// test other behavior
    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_NULL|BME_PROPERTY_VALUE);
    die.Roll(); // triggers a Recompute
    EXPECT_EQ(die.GetScore(false), 0);
    EXPECT_EQ(die.GetScore(true), 0);
}

TEST(SkillTests, MorphingSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("m9:8","m7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	// // test other behavior
	// TEST_ParserZ parser = TestUtils::takeOneStep("m30:15","4:4");
	// EXPECT_EQ(parser.GetGame().GetPlayer(0)->GetDie(0)->GetSides(0), 4);
}

TEST(SkillTests, PoisonSkill) {
	TEST_Util test;

	BMC_Move _move;
	EXPECT_NO_THROW({
		_move = test.ParseFightGetAttack("p9:8","p7:6");
	});
	EXPECT_THAT(_move, IsAttack(BME_ATTACK_TYPE_1_1, "power", 0, 0));

	// test other behavior
    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_POISON);
    die.Roll(); // triggers a Recompute
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

	// test other behavior
    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_POISON|BME_PROPERTY_VALUE);
    die.Roll(); // triggers a Recompute
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

	// test other behavior
    BMC_Die die = TEST_Util::createTestDie(30, BME_PROPERTY_POISON|BME_PROPERTY_NULL);
    die.Roll(); // triggers a Recompute
    EXPECT_EQ(die.GetScore(false), 0);
    EXPECT_EQ(die.GetScore(true), 0);
}
