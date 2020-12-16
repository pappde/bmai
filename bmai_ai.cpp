///////////////////////////////////////////////////////////////////////////////////////////
// bmai_ai.cpp
// Copyright (c) 2001 Denis Papp. All rights reserved.
// denis@accessdenied.net
// http://www.accessdenied.net/bmai
// 
// DESC: AI code for BMAI
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
///////////////////////////////////////////////////////////////////////////////////////////

// QAI IDEAS:
// - look at delta score
// - look at my die (to be rerolled) vs its avg.  E.g. 5/20 is +5 to reroll
// - can have depth 1 look at "captureable?"

// TWO PLY?
// have it use BMAI again at level 1 (for the opponent).  Why this is expensive:
// - consider the original ONE_PLY system, at 100 simulations.  If you have 10 valid moves,
//   then you need to play the game out 10 * 100 = 1000 times.  Adding a PLY is more than
//   squaring the number of nodes, it is exponential in both number of valid moves and
//   number of simulations.  Consider TWO_PLY with only 10 simulations.  If you have 10
//   valid moves, then you need to play the game out 10 * 10 * 10 * 10 = 10000 times.

#include "bmai.h"

#define BMD_DEFAULT_SIMS	500
#define BMD_MIN_SIMS		10
#define BMD_QAI_FUZZINESS	5

// globals
BMC_QAI	g_qai;
extern BMC_RNG g_rng;
INT	g_sims = BMD_DEFAULT_SIMS;
static INT level = 0;

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
	BMC_Game	sim(TRUE);
	BMC_Move *	best_move = NULL;
	float		best_score, score, delta;
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
		BOOL extra_turn = FALSE;
		sim.SimulateAttack(*attack, extra_turn);
		score = sim.GetPhasePlayer()->GetScore() - sim.GetTargetPlayer()->GetScore();

		// modify score according to what dice are going to be re-rolled
		switch (g_attack_type[attack->m_attack])
		{
		case BME_ATTACK_TYPE_1_1:
		case BME_ATTACK_TYPE_1_N:
			die = attacker->GetDie(attack->m_attacker);
			delta = (die->GetSidesMax() + 1) * 0.5 - die->GetValueTotal();
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
				delta = (die->GetSidesMax() + 1) * 0.5 - die->GetValueTotal();
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

	g_logger.Log(BME_DEBUG_QAI, "QAI p%d best move (%.1f) ", 	_game->GetPhasePlayerID(), 	best_score);	best_move->Debug(BME_DEBUG_QAI);
	best_move->m_game = _game;

	_move = *best_move;
}

