///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Parser.cpp
//
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp <denis@accessdenied.net>
// 
// DESC: main parsing class for setting up game state
//
// REVISION HISTORY:
// dbl100524 - broke this logic out into its own class file
///////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Parser methods
///////////////////////////////////////////////////////////////////////////////////////////

#include "BMC_Parser.h"

#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cstdlib>
#include <iostream>
#include <string>

#include "BMC_AI_Maximize.h"
#include "BMC_AI_MaximizeOrRandom.h"
#include "BMC_BMAI3.h"
#include "BMC_Logger.h"
#include "BMC_QAI.h"
#include "BMC_RNG.h"
#include "BMC_Stats.h"


// PHASE names
const char *c_phase_name[BME_PHASE_MAX] =
{
	"preround",
	"reserve",
	"initiative",
	"chance",
	"focus",
	"fight",
	"gameover"
};

// other AI instances that were once global
BMC_QAI		g_qai;
BMC_BMAI3	g_ai(&g_qai);
BMC_QAI		g_qai2;
BMC_BMAI	g_bmai(&g_qai);
BMC_BMAI3	g_bmai3(&g_qai);

BMC_AI * c_ai_type[BMD_AI_TYPES] = { &g_bmai, &g_qai2, &g_bmai3 };

INT BMC_Parser::ParseDieNumber(INT & _pos)
{
	INT value = atoi(m_line + _pos);
	while (DieIsValue(m_line[++_pos]))	// skip past number
	{
	}
	return value;
}

INT BMC_Parser::ParseDieDefinedSides(INT _pos)
{
	while (_pos < (INT)std::strlen(m_line) && m_line[_pos] != '-')
		_pos++;

	if (m_line[_pos]=='-')
		return ParseDieNumber(++_pos);

	return 0;
}

// TODO: p->m_swing_value[BME_SWING_MAX] if known at this stage
// TODO: p->m_swing_set
// PRE: if this is _die 1, then _die 0 has already been parsed
// POST: sets m_swing_type[], m_sides[], m_sides_max, and m_swing_set
void BMC_Parser::ParseDieSides(INT & _pos, INT _die)
{
	if (DieIsSwing(m_line[_pos]))
	{
		BME_SWING	swing_type = (BME_SWING)(BME_SWING_FIRST + (m_line[_pos] - BMD_FIRST_SWING_CHAR));
		m_die->m_swing_type[_die] =  swing_type;
		m_die->m_sides[_die] = 0;
		m_player->m_swing_set = BMC_Player::SWING_SET_NOT;
		_pos++;

		// check if swing sides have been defined - NOTE this does not move _pos
		INT sides = ParseDieDefinedSides(_pos);
		if (sides>0)
		{
			m_die->m_sides[_die] = sides;
			m_die->m_sides_max += sides;
			m_player->m_swing_value[swing_type] = sides;
			m_player->m_swing_set = BMC_Player::SWING_SET_LOCKED;
		}
	}
	else
	{
		m_die->m_swing_type[_die] = BME_SWING_NOT;
		m_die->m_sides[_die] = ParseDieNumber(_pos);
		m_die->m_sides_max += m_die->m_sides[_die];
	}
}

