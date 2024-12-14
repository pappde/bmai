// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp <denis@accessdenied.net>

#include "../src/BMC_Die.h"
#include "../src/BMC_Game.h"
#include "../src/BMC_Parser.h"

// until the objects are better suited for testing
// it may be necessary to use a couple hacks to access members
// http://www.gotw.ca/gotw/076.htm
class TEST_DieData : public BMC_DieData
{
public:
    void setProperties(U64 properties) { m_properties = properties; }
    void setSides(U8 sides) { m_sides[0] = sides; }
    void setSwingType(U8 swing_type) { m_swing_type[0] = swing_type; }

protected:
    U8 m_swing_type[BMD_MAX_TWINS];
};

class TEST_Game : public BMC_Game
{
public:
    TEST_Game(const BMC_Game& _game): BMC_Game(_game) {}
    void PlayPreround(){BMC_Game::PlayPreround();}
    void FinishPreround(){BMC_Game::FinishPreround();}
};

class TEST_Parser : public BMC_Parser
{
public:
    TEST_Parser() : m_game(false) {}
    TEST_Game GetGame() { return TEST_Game(m_game); }
protected:
    BMC_Game m_game;
};

class TestUtils {
public:
    static BMC_Die createTestDie(U8 sides, U64 properties) {
        return createTestDie(sides, properties, BME_SWING_NOT);
    }

    static BMC_Die createTestDie(U8 sides, U64 properties, U8 swing_type) {
        TEST_DieData die_data;
        die_data.setSides(sides);
        die_data.setProperties(BME_PROPERTY_VALID | properties);
        die_data.setSwingType(swing_type);

        BMC_Die die;
        die.SetDie(&die_data);
        die.SetState(BME_STATE_NOTSET);
        return die;
    }

    static TEST_Parser createTestParser(const std::string& phase, const std::vector<std::string>& player1Dice, const std::vector<std::string>& player2Dice) {

        TEST_Parser parser;
        std::stringstream ss;

        // Set up the game phase
        ss << "game\n" << phase << "\n";

        // Set up player 1
        ss << "player 0 " << player1Dice.size() << " 0\n";
        for (const auto& die : player1Dice) {
            ss << die << "\n";
        }

        // Set up player 2
        ss << "player 1 " << player2Dice.size() << " 0\n";
        for (const auto& die : player2Dice) {
            ss << die << "\n";
        }

        ss << "ply 1\n";
        if (phase == "preround")
        {
            ss << "playgame 1\n";
        } else if (phase=="fight")
        {
            ss << "getaction\n";
        }


        // Initialize the parser with the constructed string
        parser.ParseString(ss.str());

        return parser;

    }
};
