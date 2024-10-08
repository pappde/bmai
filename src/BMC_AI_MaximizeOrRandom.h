///////////////////////////////////////////////////////////////////////////////////////////
// BMC_AI_MaximizeOrRandom.h
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
//
// DESC:
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
// dbl100824 - migrated this logic from bmai_ai.h
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "BMC_AI.h"


// Randomly selects between a basic AI and a maximizing AI
// P(p) use Maximize (mode 1), else Basic (mode 0)
class BMC_AI_MaximizeOrRandom : public BMC_AI
{
public:
	virtual void		GetAttackAction(BMC_Game *_game, BMC_Move &_move);

	void	SetP(F32 _p) { p = _p; }

private:
	F32	p;
};
