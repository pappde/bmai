#pragma once

#include <cstdarg>
#include <cstdio>
#include <iostream>

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

// lets us collect what is being sent to Send for inspection during tests
class TEST_Parser : public BMC_Parser {
public:

	std::string tm_third_to_last_fmt;
	std::string tm_next_to_last_fmt;
	std::string tm_last_fmt;
	std::string tm_current_fmt;
	void Send(const char *_fmt, ...) override
	{
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
			BMC_Parser::Send(tm_last_fmt.c_str());
		}
	}

	BMC_Move last_attack;
	void SendAttack(BMC_Move &_move) override
	{
		last_attack = _move;
		BMC_Parser::SendAttack(_move);
	}

};

class TEST_Util
{
public:
	TEST_Parser parser;

	BMC_Move ParseFightGetAttack(std::string d0, std::string d1)
	{
		std::stringstream ss;
		ss << "game\nfight\n";
		ss << "player 0 1 0\n" << d0 << "\n";
		ss << "player 1 1 0\n" << d1 << "\n";
		ss << "ply 1\nsurrender off\ngetaction\n";
		parser.ParseString(ss.str());
		return parser.last_attack;
	}

	static std::vector<BMC_Die*> extractAttackerDice(BMC_Move move)
	{
		return extractDice(move.m_game->GetPlayer(move.m_attacker_player));
	}

	static std::vector<BMC_Die*> extractTargetDice(BMC_Move move)
	{
		return extractDice(move.m_game->GetPlayer(move.m_target_player));
	}

	static std::vector<BMC_Die*> extractDice(BMC_Player *player)
	{
		std::vector<BMC_Die*> dice;
		for (int i=0; i<player->GetAvailableDice(); i++)
		{
			dice.push_back(player->GetDie(i));
		}
		return dice;
	}


    static BMC_Die createTestDie(U8 sides, U64 properties)
	{
        return createTestDie(sides, properties, BME_SWING_NOT);
    }

    static BMC_Die createTestDie(U8 sides, U64 properties, U8 swing_type)
	{
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
