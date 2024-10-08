///////////////////////////////////////////////////////////////////////////////////////////
// BMC_AI.h
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
//
// DESC: main header for BMAI AI classes
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
// dbl100824 - migrated this logic from bmai_ai.h
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "BMC_Game.h"


class BMC_Move;

// basic AI class
class BMC_AI
{
public:

	virtual void	GetSetSwingAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetAttackAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetReserveAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetUseChanceAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetUseFocusAction(BMC_Game *_game, BMC_Move &_move);

	void		GetUseAuxiliaryAction(BMC_Game *_game, BMC_Move &_move);
	void		GetSetTurboAction(BMC_Game *_game, BMC_Move &_move);

	F32			ScoreAttack(BMC_Game *_game, BMC_Move &_move);

	// class testing
	virtual		bool	IsBMAI() { return false; }
	virtual		bool	IsBMAI3() { return true; }

protected:

private:
	BMC_AI *	m_qai;
};
