///////////////////////////////////////////////////////////////////////////////////////////
// BMC_AI_MaximizeOrRandom.cpp
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
//
// DESC:
//
// REVISION HISTORY:
// dbl100824 - migrated this logic from bmai_ai.cpp
///////////////////////////////////////////////////////////////////////////////////////////

#include "BMC_AI_MaximizeOrRandom.h"

#include "BMC_Parser.h"
#include "BMC_RNG.h"


void BMC_AI_MaximizeOrRandom::GetAttackAction(BMC_Game *_game, BMC_Move &_move)
{
	if (g_rng.GetFRand()<p)
		m_ai_mode1.GetAttackAction(_game, _move);
	else
		m_ai_mode0.GetAttackAction(_game, _move);
}
