///////////////////////////////////////////////////////////////////////////////////////////
// BMC_AI_MaximizeOrRandom.h
//
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2021 Denis Papp <denis@accessdenied.net>
// SPDX-FileComment: https://github.com/pappde/bmai
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
	BMC_AI_MaximizeOrRandom(const BMC_AI & _ai_mode0, const BMC_AI & _ai_mode1)
	{
		m_ai_mode0=_ai_mode0;
		m_ai_mode1=_ai_mode1;
	};

	virtual void		GetAttackAction(BMC_Game *_game, BMC_Move &_move);

	void	SetP(F32 _p) { p = _p; }

private:
	F32	p;
	BMC_AI m_ai_mode0;
	BMC_AI m_ai_mode1;
};