// PRE: line is already filled
// TODO: deal with option and twin dice
void BMC_Parser::ParseDie(INT _p, INT _die)
{
	BM_ERROR(_die<BMD_MAX_DICE);

	m_player = m_game.GetPlayer(_p);
	m_die = m_player->GetDie(_die);

	// save original index
	m_die->m_original_index = _die;

	// DIE DEFINITION:
	// - m_sides[2], m_properties, m_swing_type[2]

	// prefix properties
	INT pos = 0;
	m_die->m_properties = BME_PROPERTY_VALID;
	while (!DieIsValue(m_line[pos]) && !DieIsTwin(m_line[pos]))
	{
		U8 ch = m_line[pos++];
		#define	DEFINE_PROPERTY(_s, _v)	case _s: m_die->m_properties |= _v; break;

		// WARNING: don't use SWING dice here (PQRSTUVWXYZ)
		switch (ch)
		{
			DEFINE_PROPERTY('z', BME_PROPERTY_SPEED)
			DEFINE_PROPERTY('^', BME_PROPERTY_TIME_AND_SPACE)
			DEFINE_PROPERTY('+', BME_PROPERTY_AUXILIARY)
			DEFINE_PROPERTY('q', BME_PROPERTY_QUEER)
			DEFINE_PROPERTY('t', BME_PROPERTY_TRIP)
			DEFINE_PROPERTY('s', BME_PROPERTY_SHADOW)
			DEFINE_PROPERTY('d', BME_PROPERTY_STEALTH)
			DEFINE_PROPERTY('p', BME_PROPERTY_POISON)
			DEFINE_PROPERTY('n', BME_PROPERTY_NULL)
			DEFINE_PROPERTY('B', BME_PROPERTY_BERSERK)
			DEFINE_PROPERTY('f', BME_PROPERTY_FOCUS)
			DEFINE_PROPERTY('H', BME_PROPERTY_MIGHTY)
			DEFINE_PROPERTY('h', BME_PROPERTY_WEAK)
			DEFINE_PROPERTY('r', BME_PROPERTY_RESERVE)
			DEFINE_PROPERTY('o', BME_PROPERTY_ORNERY)
			DEFINE_PROPERTY('D', BME_PROPERTY_DOPPLEGANGER)
			DEFINE_PROPERTY('c', BME_PROPERTY_CHANCE)
			DEFINE_PROPERTY('m', BME_PROPERTY_MORPHING)
			DEFINE_PROPERTY('%', BME_PROPERTY_RADIOACTIVE)
			DEFINE_PROPERTY('`', BME_PROPERTY_WARRIOR)
			DEFINE_PROPERTY('w', BME_PROPERTY_SLOW)
			DEFINE_PROPERTY('u', BME_PROPERTY_UNIQUE)
			DEFINE_PROPERTY('~', BME_PROPERTY_UNSKILLED)
			DEFINE_PROPERTY('g', BME_PROPERTY_STINGER)
			DEFINE_PROPERTY('G', BME_PROPERTY_RAGE)
			DEFINE_PROPERTY('k', BME_PROPERTY_KONSTANT)
			DEFINE_PROPERTY('M', BME_PROPERTY_MAXIMUM)
			DEFINE_PROPERTY('I', BME_PROPERTY_INSULT)
			DEFINE_PROPERTY('v', BME_PROPERTY_VALUE)
		default:
			BMF_Error("error parsing die %s (pre-fix property at %c)", m_line, ch);
		}
	}

	// parse sides
	// TWIN dice
	m_die->m_sides_max = 0;
	if (DieIsTwin(m_line[pos]))
	{
		pos++;
		m_die->m_properties |= BME_PROPERTY_TWIN;
		ParseDieSides(pos, 0);
		if (m_line[pos++]!=',')
			BMF_Error("Error parsing twin die");
		ParseDieSides(pos, 1);
		if (m_line[pos++]!=')')
			BMF_Error("Error parsing twin die");
	}
	else
	{
		ParseDieSides(pos, 0);

		// OPTION dice
		if (DieIsOption(m_line[pos]))
		{
			pos++;
			m_die->m_properties |= BME_PROPERTY_OPTION;
			m_player->m_swing_set = BMC_Player::SWING_SET_NOT;
			ParseDieSides(pos, 1);
			// the previous call added to m_sides_max, so correct m_sides_max back to the # on the first die (by default)
			m_die->m_sides_max = m_die->m_sides[0];
			// check for defined sides - NOTE this does not move pos
			INT sides = ParseDieDefinedSides(pos);
			if (sides>0)
			{
				BM_ASSERT(sides==m_die->m_sides[0] || sides==m_die->m_sides[1]);
				if (sides==m_die->m_sides[1])
				{
					m_die->SetOption(1);
				}
				m_player->m_swing_set = BMC_Player::SWING_SET_LOCKED;
			} // end if sides are defined for OPTION
		} // end if OPTION die
	}

	// postfix properties
	while (pos < (INT)std::strlen(m_line) && m_line[pos] != ':')
	{
		U8 ch = m_line[pos++];
		#define	DEFINE_POST_PROPERTY(_s, _v)	case _s: m_die->m_properties |= _v; break;

		switch (ch)
		{
			DEFINE_POST_PROPERTY('!', BME_PROPERTY_TURBO)
			DEFINE_POST_PROPERTY('?', BME_PROPERTY_MOOD)
		// if we spot a '-', then we have already parsed the defined sides for a SWING or OPTION,
		// so skip the number
		case '-':
			ParseDieNumber(pos);
			break;
		default:
			BMF_Error("error parsing die %s (post-fix property at %c)", m_line, ch);
		}
	}

	// state
	if (m_die->HasProperty(BME_PROPERTY_RESERVE))
		m_die->m_state = BME_STATE_RESERVE;
	else
		m_die->m_state = BME_STATE_NOTSET;

	// actual value
	if (m_line[pos]==':')
	{
		pos++;
		if (!DieIsNumeric(m_line[pos]))
			BMF_Error("error parsing die %s (expecting current value at %c)", m_line, m_line[pos]);

		BM_ASSERT(m_die->m_state == BME_STATE_NOTSET);	// die marked as RESERVE shouldn't be in play
		m_die->m_state = BME_STATE_READY;
		m_die->m_value_total = atoi(m_line + pos);
		while (DieIsValue(m_line[++pos]))	// skip past number
		{
		}

		// FOCUS: dizzy?
		if (m_line[pos]=='d')
		{
			pos++;
			m_die->m_state = BME_STATE_DIZZY;
		}
	}

	// did we successfully determine what this die is?
	if (pos != std::strlen(m_line))
		BMF_Error( "Could not successfully parse die: %s (broken at '%c')", m_line, m_line[pos]);

	// set up attacks/vulnerabilities
	if (m_die->m_state==BME_STATE_READY)
		m_die->RecomputeAttacks();
}

