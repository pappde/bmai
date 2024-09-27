#include <gtest/gtest.h>
#include "./testutils.h"



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
