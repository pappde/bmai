// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp <denis@accessdenied.net>
// SPDX-FileComment: https://github.com/pappde/bmai

#include "../src/BMC_Player.h"
#include <gtest/gtest.h>

TEST(PlayerTests, CopyConstructor) {

    // Arrange Act Assert
    // is a parallel to the pattern
    // Given When Then

    // Arrange
    // Given a player with id of 1
    BMC_Player player1 = BMC_Player();
    player1.SetID(1);
    EXPECT_EQ(player1.GetID(), 1);

    // Act
    // When another player is created using the copy constructor
    BMC_Player player2 = BMC_Player(player1);

    // Assert
    // Then the new player should have the same ID
    EXPECT_EQ(player2.GetID(), 1);

    // Act
    // When new player changes its ID
    player2.SetID(2);
    EXPECT_EQ(player2.GetID(), 2);

    // Assert
    // Then the first player should not have its id changed
    EXPECT_EQ(player1.GetID(), 1);

}
