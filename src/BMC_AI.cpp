///////////////////////////////////////////////////////////////////////////////////////////
// BMC_AI.cpp
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
//
// DESC:
//
// REVISION HISTORY:
// drp051001 - implemented GetSetSwingAction() with simulations
// drp051101 - moved level to be global so that it applies to both Swing and Attack
// drp051201 -	implemented GetReserveAction
//			 -	implemented m_simulation in BMC_Game and reorganized how 'level' works (now very much a hack)
//				This prevents a ply-2 BMAI from running	the full AI evaluation at each step within a given simulation.
// drp061601 - winning % was displaying as too low if sims were reduced
// drp072101	- added QAI logging
//				- corrected display of "win%" in swing action logging (was using g_sims instead of local sims)
//				- decreased QAI fuzziness from 20 to 5 (this needs work)
// drp072101	- added QAI logging
//				- corrected display of "win%" in swing action logging (was using g_sims instead of local sims)
//				- decreased QAI fuzziness from 20 to 5 (this needs work)
// drp033102 - added BMAI2 which iteratively runs simulations across all moves and culls moves based on an
//			   interpolated score threshold (vs the best move).  This allows the ply to be increased to 3. Replaced 'g_ai'
// drp040102 - smarter culling (such as ignoring cases with 0% moves but delta_points was close)
//			 - also culling out moves where percentage is close but there aren't enough sims left to catch up to best_move
//			 - [script] increasing to ply 4
//			 - moved g_sims to m_sims
// drp062702 - more aggressive culling of TRIP, and more aggressive culling of 0-point moves
// dbl100824 - migrated this logic from bmai_ai.cpp
///////////////////////////////////////////////////////////////////////////////////////////

// TWO PLY?
// have it use BMAI again at level 1 (for the opponent).  Why this is expensive:
// - consider the original ONE_PLY system, at 100 simulations.  If you have 10 valid moves,
//   then you need to play the game out 10 * 100 = 1000 times.  Adding a PLY is more than
//   squaring the number of nodes, it is exponential in both number of valid moves and
//   number of simulations.  Consider TWO_PLY with only 10 simulations.  If you have 10
//   valid moves, then you need to play the game out 10 * 10 * 10 * 10 = 10000 times.

#include "BMC_AI.h"

#include "BMC_Game.h"
#include "BMC_Logger.h"
#include "BMC_RNG.h"


class BMC_Player;
// DESC: basic behavior is to pick first available reserve (probably is the lowest)
void BMC_AI::GetReserveAction(BMC_Game *_game, BMC_Move &_move)
{
	_move.m_game = _game;
	_move.m_action = BME_ACTION_USE_RESERVE;

	BMC_Player *p = _game->GetPhasePlayer();

	INT i;
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!p->GetDie(i)->IsInReserve())
			continue;

		_move.m_use_reserve = i;
		return;
	}

	_move.m_action = BME_ACTION_PASS;
}

// TODO: currently, this just uses min value or first option die
void BMC_AI::GetSetSwingAction(BMC_Game *_game, BMC_Move &_move)
{
	//BMC_MoveList	movelist;
	//_game->GenerateValidPreround(movelist);

	_move.m_game = _game;
	_move.m_action = BME_ACTION_SET_SWING_AND_OPTION;

	BMC_Player *p = _game->GetPhasePlayer();

	int actions = 0;
	INT i;

	// reset all swing/option settings so can use ValidSetSwing()
	for (i=0; i<BME_SWING_MAX; i++)
		_move.m_swing_value[i] = 0;

	for (i=0; i<BMD_MAX_DICE; i++)
		_move.m_option_die.Set(i, 0);

	// check swing dice
	// PRE: since using ValidSetSwing() all swing/option dice need to have valid settings
	for (i=0; i<BME_SWING_MAX; i++)
	{
		if (p->GetTotalSwingDice(i)>0 && c_swing_sides_range[i][0]>0)
		{
			_move.m_swing_value[i] = c_swing_sides_range[i][0];	// set first value
			actions++;
		}
	}

	// if necessary cycle through all options - necessary for UNIQUE
	if (!_game->ValidSetSwing(_move))
	{
		for (i=0; i<BME_SWING_MAX; i++)
		{
			if (p->GetTotalSwingDice(i)>0 && c_swing_sides_range[i][0]>0)
			{
				int v = c_swing_sides_range[i][0];
				do
				{
					BM_ASSERT(v<=c_swing_sides_range[i][1]);
					_move.m_swing_value[i] = v++;
				}
				while (!_game->ValidSetSwing(_move));
			}
		}

	}

	// check option dice
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!p->GetDie(i)->IsUsed())
			continue;

		if (p->GetDie(i)->HasProperty(BME_PROPERTY_OPTION))
		{
			_move.m_option_die.Set(i, 0);
			actions++;
		}
	}

	// if no swing or option, pass
	if (actions == 0)
	{
		_move.m_action = BME_ACTION_PASS;
		return;
	}
}

