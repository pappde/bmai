#include "../src/BMC_Parser.h"
#include <gtest/gtest.h>

TEST(ParserTests, ParseString) {

    // Arrange Act Assert
    // is a parallel to the pattern
    // Given When Then

    // Arrange
    // Given a parser
    BMC_Parser parser;
    // Given a nice big multiline string
    const char * input = R"IN(
game
fight
player 0 3 0
30:16
30:16
6/30-6:5
player 1 3 0
5/7-5:4
7/9-7:3
9/11-9:3
getaction
)IN";

    // Act
    // When parser is asked to parse the string
    // Assert
    // Then no exception should be thrown
    EXPECT_NO_THROW({
        parser.ParseString(input);
    });

}
