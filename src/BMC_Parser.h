///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Parser.h
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: main parsing class for setting up game state
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
// dbl100524 - further split out of individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "BMC_AI.h"
#include "BMC_AI_Maximize.h"
#include "BMC_AI_MaximizeOrRandom.h"
#include "BMC_BMAI.h"
#include "BMC_Die.h"


class BMC_Parser
{
public:
	BMC_Parser();
	void	ParseGame();
	void	Parse();
	void	Parse(FILE *_fp) { file = _fp; Parse(); }

protected:
	void			GetAction();
	void			PlayGame(INT _games);
	void			CompareAI(INT _games);
	void			PlayFairGames(INT _games, INT _mode, F32 _p);
	void			ParseDie(INT _p, INT _dice);
	void			ParsePlayer(INT _p, INT _dice);
	bool			Read(bool _fatal = true);

	// output

    // virtual can be overridden in subclass. makes testing a little easier
	virtual void	Send(const char *_fmt, ...);
	void			SendStats();
	void			SendSetSwing(BMC_Move &_move);
	void			SendUseReserve(BMC_Move &_move);
	void			SendAttack(BMC_Move &_move);
	void			SendUseChance(BMC_Move &_move);
	void			SendUseFocus(BMC_Move &_move);

	// parsing dice
	bool			DieIsSwing(char _c) { return _c >= BMD_FIRST_SWING_CHAR && _c <= BMD_LAST_SWING_CHAR; }
	bool			DieIsNumeric(char _c) { return (_c >= '0' && _c <= '9'); }
	bool			DieIsValue(char _c) { return DieIsSwing(_c) || DieIsNumeric(_c); }
	bool			DieIsTwin(char _c) { return _c == '('; }
	bool			DieIsOption(char _c) { return _c == '/'; }

private:
	// parsing dice methods (uses 'line')
	INT				ParseDieNumber(INT & _pos);
	void			ParseDieSides(INT & _pos, INT _die);
	INT				ParseDieDefinedSides(INT _pos);

	// state for dice parsing
	BMC_Player *p;
	BMC_Die *d;
	char			line[BMD_MAX_STRING];
	FILE *file;
};

extern BMC_AI					g_ai_mode0;
extern BMC_AI_Maximize			g_ai_mode1;
extern BMC_AI_MaximizeOrRandom	g_ai_mode1b;
extern BMC_BMAI					g_ai_mode2;
extern BMC_BMAI					g_ai_mode3;