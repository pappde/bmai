#include "../src/die.h"


// until the objects are better suited for testing
// it may be necessary to use a couple hacks to access members
// http://www.gotw.ca/gotw/076.htm
class TEST_DieData : public BMC_DieData {
public:
    void setProperties(U32 properties) { m_properties = properties; }
    void setSides(U8 sides) { m_sides[0] = sides; }
    void setSwingType(U8 swing_type) { m_swing_type[0] = swing_type; }

protected:
    U8 m_swing_type[BMD_MAX_TWINS];
};

class TestUtils {
public:
    static BMC_Die createTestDie(U8 sides, U32 properties) {
        return createTestDie(sides, properties, BME_SWING_NOT);
    }

    static BMC_Die createTestDie(U8 sides, U32 properties, U8 swing_type) {
        TEST_DieData die_data;
        die_data.setSides(sides);
        die_data.setProperties(BME_PROPERTY_VALID | properties);
        die_data.setSwingType(swing_type);

        BMC_Die die;
        die.SetDie(&die_data);
        die.SetState(BME_STATE_NOTSET);
        return die;
    }
};