void BMC_Parser::ParsePlayer(INT _p, INT _dice)
{
	BMC_Player *p = m_game.GetPlayer(_p);

	p->Reset();

	// TODO: float	m_score;

	INT i;
	for (i=0; i<_dice; i++)
	{
		ReadNextInputToLine();
		ParseDie(_p,i);
	}

	p->OnDiceParsed();
}

// USAGE: game [wins]
// wins is optional, default is 3
void BMC_Parser::ParseGame()
{
	INT wlt,i;

	// parse wins required
	if (sscanf(m_line, "game %d", &i)==1)
	{
		BMF_Log(BME_DEBUG_ALWAYS, "target wins set to %d\n", i);
		m_game.m_target_wins = i;
	}

	// TODO: parse standings
	for (wlt=0; wlt < BME_WLT_MAX; wlt++)
		m_game.m_standing[wlt] = 0;

	// parse phase
	ReadNextInputToLine();
	m_game.m_phase = BME_PHASE_MAX;
	for (i=0; i<BME_PHASE_MAX; i++)
	{
		if (!std::strcmp(c_phase_name[i], m_line))
			m_game.m_phase = (BME_PHASE)i;
	}
	if (m_game.m_phase == BME_PHASE_MAX)
		BMF_Error( "phase not found");

	// parse players
	for (i=0; i<BMD_MAX_PLAYERS; i++)
	{
		ReadNextInputToLine();
		INT p, dice;
		F32	score;
		if (sscanf(m_line, "player %d %d %f", &p, &dice, &score)<3 || p!=i)
			BMF_Error( "missing player: %s", m_line );
		ParsePlayer(p, dice);
		// don't clobber score in INITIATIVE phases
		if (m_game.GetPhase()!=BME_PHASE_INITIATIVE && m_game.GetPhase()!=BME_PHASE_INITIATIVE_CHANCE && m_game.GetPhase()!=BME_PHASE_INITIATIVE_FOCUS)
			m_game.GetPlayer(p)->m_score = score;
		// if this is still preround, then print all dice (i.e. including those not ready)
		if (m_game.IsPreround())
			m_game.GetPlayer(p)->DebugAllDice();
		else
			m_game.GetPlayer(p)->Debug();
	}

	// set up AI
	m_game.SetAI(0, &g_ai);
	m_game.SetAI(1, &g_ai);
}

