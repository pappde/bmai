///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Parser.h
//
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2021 Denis Papp <denis@accessdenied.net>
// SPDX-FileComment: https://github.com/pappde/bmai
// 
// DESC: main parsing class for setting up game state
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
// dbl100524 - further split out of individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdio>
#include <sstream>

#include "BMC_AI.h"
#include "BMC_BMAI.h"
#include "BMC_BMAI3.h"
#include "BMC_Die.h"
#include "BMC_QAI.h"


class BMC_Parser
{
public:
	BMC_Parser();
	void	ParseStdIn() { m_inputFile = stdin; Parse(); }
	void	ParseFile(FILE *_fp) { m_inputFile = _fp; Parse(); }
	void	ParseString(std::string  _data);

protected:
	void			GetAction();
	void			PlayGame(INT _games);
	void			CompareAI(INT _games);
	void			PlayFairGames(INT _games, INT _mode, F32 _p);
	void			ParseDie(INT _p, INT _dice);
	void			ParseGame();
	void			ParsePlayer(INT _p, INT _dice);
	bool			ReadNextInputToLine(bool _fatal = true);

	// output

    // virtual can be overridden in subclass. makes testing a little easier
	virtual void	Send(const char *_fmt, ...);
	void			SendStats();
	void			SendSetSwing(BMC_Move &_move);
	void			SendUseReserve(BMC_Move &_move);
	virtual void	SendAttack(BMC_Move &_move);
	void			SendUseChance(BMC_Move &_move);
	void			SendUseFocus(BMC_Move &_move);

	// parsing dice
	bool			DieIsSwing(char _c) { return _c >= BMD_FIRST_SWING_CHAR && _c <= BMD_LAST_SWING_CHAR; }
	bool			DieIsNumeric(char _c) { return (_c >= '0' && _c <= '9'); }
	bool			DieIsValue(char _c) { return DieIsSwing(_c) || DieIsNumeric(_c); }
	bool			DieIsTwin(char _c) { return _c == '('; }
	bool			DieIsOption(char _c) { return _c == '/'; }

private:
	void			Parse();

	// parsing dice methods (uses 'line')
	INT				ParseDieNumber(INT & _pos);
	void			ParseDieSides(INT & _pos, INT _die);
	INT				ParseDieDefinedSides(INT _pos);

	// state for dice parsing
	BMC_Player		*m_player;
	BMC_Die			*m_die;
	char			m_line[BMD_MAX_STRING];
	FILE			*m_inputFile;

	BMC_Game		m_game;

	// support reading strings in addition to files
	std::stringstream m_inputStream;
};

extern BMC_QAI		g_qai;
extern BMC_BMAI3	g_ai;
extern BMC_QAI		g_qai2;
extern BMC_BMAI		g_bmai;
extern BMC_BMAI3	g_bmai3;
