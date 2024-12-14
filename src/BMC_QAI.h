///////////////////////////////////////////////////////////////////////////////////////////
// BMC_QAI.h
//
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2021 Denis Papp <denis@accessdenied.net>
//
// DESC:
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
// dbl100524 - further split out of individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "BMC_AI.h"


// Heuristic Quick AI that BMAI uses by default
class BMC_QAI : public BMC_AI
{
public:
	virtual void		GetAttackAction(BMC_Game *_game, BMC_Move &_move);


protected:
private:
};
