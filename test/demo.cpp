#include <gtest/gtest.h>

int factorial( const int number ) {
  //  return number <= 1 ? number : Factorial( number - 1 ) * number;  // fail
return number <= 1 ? 1      : factorial( number - 1 ) * number;  // pass
}

// Demonstrate some basic assertions.
TEST(DemoTest, BasicAssertions) {
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
}

TEST(DemoTest, FacOf0Is1  ) {
    EXPECT_EQ( factorial(0), 1 );
}

TEST(DemoTest, FacOf1PlusAreComputed ) {
    EXPECT_EQ( factorial(1), 1 );
    EXPECT_EQ( factorial(2),  2 );
    EXPECT_EQ( factorial(3),  6 );
    EXPECT_EQ( factorial(10),  3628800 );
}