///////////////////////////////////////////////////////////////////////////////////////////
// basic AI methods
///////////////////////////////////////////////////////////////////////////////////////////

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

	// check swing dice
	for (i=0; i<BME_SWING_MAX; i++)
	{
		if (p->GetTotalSwingDice(i)>0 && g_swing_sides_range[i][0]>0)
		{
			_move.m_swing_value[i] = g_swing_sides_range[i][0];
			actions++;
		}
	}

	// check option dice
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!p->GetDie(i)->IsUsed())
			continue;

		if (p->GetDie(i)->HasProperty(BME_PROPERTY_OPTION))
		{
			_move.m_option_die[i] = 0;
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
	BMC_MoveList	movelist;
	_game->GenerateValidFocus(movelist);

	// TODO: fill
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

// TODO: account for side changes from WEAK, MIGHTY, MOOD, TURBO, BERSERK
// TODO: account for TIME_AND_SPACE, NULL
// TODO: account for prob distribution of twin TRIP targets
// DESC: compute what the value of the attack is, without performing the attack.  Accounts
// for TRIP attacks by looking at the probability of capture.
F32 BMC_AI::ScoreAttack(BMC_Game *_game, BMC_Move &_move)
{
	if (_move.m_action!=BME_ACTION_ATTACK)
		return 0;

	BMC_Game	sim(TRUE);
	sim = *_game;
	BOOL extra_turn = FALSE;
	sim.SimulateAttack(_move, extra_turn);
	return sim.GetPhasePlayer()->GetScore() - sim.GetTargetPlayer()->GetScore();

	/*
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
			score += (tgt_die->GetScore(TRUE) + tgt_die->GetScore(FALSE)) * prob_capture;
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
				score += (tgt_die->GetScore(TRUE) + tgt_die->GetScore(FALSE)) * prob_capture;
			}
			break;
		}
	}

	return score;
	*/
}

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

		//BOOL extra_turn = FALSE;
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

///////////////////////////////////////////////////////////////////////////////////////////
// BMAI methods
///////////////////////////////////////////////////////////////////////////////////////////

BMC_BMAI::BMC_BMAI() : m_qai(&g_qai)
{
	m_max_ply = 1;
	m_max_branch = 5000;
}

// DESC: every AI action increments level, but level is relative to the original non-simulation game and is reset in this 
// situation before every simulated game.  This method saves the level at the start of the evaluation for a non-simulation game.
// It also increments 'level' so that the max_ply logic works and does not make extra BMAI calls within a simulation.
void BMC_BMAI::OnStartEvaluation(INT & _enter_level)
{
	_enter_level = level;
	level++;
}

// DESC: at the end of the evaluation, for non-simulation games, restore level.  For simulation games, if level has gone
// past max_ply, then ensure BMAI is not called further in the simulation.
void BMC_BMAI::OnEndEvaluation(BMC_Game *_game, INT _enter_level)
{
	if (!_game->IsSimulation())
		level = _enter_level;
	else if (level>=m_max_ply)
	{
		INT pl;
		for (pl=0; pl<2; pl++)
			_game->SetAI(pl, m_qai);
	}
}

// DESC: if level has gone past max_ply, then ensure BMAI is not called further in the simulation.  Otherwise, ensure to
// continue to use BMAI.
void BMC_BMAI::OnPreSimulation(BMC_Game &_sim)
{
	// use QAI for later actions
	INT p;
	for (p=0; p<2; p++)
	{
		if (level<m_max_ply)
			_sim.SetAI(p, this);
		else
			_sim.SetAI(p, m_qai);
	}
}

// DESC: restore level if this is the top call to BMAI (within the non-simulation game) so that it correctly
// reuses BMAI in the next recursive call
void BMC_BMAI::OnPostSimulation(BMC_Game *_game, INT _enter_level)
{
	if (!_game->IsSimulation())
		level = _enter_level + 1;
}

// DESC: based on m_max_branch and g_sims, determine a correct number of sims to not exceed m_max_branch
INT BMC_BMAI::ComputeNumberSims(INT _moves)
{
	INT sims = m_max_branch / _moves;

	if (sims > g_sims)
		sims = g_sims;
	else if (sims < BMD_MIN_SIMS)
		sims = BMD_MIN_SIMS;

	return sims;
}

// PRE: this is the phasing player
void BMC_BMAI::GetAttackAction(BMC_Game *_game, BMC_Move &_move)
{
	BMC_MoveList	movelist;
	_game->GenerateValidAttacks(movelist);

	INT enter_level;
	OnStartEvaluation(enter_level);

	INT			sims = ComputeNumberSims(movelist.Size());
	g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d Valid Moves %d Sims %d\n", level, _game->GetPhasePlayerID(), movelist.Size(), sims);

	INT i, s;
	BMC_Game	sim(TRUE);
	BMC_Move *	best_move = NULL;
	float		best_score, score;
	for (i=0; i<movelist.Size(); i++)
	{
		BMC_MoveAttack * attack = movelist.Get(i);
		if (attack->m_action != BME_ACTION_ATTACK)
		{
			best_move = attack;
			break;
		}

		g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d m%d: ", level, _game->GetPhasePlayerID(), i);	
		attack->Debug(BME_DEBUG_SIMULATION);

		score = 0;
		for (s=0; s<sims; s++)
		{
			//BMF_Log(BME_DEBUG_SIMULATION, "l%d m%d sim #%d: ", level, i, s);	attack->Debug();
			sim = *_game;

			OnPreSimulation(sim);
			BME_WLT rv = sim.PlayRound(attack);
			OnPostSimulation(_game, enter_level);

			// reverse score if this is player 1, since WLT is wrt player 0
			if (rv==BME_WLT_TIE)
				score += 0.5f;
			else if ((rv==BME_WLT_WIN) ^ (_game->GetPhasePlayerID()!=0))
				score += 1.0f;
		}

		g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d m%d: score %.1f - ", level, _game->GetPhasePlayerID(), i, score);	
		attack->Debug(BME_DEBUG_SIMULATION);

		//printf("l%d p%d m%d score %f\n", level, _game->GetPhasePlayerID(), i, score);
		if (!best_move || score > best_score)
		{
			best_score = score;
			best_move = attack;
		}
	}

	g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d best move (%.1f points, %.1f%% win) ", 
		level,
		_game->GetPhasePlayerID(), 
		best_score,
		best_score / sims * 100);
	best_move->m_game = _game;
	best_move->Debug(BME_DEBUG_SIMULATION);

	OnEndEvaluation(_game, enter_level);

	_move = *best_move;
}

