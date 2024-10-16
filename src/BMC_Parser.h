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

#include <cstdio>
#include "BMC_AI.h"
#include "BMC_BMAI.h"
#include "BMC_BMAI3.h"
#include "BMC_Die.h"
#include "BMC_QAI.h"


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
	BMC_Player		*p;
	BMC_Die			*d;
	char			line[BMD_MAX_STRING];
	FILE			*file;

	BMC_Game		m_game;
};

extern BMC_QAI		g_qai;
extern BMC_BMAI3	g_ai;
extern BMC_QAI		g_qai2;
extern BMC_BMAI		g_bmai;
extern BMC_BMAI3	g_bmai3;