BMC_Parser::BMC_Parser() : m_game(false) {
}

void BMC_Parser::ParseString(const std::string _data) {
	m_inputStream = std::stringstream(_data);
	Parse();
}

bool BMC_Parser::ReadNextInputToLine(bool _fatal)
{
	if (!m_inputStream.str().empty())
	{
		// a string was provided for input. read it line-by-line

		std::string temp;
		if (getline(m_inputStream, temp))
		{
			std::strncpy(m_line, temp.c_str(), sizeof(m_line) - 1);
			m_line[sizeof(m_line) - 1] = '\0';
			return true;
		}
		else
			return false;
	}
	else
	{
		// a file (possibly stdin) is being used for input

		if (!fgets(m_line, BMD_MAX_STRING, m_inputFile))
		{
			if (_fatal)
				BMF_Error( "missing input" );
			return false;
		}

		// remove EOL
		INT len = (INT)std::strlen(m_line);
		if (len>0 && m_line[len-1]=='\n')
			m_line[len-1] = 0;

		return true;
	}
}

void BMC_Parser::Send( const char *_fmt, ... )
{
	va_list	ap;
	va_start(ap, _fmt),
	vprintf( _fmt, ap );
	va_end(ap);
}

void BMC_Parser::SendStats()
{
	printf("stats %d/%d-%d/%d/%.2f ", g_ai.GetMaxPly(), g_ai.GetMinSims(), g_ai.GetMaxSims(), g_ai.GetMaxBranch(), s_ply_decay);
	g_stats.DisplayStats();
}

// NOTE: this parallels ApplySetSwing()
void BMC_Parser::SendSetSwing(BMC_Move &_move)
{
	if (_move.m_action == BME_ACTION_PASS)
	{
		Send("pass\n");
		return;
	}

	BM_ASSERT(_move.m_action == BME_ACTION_SET_SWING_AND_OPTION);

	INT i;

	// check swing dice
	BMC_Player *p = _move.m_game->GetPlayer(_move.m_game->m_phase_player);

	for (i=0; i<BME_SWING_MAX; i++)
	{
		if (p->GetTotalSwingDice(i)>0 && c_swing_sides_range[i][0]>0)
			Send("swing %c %d\n", BMD_FIRST_SWING_CHAR + i - BME_SWING_FIRST, _move.m_swing_value[i]);
	}

	// check option dice
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!p->GetDie(i)->IsUsed())
			continue;

		if (p->GetDie(i)->HasProperty(BME_PROPERTY_OPTION))
			Send("option %d %d\n", p->GetDie(i)->GetOriginalIndex(), p->GetDie(i)->GetSides(_move.m_option_die[i]));
	}
}

void BMC_Parser::SendUseReserve(BMC_Move &_move)
{
	if (_move.m_action == BME_ACTION_PASS)
	{
		Send("reserve -1\n");
		return;
	}

	BM_ASSERT(_move.m_action==BME_ACTION_USE_RESERVE);

	BMC_Player *p = _move.m_game->GetPlayer(_move.m_game->m_phase_player);

	Send("reserve %d\n", p->GetDie(_move.m_use_reserve)->GetOriginalIndex());
}

void BMC_Parser::SendUseChance(BMC_Move &_move)
{
	if (_move.m_action == BME_ACTION_PASS)
	{
		Send("pass\n");
		return;
	}

	BM_ASSERT(_move.m_action==BME_ACTION_USE_CHANCE);

	BMC_Player *p = _move.m_game->GetPlayer(_move.m_game->m_phase_player);

	INT i;
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (_move.m_chance_reroll.IsSet(i))
			Send("chance %d\n", p->GetDie(i)->GetOriginalIndex());
	}
}