// TODO: currently, this just uses min value or first option die
void BMC_BMAI::GetSetSwingAction(BMC_Game *_game, BMC_Move &_move)
{
	// array of valid values
	struct SWING_ACTION {
		BOOL		swing;	// or option
		INT			index;	// swing type or die
		INT			value;	// from [min..max] (or [0..1] for option)
	};

	SWING_ACTION	swing_action[BME_SWING_MAX + BMD_MAX_DICE];
	INT				actions = 0;
	INT				combinations = 1;
	INT i, s, p;

	_move.m_game = _game;
	_move.m_action = BME_ACTION_SET_SWING_AND_OPTION;

	BMC_Player *pl = _game->GetPhasePlayer();

	// walk dice and determine possible things to set
	for (i=0; i<BME_SWING_MAX; i++)
	{
		if (pl->GetTotalSwingDice(i)>0 && g_swing_sides_range[i][0]>0)
		{
			swing_action[actions].swing = TRUE;
			swing_action[actions].index = i;
			swing_action[actions].value = g_swing_sides_range[i][0];	// initialize to min
			actions++;
			combinations *= (g_swing_sides_range[i][1]-g_swing_sides_range[i][0])+1;
			_move.m_swing_value[i] = g_swing_sides_range[i][0];
		}
	}

	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!pl->GetDie(i)->IsUsed())
			continue;

		if (pl->GetDie(i)->HasProperty(BME_PROPERTY_OPTION))
		{
			swing_action[actions].swing = FALSE;
			swing_action[actions].index = i;
			swing_action[actions].value = 0;	// initialize to first die
			actions++;
			combinations *= 2;
			_move.m_option_die[i] = 0;
		}
	}

	INT			sims = ComputeNumberSims(combinations);

	if (level<1)
		g_logger.Log(BME_DEBUG_SIMULATION, "swing action combinations %d sims %d\n", combinations, sims);

	if (!actions)
	{
		_move.m_action = BME_ACTION_PASS;
		return;
	}

	INT enter_level;
	OnStartEvaluation(enter_level);

	// now iterate over all combinations of actions
	// TODO: stratify, at least in situations with a lot of moves
	INT max;
	BMC_Game	sim(TRUE);
	BMC_Move 	best_move;
	float		best_score = -1, score;
	do
	{
		// try case
		score = 0;
		for (s=0; s<sims; s++)
		{
			//BMF_Log(BME_DEBUG_SIMULATION, "l%d m%d sim #%d: ", level, i, s);	attack->Debug();
			sim = *_game;

			OnPreSimulation(sim);

			sim.ApplySetSwing(_move);
			BME_WLT rv = sim.PlayRound(NULL);
			OnPostSimulation(_game, enter_level);

			// reverse score if this is player 1, since WLT is wrt player 0
			if (rv==BME_WLT_TIE)
				score += 0.5f;
			else if ((rv==BME_WLT_WIN) ^ (_game->GetPhasePlayerID()!=0))
				score += 1.0f;
		}

		if (enter_level<1)
		{
			g_logger.Log(BME_DEBUG_SIMULATION, "swing sims over - score %.1f - ", score);	
			_move.Debug(BME_DEBUG_SIMULATION);
		}

		//printf("l%d p%d m%d score %f\n", level, _game->GetPhasePlayerID(), i, score);
		if (score > best_score)
		{
			best_score = score;
			best_move = _move;
		}

		// increment step
		p = actions-1;
		while (1)
		{
			swing_action[p].value++;

			// update move
			if (swing_action[p].swing)
				_move.m_swing_value[swing_action[p].index]++;
			else
				_move.m_option_die[swing_action[p].index]++;

			// past max?
			if (swing_action[p].swing)
				max = g_swing_sides_range[swing_action[p].index][1];
			else
				max = 1;
			if (swing_action[p].value <= max)
				break;

			// we are past max - reset to min and cycle the next value
			if (swing_action[p].swing)
			{
				swing_action[p].value = g_swing_sides_range[swing_action[p].index][0];
				_move.m_swing_value[swing_action[p].index] = swing_action[p].value;
			}
			else
			{
				swing_action[p].value = 0;
				_move.m_option_die[swing_action[p].index] = 0;
			}

			p--;

			// finished?
			if (p<0)
				break;
		} // end while(1) - increment step
	} while (p>=0);

	g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d best move swing (%.1f points, %.1f%% win) ", 
		level,
		_game->GetPhasePlayerID(), 
		best_score,
		best_score / sims * 100);
	best_move.m_game = _game;
	best_move.Debug(BME_DEBUG_SIMULATION);

	OnEndEvaluation(_game, enter_level);

	_move = best_move;
}

