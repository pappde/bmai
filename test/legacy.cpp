#include <gtest/gtest.h>
#include "../src/game.h"
#include "../src/bmai_ai.h"


class LegacyMembers : public ::testing::Test {
protected:
    BMC_Game tg_game = BMC_Game(false);
    BMC_Man tg_testman1, tg_testman2;
    BMC_BMAI3 tg_ai;
    BMC_RNG tg_rng;
};

TEST_F(LegacyMembers, SetupTestGame) {

    // Comment out next line to be able to easily run this code from IDE
    // or to include this in the overall test execution
    GTEST_SKIP() << "Skipping Setup Test Game";

    class TEST_DieData : public BMC_DieData {
        friend class LegacyMembers_SetupTestGame_Test;
    protected:
        U8 m_swing_type[BMD_MAX_TWINS];
    };

    TEST_DieData *die;

    die = reinterpret_cast<TEST_DieData *>(tg_testman1.GetDieData(0));
    die->m_properties = BME_PROPERTY_VALID;
    die->m_sides[0] = 12;
    die->m_swing_type[0] = BME_SWING_NOT;

    die = reinterpret_cast<TEST_DieData *>(tg_testman1.GetDieData(1));
    die->m_properties = BME_PROPERTY_VALID;
    die->m_sides[0] = 8;
    die->m_swing_type[0] = BME_SWING_NOT;

    die = reinterpret_cast<TEST_DieData *>(tg_testman1.GetDieData(2));
    die->m_properties = BME_PROPERTY_VALID;
    die->m_sides[0] = 4;
    die->m_swing_type[0] = BME_SWING_NOT;

    die = reinterpret_cast<TEST_DieData *>(tg_testman1.GetDieData(3));
    die->m_properties = BME_PROPERTY_VALID;
    die->m_sides[0] = 10;
    die->m_swing_type[0] = BME_SWING_NOT;

    die = reinterpret_cast<TEST_DieData *>(tg_testman2.GetDieData(0));
    die->m_properties = BME_PROPERTY_VALID;
    die->m_sides[0] = 8;
    die->m_swing_type[0] = BME_SWING_NOT;

    die = reinterpret_cast<TEST_DieData *>(tg_testman2.GetDieData(1));
    die->m_properties = BME_PROPERTY_VALID;
    die->m_sides[0] = 20;
    die->m_swing_type[0] = BME_SWING_NOT;

    die = reinterpret_cast<TEST_DieData *>(tg_testman2.GetDieData(2));
    die->m_properties = BME_PROPERTY_VALID;
    die->m_sides[0] = 6;
    die->m_swing_type[0] = BME_SWING_NOT;

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

    // TODO determine what tests could actually be done against BMC_RNG
    // based on BMC comments I have these ideas
    ASSERT_LT(err*100, 0.3);
    ASSERT_LT(stddev, 3.8);

}