void BMC_Parser::SendUseFocus(BMC_Move &_move)
{
	if (_move.m_action == BME_ACTION_PASS)
	{
		Send("pass\n");
		return;
	}

	BM_ASSERT(_move.m_action==BME_ACTION_USE_FOCUS);

	BMC_Player *p = _move.m_game->GetPlayer(_move.m_game->m_phase_player);

	INT i;
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		BMC_Die *d = p->GetDie(i);

		if (!d->IsUsed() || !d->HasProperty(BME_PROPERTY_FOCUS))
			continue;

		if (_move.m_focus_value[i]>0)
			Send("focus %d %d\n", d->GetOriginalIndex(), _move.m_focus_value[i]);
	}
}

// NOTE: this parallels ApplyAction()
void BMC_Parser::SendAttack(BMC_Move &_move)
{
	if (_move.m_action == BME_ACTION_PASS)
	{
		Send("pass\n");
		return;
	}

	if (_move.m_action == BME_ACTION_SURRENDER)
	{
		Send("surrender\n");
		return;
	}

	BM_ASSERT(_move.m_action == BME_ACTION_ATTACK);

	// send attack type
	Send("%s\n", c_attack_name[_move.m_attack]);

	/// ensure m_game is correct
	_move.m_game = &m_game;

	BMC_Player *attacker = _move.m_game->GetPhasePlayer();
	BMC_Player *target = _move.m_game->GetTargetPlayer();
	BMC_Die		*att_die, *tgt_die;
	INT i;

	// send attackers
	switch (c_attack_type[_move.m_attack])
	{
	case BME_ATTACK_TYPE_1_1:
	case BME_ATTACK_TYPE_1_N:
		{
			att_die = attacker->GetDie(_move.m_attacker);
			Send("%d\n", att_die->GetOriginalIndex());
			break;
		}
	case BME_ATTACK_TYPE_N_1:
		{
			int sent = 0;
			for (i=0; i<attacker->GetAvailableDice(); i++)
			{
				if (!_move.m_attackers.IsSet(i))
					continue;
				att_die = attacker->GetDie(i);
				if (sent++ > 0)
					Send(" ");
				Send("%d", att_die->GetOriginalIndex());
			}
			Send("\n");
			break;
		}
	}

	// send defenders
	switch (c_attack_type[_move.m_attack])
	{
	case BME_ATTACK_TYPE_N_1:
	case BME_ATTACK_TYPE_1_1:
		{
			tgt_die = target->GetDie(_move.m_target);
			Send("%d\n", tgt_die->GetOriginalIndex());
			break;
		}
	case BME_ATTACK_TYPE_1_N:
		{
			int sent = 0;
			for (i=0; i<target->GetAvailableDice(); i++)
			{
				if (!_move.m_targets.IsSet(i))
					continue;
				tgt_die = target->GetDie(i);
				if (sent++ > 0)
					Send(" ");
				Send("%d", tgt_die->GetOriginalIndex());
			}
			Send("\n");
			break;
		}
	}

	// send TURBO
	if (_move.m_turbo_option>=0)
	{
		INT i;
		for (i=0; i<attacker->GetAvailableDice(); i++)
		{
			att_die = attacker->GetDie(i);
			if (!att_die->HasProperty(BME_PROPERTY_TURBO))
				continue;

			if (att_die->HasProperty(BME_PROPERTY_OPTION))
			{
				Send("option %d %d\n", att_die->GetOriginalIndex(), att_die->GetSides(_move.m_turbo_option));
				break;
			}
			else // TURBO SWING
			{
				Send("swing %c %d\n", BMD_FIRST_SWING_CHAR + att_die->GetSwingType(0) - BME_SWING_FIRST, _move.m_turbo_option);
				break;
			}
		}
	}
}