void BMC_BMAI::GetReserveAction(BMC_Game *_game, BMC_Move &_move)
{
	_move.m_game = _game;
	_move.m_action = BME_ACTION_USE_RESERVE;

	BMC_Player *p = _game->GetPhasePlayer();

	INT			i,s;
	BMC_Move 	best_move;
	float		best_score = -1, score;
	BMC_Game	sim(TRUE);

	INT enter_level;
	OnStartEvaluation(enter_level);

	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!p->GetDie(i)->IsInReserve())
			continue;

		// found an action
		_move.m_use_reserve = i;

		// evaluate action
		score = 0;
		for (s=0; s<g_sims; s++)
		{
			//BMF_Log(BME_DEBUG_SIMULATION, "l%d m%d sim #%d: ", level, i, s);	attack->Debug();
			sim = *_game;

			OnPreSimulation(sim);
			sim.ApplyUseReserve(_move);
			BME_WLT rv = sim.PlayRound(NULL);
			OnPostSimulation(_game, enter_level);

			// reverse score if this is player 1, since WLT is wrt player 0
			if (rv==BME_WLT_TIE)
				score += 0.5f;
			else if ((rv==BME_WLT_WIN) ^ (_game->GetPhasePlayerID()!=0))
				score += 1.0f;
		}

		if (enter_level<1)
		{
			g_logger.Log(BME_DEBUG_SIMULATION, "reserve sims over - score %.1f - ", score);	
			_move.Debug(BME_DEBUG_SIMULATION);
		}

		//printf("l%d p%d m%d score %f\n", level, _game->GetPhasePlayerID(), i, score);
		if (score > best_score)
		{
			best_score = score;
			best_move = _move;
		}
	}

	// should not be calling GetReserveAction() if there are no valid moves
	BM_ASSERT(best_score>=0);

	// evaluate the "no reserve" move
	_move.m_action = BME_ACTION_PASS;

	score = 0;
	for (s=0; s<g_sims; s++)
	{
		//BMF_Log(BME_DEBUG_SIMULATION, "l%d m%d sim #%d: ", level, i, s);	attack->Debug();
		sim = *_game;

		OnPreSimulation(sim);
		sim.ApplyUseReserve(_move);
		BME_WLT rv = sim.PlayRound(NULL);
		OnPostSimulation(_game, enter_level);

		// reverse score if this is player 1, since WLT is wrt player 0
		if (rv==BME_WLT_TIE)
			score += 0.5f;
		else if ((rv==BME_WLT_WIN) ^ (_game->GetPhasePlayerID()!=0))
			score += 1.0f;
	}

	if (enter_level<1)
	{
		g_logger.Log(BME_DEBUG_SIMULATION, "reserve pass sims over - score %.1f - ", score);	
		_move.Debug(BME_DEBUG_SIMULATION);
	}

	//printf("l%d p%d m%d score %f\n", level, _game->GetPhasePlayerID(), i, score);
	if (score > best_score)
	{
		best_score = score;
		best_move = _move;
	}

	g_logger.Log(BME_DEBUG_SIMULATION, "p%d best move reserve (%.1f points, %.1f%% win) ", 
		_game->GetPhasePlayerID(), 
		best_score,
		best_score / g_sims * 100);
	best_move.m_game = _game;
	best_move.Debug(BME_DEBUG_SIMULATION);

	OnEndEvaluation(_game, enter_level);

	_move = best_move;
}


