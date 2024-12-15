// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2023 Denis Papp <denis@accessdenied.net>
// SPDX-FileComment: https://github.com/pappde/bmai

#include "../src/BMC_Game.h"
#include "../src/BMC_RNG.h"
#include "../src/BMC_QAI.h"
#include "../src/BMC_BMAI3.h"
#include "../src/BMC_Logger.h"
#include <cstdio>
#include <cmath>
#include <gtest/gtest.h>

// //
//
// This file contains some functions that where originally in `bmai.cpp`
// The functions were occasionally uncommented to test specific scenarios
// Ideally by wrapping up these test scenarios into a "test" we gain a few benefits:
//  - your IDE will help you kick off the test whenever you need to without changing primary source code
//  - the build process can easily run the code to make usre it functions as needed
//  - the task of unit testing will encourage more sound object design and pattern usage
//
// //

BMC_QAI tg_qai;

class LegacyMembers : public ::testing::Test {
protected:
    BMC_Game tg_game = BMC_Game(false);
    BMC_Man tg_testman1, tg_testman2;
    BMC_BMAI3 tg_ai = BMC_BMAI3(&tg_qai);
    BMC_RNG tg_rng;
};

TEST_F(LegacyMembers, SetupTestGame) {

    // Comment out next line to be able to easily run this code from IDE
    // or to include this in the overall test execution
    GTEST_SKIP() << "Skipping Setup Test Game";

    // until the objects are better suited for testing
    // it may be necessary to use a couple hacks to access members
    // http://www.gotw.ca/gotw/076.htm

    class TEST_DieData : public BMC_DieData {
    public:
        void setProperties(U32 properties) { m_properties=properties;}
        void setSides(U8 sides) { m_sides[0]=sides;}
        void setSwingType(U8 swing_type) { m_swing_type[0]=swing_type;}
    protected:
        U8 m_swing_type[BMD_MAX_TWINS];
    };

    TEST_DieData *die;

    die = reinterpret_cast<TEST_DieData *>(tg_testman1.GetDieData(0));
    die->setProperties(BME_PROPERTY_VALID);
    die->setSides(12);
    die->setSwingType(BME_SWING_NOT);

    die = reinterpret_cast<TEST_DieData *>(tg_testman1.GetDieData(1));
    die->setProperties(BME_PROPERTY_VALID);
    die->setSides(8);
    die->setSwingType(BME_SWING_NOT);

    die = reinterpret_cast<TEST_DieData *>(tg_testman1.GetDieData(2));
    die->setProperties(BME_PROPERTY_VALID);
    die->setSides(4);
    die->setSwingType(BME_SWING_NOT);

    die = reinterpret_cast<TEST_DieData *>(tg_testman1.GetDieData(3));
    die->setProperties(BME_PROPERTY_VALID);
    die->setSides(10);
    die->setSwingType(BME_SWING_NOT);

    die = reinterpret_cast<TEST_DieData *>(tg_testman2.GetDieData(0));
    die->setProperties(BME_PROPERTY_VALID);
    die->setSides(8);
    die->setSwingType(BME_SWING_NOT);

    die = reinterpret_cast<TEST_DieData *>(tg_testman2.GetDieData(1));
    die->setProperties(BME_PROPERTY_VALID);
    die->setSides(20);
    die->setSwingType(BME_SWING_NOT);

    die = reinterpret_cast<TEST_DieData *>(tg_testman2.GetDieData(2));
    die->setProperties(BME_PROPERTY_VALID);
    die->setSides(6);
    die->setSwingType(BME_SWING_NOT);

    tg_game.SetAI(0, &tg_ai);
    tg_game.SetAI(1, &tg_ai);
    tg_game.PlayGame(&tg_testman1, &tg_testman2);
}

TEST_F(LegacyMembers, TestRNG) {

    const int ranges = 10;
    const float range_size = 1.0f / ranges;
    const int sims = 1000000;
    int i, r;
    float f;
    int range[ranges] = {0,};

    // sampling
    for (i = 0; i < sims; i++) {
        f = tg_rng.GetFRand();
                BM_ASSERT(f >= 0.0f && f < 1.0f);
        r = (int) (f / range_size);
        range[r]++;
    }

    // analysis
    // var = tot2 / avg2
    double max_error = 0;
    double total = 0, total2 = 0;
    for (i = 0; i < ranges; i++) {
        double dist = (double) range[i] / (double) sims;
        double error = fabs(dist - range_size);
        total += error;
        total2 += error * error;
        max_error = std::max(max_error, error);
        printf("range %d dist %lf error %lf\n", i, dist, error);
    }
    double avg = total / ranges;
    double var = total2 / (avg * avg);

    double err = max_error / range_size;
    double stddev = sqrt(var);
    printf("max error %lf var %lf stddev %lf\n", err, var, stddev);

    EXPECT_LT(err*100, 0.3);
    EXPECT_LT(stddev, 3.8);
    // TODO consider if other expectations would be more valuable

}