void BMC_Parser::GetAction()
{
	if (m_game.m_phase == BME_PHASE_PREROUND)
	{
		INT i = 0;
		BMC_Move move;
		m_game.m_phase_player = i;	// setup phase player for the AI
		m_game.m_ai[i]->GetSetSwingAction(&m_game, move);

		SendStats();

		Send("action\n");
		SendSetSwing(move);
	}
	else if (m_game.m_phase == BME_PHASE_RESERVE)
	{
		INT i = 0;
		BMC_Move move;
		m_game.m_phase_player = i;	// setup phase player for the AI
		m_game.m_ai[i]->GetReserveAction(&m_game, move);

		SendStats();

		Send("action\n");
		SendUseReserve(move);
	}
	else if (m_game.m_phase == BME_PHASE_FIGHT)
	{
		m_game.m_phase_player = 0;
		m_game.m_target_player = 1;
		BMC_Move move;
		m_game.m_ai[0]->GetAttackAction(&m_game, move);

		SendStats();

		Send("action\n");
		SendAttack(move);
	}
	else if (m_game.m_phase == BME_PHASE_INITIATIVE_CHANCE)
	{
		m_game.m_phase_player = 0;
		m_game.m_target_player = 1;
		BMC_Move move;
		m_game.m_ai[0]->GetUseChanceAction(&m_game, move);

		SendStats();

		Send("action\n");
		SendUseChance(move);
	}
	else if (m_game.m_phase == BME_PHASE_INITIATIVE_FOCUS)
	{
		m_game.m_phase_player = 0;
		m_game.m_target_player = 1;
		BMC_Move move;
		m_game.m_ai[0]->GetUseFocusAction(&m_game, move);

		SendStats();

		Send("action\n");
		SendUseFocus(move);
	}
	else
		BMF_Error("GetAction(): Unrecognized phase");

	//Send("endaction\n");
}

void BMC_Parser::PlayGame(INT _games)
{
	if (m_game.GetPhase()!=BME_PHASE_PREROUND)
		BMF_Error("Cannot PlayGame unless it is preround");

	INT wins[2] = {0,0};

	BMC_Game	g_sim(false);
	while (_games-->0)
	{
		g_sim = m_game;
		g_sim.PlayGame();
		if (g_sim.GetStanding(0) > g_sim.GetStanding(1))
			wins[0]++;
		else
			wins[1]++;
	}

	BMF_Log(BME_DEBUG_ALWAYS, "matches over %d - %d\n", wins[0], wins[1]);
}