void BMC_AI::GetUseFocusAction(BMC_Game *_game, BMC_Move &_move)
{
	_move.m_action = BME_ACTION_PASS;
}

void BMC_AI::GetUseChanceAction(BMC_Game *_game, BMC_Move &_move)
{
	_move.m_action = BME_ACTION_PASS;
}

// DESC: picks a random move
void BMC_AI::GetAttackAction(BMC_Game *_game, BMC_Move &_move)
{
	BMC_MoveList	movelist;
	_game->GenerateValidAttacks(movelist);

	_move = *movelist.Get(g_rng.GetRand(movelist.Size()));
}

// DESIRED: compute what the value of the attack is, without performing the attack.  Accounts
// for TRIP attacks by looking at the probability of capture.
// ACTUAL: simulates the attack once and examines the score differential for that one sampled move
// TODO: account for TRIP attacks probabilistically
F32 BMC_AI::ScoreAttack(BMC_Game *_game, BMC_Move &_move)
{
	if (_move.m_action!=BME_ACTION_ATTACK)
		return 0;

	BMC_Game	sim(true);
	sim = *_game;
	bool extra_turn = false;
	sim.SimulateAttack(_move, extra_turn);
	return sim.GetPhasePlayer()->GetScore() - sim.GetTargetPlayer()->GetScore();

	/* The cheaper yet more complex ad hoc way of evaluating a move.

	// TODO: account for side changes from WEAK, MIGHTY, MOOD, TURBO, BERSERK
	// TODO: account for TIME_AND_SPACE, NULL
	// TODO: account for prob distribution of twin TRIP targets

	F32	score = 0;
	BMC_Die *tgt_die;
	F32 prob_capture = 1.0f;
	BMC_Player *target = _game->GetTargetPlayer();

	// TRIP - only a certain percentage will capture
	if (_move.m_attack == BME_ATTACK_TRIP)
	{
		BMC_Die *att_die = _game->GetPhasePlayer()->GetDie(_move.m_attacker);
		tgt_die = target->GetDie(_move.m_target);
		// if Dice()>1 then the distribution is more complex
		BM_ASSERT(att_die->Dice()==1);
		BM_ASSERT(tgt_die->Dice()==1);
		// If Na<=Nt
		// => P(capture) = (1+2+3+...Na) / (Nt * Na) = ((Na+1) * Na / 2) / (Nt * Na) = (Na+1) / (Nt * 2)
		// If Na>Nt
		// => P(capture) = (1+2+3+...Nt) / (Nt * Na) + (Na-Nt) / Na
		// => (first term) ((Nt+1)*Nt/2) / (Nt * Na) = (Nt+1) / (2 * Na)
		// => P(capture) = (Nt+1) / (2*Na) - Nt / Na + 1
		// => P(capture) = (1 - Nt) / (2*Na) + 1
		F32 Na = (float)att_die->GetSidesMax();
		F32 Nt = (float)tgt_die->GetSidesMax();
		if (Na<=Nt)
			prob_capture =  (Na + 1) / (Nt * 2);
		else
			prob_capture =  (1.0f - Nt) / (Na * 2) + 1;
	}

	// add value of dice captured + negative of value to opponent
	switch (g_attack_type[_move.m_attack])
	{
	case BME_ATTACK_TYPE_N_1:
	case BME_ATTACK_TYPE_1_1:
		{
			tgt_die = target->GetDie(_move.m_target);
			score += (tgt_die->GetScore(true) + tgt_die->GetScore(false)) * prob_capture;
			break;
		}
	case BME_ATTACK_TYPE_1_N:
		{
			INT i;
			for (i=0; i<target->GetAvailableDice(); i++)
			{
				if (!_move.m_targets.IsSet(i))
					continue;
				tgt_die = target->GetDie(i);
				score += (tgt_die->GetScore(true) + tgt_die->GetScore(false)) * prob_capture;
			}
			break;
		}
	}

	return score;
	*/
}
