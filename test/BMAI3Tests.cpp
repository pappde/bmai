#include <gtest/gtest.h>
#include "../src/bmai_ai.h"
#include "../src/parser.h"
#include <cstdarg>  // for va_start() va_end()
#include <ghc/filesystem.hpp>

namespace fs = ghc::filesystem;

// a path util
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

// shim util
class TEST_Parser : public BMC_Parser {
public:
    void Send(char *_fmt, ...) {
        tm_next_to_last_fmt = tm_last_fmt;

        char buff[BMD_MAX_STRING];
        va_list ap;
        va_start (ap, _fmt);
        vsnprintf(buff, BMD_MAX_STRING, _fmt, ap);
        va_end (ap);

        tm_last_fmt = std::string(buff);
    }
    std::string tm_next_to_last_fmt;
    std::string tm_last_fmt;
};

class BMAISurrenderTests :public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
protected:
    TEST_Parser parser;
};

INSTANTIATE_TEST_SUITE_P(
        BMAI3Tests,
        BMAISurrenderTests,
        ::testing::Values(
            std::make_tuple("test/SurrenderDefault-Pass-in.txt", "action\nsurrender\n"),
            std::make_tuple("test/SurrenderOff-Attack-in.txt", "0\n0\n"),
            std::make_tuple("test/SurrenderOff-Pass-in.txt", "action\npass\n"),
            std::make_tuple("test/SurrenderOn-Attack-in.txt", "action\nsurrender\n"),
            std::make_tuple("test/SurrenderOn-Pass-in.txt", "action\nsurrender\n")
        ));

TEST_P(BMAISurrenderTests, CheckSurrenderAction){
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
    EXPECT_EQ(parser.tm_next_to_last_fmt+parser.tm_last_fmt, expected_last_actions);
}
