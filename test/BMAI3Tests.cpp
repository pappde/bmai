#include <gtest/gtest.h>
#include "../src/bmai_ai.h"
#include "../src/parser.h"
#include <cstdarg>  // for va_start() va_end()
#include <filesystem>
namespace fs = std::filesystem;

// a path util, if useful consider creaking out into generic utility
inline std::string resolvePath(const std::string &relPath)
{
    auto baseDir = fs::current_path();
    while (baseDir.has_parent_path())
    {
        auto combinePath = baseDir / relPath;
        if (fs::exists(combinePath))
        {
            return combinePath.string();
        }
        if(baseDir==baseDir.parent_path()) {
            break;
        } else {
            baseDir = baseDir.parent_path();
        }

    }
    throw std::runtime_error("File not found!");
}

// lets us collect what is being sent to Send for inspection during tests
class TEST_Parser : public BMC_Parser {
public:
    void Send(const char *_fmt, ...) override {
        char buff[BMD_MAX_STRING];
        va_list ap;
        va_start (ap, _fmt);
        vsnprintf(buff, BMD_MAX_STRING, _fmt, ap);
        va_end (ap);

        tm_current_fmt = tm_current_fmt + std::string(buff);
        if (tm_current_fmt.back()=='\n') {
            tm_third_to_last_fmt = tm_next_to_last_fmt;
            tm_next_to_last_fmt = tm_last_fmt;
            tm_last_fmt = tm_current_fmt;
            tm_current_fmt = "";
        }
    }
    std::string tm_third_to_last_fmt;
    std::string tm_next_to_last_fmt;
    std::string tm_last_fmt;
    std::string tm_current_fmt;
};

class BMAIActionTests :public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
protected:
    TEST_Parser parser;
};

// I wanted to test the smaller objects and methods. 
// For example, I could build an AI object and just execute the method GetAttackAction()
// to inspect the behavior around surrender
// however the code is not currently built in a way that is friendly to unit testing
// so the current pattern is to run full simulations and to monitor the output

INSTANTIATE_TEST_SUITE_P(
        BMAI3Tests,
        BMAIActionTests,
        ::testing::Values(
            std::make_tuple("test/SurrenderDefault-Pass-in.txt", "action\nsurrender\n"),
            std::make_tuple("test/SurrenderOff-Attack-in.txt", "power\n0\n0\n"),
            std::make_tuple("test/SurrenderOff-Pass-in.txt", "action\npass\n"),
            std::make_tuple("test/SurrenderOn-Attack-in.txt", "action\nsurrender\n"),
            std::make_tuple("test/SurrenderOn-Pass-in.txt", "action\nsurrender\n"),

            std::make_tuple("test/bug55_a_in.txt", "skill\n2 0\n0\n"),
            std::make_tuple("test/bug55_b_in.txt", "skill\n3 0 1\n2\n")

            // LONG RUNNING! use for MANUAL REGRESSION
            // TODO create better regression patterns that can be triggered from some builds
            // , std::make_tuple("test/bug11_in.txt", "action\nswing T 4\nswing W 4\n")
            // , std::make_tuple("test/bug16_in.txt", "action\nreserve 6\n")
        ));

TEST_P(BMAIActionTests, CheckSurrenderAction){
    // Arrange Act Assert
    // is a parallel to the pattern
    // Given When Then

    // Arrange
    // Given params that describe out inputs and expected outputs
    std::string test_file = std::get<0>(GetParam());
    std::string expected_last_actions= std::get<1>(GetParam());

    // Arrange
    // Given a game with one of many surrender combinations
    std::string inpath = resolvePath(test_file);
    FILE *in = fopen(inpath.c_str(),"r");

    // Act
    // When calculating the best move
    parser.Parse(in);

    // Assert
    // Then the move selected should match our expectations
    EXPECT_EQ(parser.tm_third_to_last_fmt+parser.tm_next_to_last_fmt+parser.tm_last_fmt, expected_last_actions);
}
