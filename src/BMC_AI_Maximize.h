///////////////////////////////////////////////////////////////////////////////////////////
// BMC_AI_Maximize.h
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2021 Denis Papp
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


// Selects the maximizing move
class BMC_AI_Maximize : public BMC_AI
{
public:
	virtual void		GetAttackAction(BMC_Game *_game, BMC_Move &_move);
};
