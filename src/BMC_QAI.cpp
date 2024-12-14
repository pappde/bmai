///////////////////////////////////////////////////////////////////////////////////////////
// BMC_QAI.cpp
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp
// denis@accessdenied.net
// https://github.com/pappde/bmai
//
// DESC:
//
// REVISION HISTORY:
// drp072101	- added QAI logging
//				- corrected display of "win%" in swing action logging (was using g_sims instead of local sims)
//				- decreased QAI fuzziness from 20 to 5 (this needs work)
// dbl100524 - broke this logic out into its own class file
///////////////////////////////////////////////////////////////////////////////////////////

#include "BMC_QAI.h"

#include "BMC_Logger.h"
#include "BMC_RNG.h"

// QAI IDEAS:
// - look at delta score
// - look at my die (to be rerolled) vs its avg.  E.g. 5/20 is +5 to reroll
// - can have depth 1 look at "captureable?"

///////////////////////////////////////////////////////////////////////////////////////////
// QAI methods
///////////////////////////////////////////////////////////////////////////////////////////

// TODO: this uses ApplyAttack() which
// a) unnecessarily rerolls and optimizes dice
// b) may have mood effects that can vary the score (meaning we are taking a single sample only)
void BMC_QAI::GetAttackAction(BMC_Game *_game, BMC_Move &_move)
{
	BMC_MoveList	movelist;
	_game->GenerateValidAttacks(movelist);

	//printf("QAI p%d Valid Moves %d: \n", _game->GetPhasePlayerID(), movelist.Size());

	INT i,j;
	BMC_Game	sim(true);
	BMC_Move *	best_move = NULL;
	// drp022521 - fix uninitialized variables
	float		best_score = 0, score = 0, delta = 0;
	BMC_Player *attacker = _game->GetPhasePlayer();
	BMC_Die *	die;

	for (i=0; i<movelist.Size(); i++)
	{
		BMC_MoveAttack * attack = movelist.Get(i);
		if (attack->m_action != BME_ACTION_ATTACK)
		{
			best_move = attack;
			break;
		}

		g_logger.Log(BME_DEBUG_QAI, "QAI p%d m%d: ", _game->GetPhasePlayerID(), i);	attack->Debug(BME_DEBUG_QAI, NULL);

		sim = *_game;
		bool extra_turn = false;
		sim.SimulateAttack(*attack, extra_turn);
		score = sim.GetPhasePlayer()->GetScore() - sim.GetTargetPlayer()->GetScore();

		// modify score according to what dice are going to be re-rolled
		switch (c_attack_type[attack->m_attack])
		{
		case BME_ATTACK_TYPE_1_1:
		case BME_ATTACK_TYPE_1_N:
			die = attacker->GetDie(attack->m_attacker);
			delta = (die->GetSidesMax() + 1) * 0.5f - (float)die->GetValueTotal();
			if ( die->HasProperty(BME_PROPERTY_SHADOW))
				 score += 0;
			else
			if (die->HasProperty(BME_PROPERTY_POISON))
				score -= delta;
			else
				score += delta;
			break;
		case BME_ATTACK_TYPE_N_1:
			for (j=0; j<attacker->GetAvailableDice(); j++)
			{
				if (!attack->m_attackers.IsSet(j))
					continue;
				die = attacker->GetDie(j);
				delta = (die->GetSidesMax() + 1) * 0.5f - (float)die->GetValueTotal();
				if ( die->HasProperty(BME_PROPERTY_SHADOW))
					score += 0;
				else
				if (die->HasProperty(BME_PROPERTY_POISON))
					score -= delta;
				else
					score += delta;
			}
			break;
		}
		score += g_rng.GetRand(BMD_QAI_FUZZINESS);


		// TODO: bonus for getting an extra turn

		g_logger.Log(BME_DEBUG_QAI, "score %.2f\n", score);

		if (!best_move || score > best_score)
		{
			best_score = score;
			best_move = attack;
		}
	}

	best_move->m_game = _game;
	g_logger.Log(BME_DEBUG_QAI, "QAI p%d best move (%.1f) ", 	_game->GetPhasePlayerID(), 	best_score);	best_move->Debug(BME_DEBUG_QAI);

	_move = *best_move;
}