// PRE: we are m_phase_player 
void BMC_BMAI::GetUseChanceAction(BMC_Game *_game, BMC_Move &_move)
{
	BM_ASSERT(_game->GetPhase() == BME_PHASE_INITIATIVE_CHANCE);

	_move.m_game = _game;

	BMC_Player *p = _game->GetPhasePlayer();

	INT			i,s,j;
	BMC_Move 	best_move;
	float		best_score = -1, score;
	BMC_Game	sim(TRUE);

	INT enter_level;
	OnStartEvaluation(enter_level);

	// compute number of chance dice
	INT num_chance_dice = 0;
	INT chance_die_index[BMD_MAX_DICE];
	INT combinations = 1;
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!p->GetDie(i)->IsUsed())
			continue;
		if (!p->GetDie(i)->HasProperty(BME_PROPERTY_CHANCE))
			continue;
		chance_die_index[num_chance_dice++] = i;
		combinations *= 2;
	}

	BM_ASSERT(num_chance_dice>0);

	// evaluate each action (i represents the 'bits' of which chance dice to reroll, 0 means pass)
	for (i=0; i<combinations; i++)
	{
		// PASS action
		if (i==0)
		{
			_move.m_action = BME_ACTION_PASS;
		}
		// USE_CHANCE action - determine dice
		else
		{
			_move.m_action = BME_ACTION_USE_CHANCE;
			_move.m_chance_reroll.Clear();
			
			for (j=0; j<num_chance_dice; j++)
			{
				if (i & (1<<j))
					_move.m_chance_reroll.Set(chance_die_index[j]);
			}
		}

		BMF_Log(BME_DEBUG_SIMULATION, "l%d chance mask %x: ", level, i); _move.Debug();

		// evaluate action
		score = 0;
		for (s=0; s<g_sims; s++)
		{
			//BMF_Log(BME_DEBUG_SIMULATION, "l%d m%d sim #%d: ", level, i, s);	attack->Debug();
			sim = *_game;

			OnPreSimulation(sim);
			sim.ApplyUseChance(_move);
			BME_WLT rv = sim.PlayRound(NULL);
			OnPostSimulation(_game, enter_level);

			// reverse score if this is player 1, since WLT is wrt player 0
			if (rv==BME_WLT_TIE)
				score += 0.5f;
			else if ((rv==BME_WLT_WIN) ^ (_game->GetPhasePlayerID()!=0))
				score += 1.0f;
		}

		if (enter_level<1)
		{
			g_logger.Log(BME_DEBUG_SIMULATION, "chance sims over - score %.1f - ", score);	
			_move.Debug(BME_DEBUG_SIMULATION);
		}

		//printf("l%d p%d m%d score %f\n", level, _game->GetPhasePlayerID(), i, score);
		if (score > best_score)
		{
			best_score = score;
			best_move = _move;
		}
	}

	g_logger.Log(BME_DEBUG_SIMULATION, "p%d best move chance (%.1f points, %.1f%% win) ", 
		_game->GetPhasePlayerID(), 
		best_score,
		best_score / g_sims * 100);
	best_move.m_game = _game;
	best_move.Debug(BME_DEBUG_SIMULATION);

	OnEndEvaluation(_game, enter_level);

	_move = best_move;
}