void BMC_Parser::CompareAI(INT _games)
{
	if (m_game.GetPhase()!=BME_PHASE_PREROUND)
		BMF_Error("Cannot PlayGame unless it is preround");

	INT wins[2] = {0,0};

	BMC_Game	g_sim(false);
	while (_games-->0)
	{
		g_sim = m_game;
		g_sim.PlayGame();
		if (g_sim.GetStanding(0) > g_sim.GetStanding(1))
			wins[0]++;
		else
			wins[1]++;
	}

	BMF_Log(BME_DEBUG_ALWAYS, "matches over %d - %d\n", wins[0], wins[1]);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// evaluating fairness
///////////////////////////////////////////////////////////////////////////////////////////////

// AI instances - for fairness test

BMC_AI					g_ai_mode0;
BMC_AI_Maximize			g_ai_mode1;
BMC_AI_MaximizeOrRandom	g_ai_mode1b(g_ai_mode0, g_ai_mode1);
BMC_BMAI				g_ai_mode2(&g_qai);
BMC_BMAI				gm_ai_mode3(&g_qai);

// DESC: written for Zomulgustar fair testing fairness
// PARAMS:
//	_mode 0 = random action
//	_mode 1 = simple maximizer
//	_mode 2 = BMAI using Maximizer/Random as QAI
//	_mode 3 = BMAI using QAI
//	_p = probability (0..1) that Maximizer/Random uses "Maximize", it will use random with probability (1-_p)
void BMC_Parser::PlayFairGames(INT _games, INT _mode, F32 _p)
{
	if (m_game.GetPhase()!=BME_PHASE_PREROUND)
		BMF_Error("Cannot PlayGame unless it is preround");

	INT wins[2][2] = {0, };	// indexes: [got_initiative] [winner]
	INT p, i;				// p here is player ID

	// setup AIs
	g_ai_mode1b.SetP(_p);
	g_ai_mode2.SetQAI(&g_ai_mode1b);
	gm_ai_mode3.SetQAI(&g_qai);

	// set ply of BMAI according to whatever the "ply" command was
	g_ai_mode2.SetMaxPly(g_ai.GetMaxPly());
	gm_ai_mode3.SetMaxPly(g_ai.GetMaxPly());

	for (p=0; p<2; p++)
	{
		if (_mode==0)
			m_game.SetAI(p, &g_ai_mode0);
		else if (_mode==1)
			m_game.SetAI(p, &g_ai_mode1);
		else if (_mode==2)
			m_game.SetAI(p, &g_ai_mode2);
		else if (_mode==3)
			m_game.SetAI(p, &gm_ai_mode3);
	}

	// play games
	INT g = 0;
	BMC_Game	g_sim(false);
	for (g=0; g<_games; g++)
	{
		g_sim = m_game;
		g_sim.PlayGame();
		if (g_sim.GetStanding(0) > g_sim.GetStanding(1))
			wins[g_sim.GetInitiativeWinner()][0]++;
		else
			wins[g_sim.GetInitiativeWinner()][1]++;
	}

	BMF_Log(BME_DEBUG_ALWAYS, "PlayFairGames: %d games, mode %d, p %f\n", _games, _mode, _p);

	for (p=0; p<2; p++)
	{
		for (i=p; i<2 && i>=0; i += (p>0) ? -1 : +1)
		{
			INT w = wins[i][p];
			INT l = wins[i][!p];
			INT g = w + l;
			BMF_Log(BME_DEBUG_ALWAYS, "P%d stats: initiative P%d games %d wins %d losses %d percent %.1f%%\n",
				p, i, g, w, l, (float)w*100/g );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
// BMC_Parser::Parse
// DESC: this is the main interface for the AI
///////////////////////////////////////////////////////////////////////////////////////////////

/*
PARSER COMMANDS

SETUP
game %1				begin parsing game setup, setting %1 as the target number of wins [%1 is an optional field, default 3]

SETTINGS
seed %1				seed the RNG, use 0 to randomize it  [default is deterministic each run]
max_sims %1			number of rollout simulations BMAI will use [default 500]
min_sims %1			when applying 'maxbranch' rules, min_sims BMAI will use [default 10]
turbo_accuracy %1	how many turbo options to consider, where 1 means consider all valid turbo options and 0 means consider only the extremes [range 0..1, default 1]
ply %1				deepest ply for BMAI to run at (uses simulations after that ply)
maxbranch %1		maximum number of total simulations to run at a ply (valid moves * simulations) [default 5000]
debug %1 %2			adjust logging settings (e.g. "debug SIMULATION 0")
debugply %1
ai %1 %2			set player %1 (0-1) to AI type %2 (0 = BMAI, 1 = QAI, 2 = BMAI v2)
surrender %1        set if AI is allowed to surrender. If off then AI will continue to play loosing positions. [default is on]

ACTIONS
playgame %1			play %1 games and output results
compare %1			play %1 games and output results of current AI vs OLD AI
playfair %1 %2 %3	play %1 games, using 'mode' %2, and 'p' %3
getaction			ask BMAI for what action it would select in the given situation
quit				terminate (same as EOF)

GAME SETUP FORMAT	use these commands after a 'game' command
%1					the phase (preround, fight, reserve, ...)
player 0 %2 %3		begin player setup, 0 = player number (0 or 1), %2 = number of dice to follow, %3 = score
%1					first die (e.g. "20:11", "!X-4:3", "2/6-6", "rz18", ...)
%1					second die, and so on
player 1 %2 %3		repeat for the second player
%1					first die
%1					second die, and so on
*/

void BMC_Parser::Parse()
{
	INT	param, param2;
	F32	fparam;
	char sparam[BMD_MAX_STRING+1];
	while (ReadNextInputToLine(false))	// non-fatal Read()
	{
		// game [wins]
		if (!std::strncmp(m_line, "game", 4))
		{
			ParseGame();
		}
		else if (sscanf(m_line, "playgame %d", &param)==1)
		{
			PlayGame(param);
		}
		else if (sscanf(m_line, "compare %d", &param)==1)
		{
			CompareAI(param);
		}
		// playfair [games] [mode] [p]
		else if (sscanf(m_line, "playfair %d %d %f", &param, &param2, &fparam)==3)
		{
			PlayFairGames(param, param2, fparam);
		}
		// ai [player] [type]
		else if (sscanf(m_line, "ai %d %d", &param, &param2)==2)
		{
			if (param2<0 || param2>=BMD_AI_TYPES)
				BMF_Error("invalid setting for ai type: %d", param2);
			if (param<0 || param>1)
				BMF_Error("invalid setting for ai player number: %d", param);
			m_game.SetAI(param, c_ai_type[param2]);
			printf("Setting AI for player %d to type %d\n", param, param2);
		}
		else if (sscanf(m_line, "max_sims %d %d", &param, &param2)==2)
		{
			BMC_AI * ai = m_game.GetAI(param);
			if (ai->IsBMAI())
			{
				((BMC_BMAI*)ai)->SetMaxSims(param2);
				printf("Setting max sims for player %d to %d\n", param, param2);
			}
		}
		else if (sscanf(m_line, "max_sims %d", &param)==1)
		{
			g_ai.SetMaxSims(param);
			printf("Setting max # simulations to %d\n", param);
		}
		else if (sscanf(m_line, "min_sims %d %d", &param, &param2)==2)
		{
			BMC_AI * ai = m_game.GetAI(param);
			if (ai->IsBMAI())
			{
				((BMC_BMAI*)ai)->SetMinSims(param2);
				printf("Setting min sims for player %d to %d\n", param, param2);
			}
		}
		else if (sscanf(m_line, "min_sims %d", &param)==1)
		{
			g_ai.SetMinSims(param);
			printf("Setting min # simulations to %d\n", param);
		}
		else if (sscanf(m_line, "turbo_accuracy %f", &fparam)==1)
		{
			s_turbo_accuracy = fparam;
			printf("Setting turbo accuracy to %f\n", s_turbo_accuracy);
		}
		else if (sscanf(m_line, "ply %d %d", &param, &param2)==2)
		{
			BMC_AI * ai = m_game.GetAI(param);
			if (ai->IsBMAI())
			{
				((BMC_BMAI*)ai)->SetMaxPly(param2);
				printf("Setting max ply for player %d to %d\n", param, param2);
			}
		}
		else if (sscanf(m_line, "ply %d", &param)==1)
		{
			g_ai.SetMaxPly(param);
			printf("Setting max ply to %d\n", param);
		}
		else if (sscanf(m_line, "debugply %d", &param)==1)
		{
			BMC_BMAI::SetDebugLevel(param);
			printf("Setting debug ply to %d\n", param);
		}
		else if (sscanf(m_line, "maxbranch %d %d", &param, &param2)==2)
		{
			BMC_AI * ai = m_game.GetAI(param);
			if (param>=0 && param<BMD_AI_TYPES && ai->IsBMAI())
			{
				((BMC_BMAI*)ai)->SetMaxBranch(param2);
				printf("Setting max branch for player %d to %d\n", param, param2);
			}
		}
		else if (sscanf(m_line, "maxbranch %d", &param)==1)
		{
			g_ai.SetMaxBranch(param);
			printf("Setting max branch to %d\n", param);
		}
		else if (!std::strcmp(m_line, "getaction"))
		{
			GetAction();
		}
		// PRE: magic # (32) must be < BMD_MAX_STRING and >= largest g_debug_name[] name
		else if (sscanf(m_line, "debug %32s %d" ,&sparam,&param)==2)
		{
			g_logger.SetLogging(sparam,param);
		}
		else if (sscanf(m_line, "seed %d", &param)==1)
		{
			g_rng.SRand(param);
			printf("Seeding with %d\n", param);
		}
        else if (sscanf(m_line, "surrender %32s", &sparam)==1)
        {
            m_game.SetSurrenderAllowed(std::string(sparam)=="on");
        }
		else if (!std::strcmp(m_line, "quit"))
		{
			return;
		}
		else if (m_line[0]==0)
		{
			;	// blank line
		}
		else
		{
			BMF_Error("unrecognized command: %s\n", m_line);
		}
	}
}
