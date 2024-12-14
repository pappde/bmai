///////////////////////////////////////////////////////////////////////////////////////////
// BMC_AI_MaximizeOrRandom.cpp
//
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp <denis@accessdenied.net>
// SPDX-FileComment: https://github.com/pappde/bmai
//
// DESC:
//
// REVISION HISTORY:
// dbl100824 - migrated this logic from bmai_ai.cpp
///////////////////////////////////////////////////////////////////////////////////////////

#include "BMC_AI_Maximize.h"

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_AI_Maximize methods
///////////////////////////////////////////////////////////////////////////////////////////

void BMC_AI_Maximize::GetAttackAction(BMC_Game *_game, BMC_Move &_move)
{
	BMC_MoveList	movelist;
	_game->GenerateValidAttacks(movelist);

	INT i;
	//BMC_Game	sim;
	BMC_Move *	best_move = NULL;
	float		best_score, score;
	BMC_Player *attacker = _game->GetPhasePlayer();
	for (i=0; i<movelist.Size(); i++)
	{
		BMC_MoveAttack * attack = movelist.Get(i);
		if (attack->m_action != BME_ACTION_ATTACK)
		{
			best_move = attack;
			break;
		}

		//sim = *_game;

		//bool extra_turn = false;
		//sim.ApplyAttack(*attack, extra_turn);
		//score = sim.GetPhasePlayer()->GetScore() - sim.GetTargetPlayer()->GetScore();
		score = ScoreAttack(_game, *attack);

		if (!best_move || score > best_score)
		{
			best_score = score;
			best_move = attack;
		}
	}

	best_move->m_game = _game;

	_move = *best_move;
}
