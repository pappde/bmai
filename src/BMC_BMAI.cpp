///////////////////////////////////////////////////////////////////////////////////////////
// BMC_BMAI.cpp
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
//
// DESC:
//
// REVISION HISTORY:
// dbl100824 - migrated this logic from bmai_ai.cpp
///////////////////////////////////////////////////////////////////////////////////////////

#include "BMC_BMAI.h"

#include "bmai_ai.h"
#include "BMC_Logger.h"
#include "BMC_Stats.h"


// HACK: sm_level should be automatically updated in all eval methods, but it is currently only updated
// by proper use of OnStartEvaluation() and OnEndEvaluation() which BMAI and BMAI3 are trusted to call.
INT BMC_BMAI::sm_level = 0;

// only output certain debug strings when 'sm_level' is <= to this. Trusts AI classes to use before calling Log().
INT BMC_BMAI::sm_debug_level = 2;		// default so only up to level 2 is output

///////////////////////////////////////////////////////////////////////////////////////////
// BMAI methods
///////////////////////////////////////////////////////////////////////////////////////////

BMC_BMAI::BMC_BMAI() : m_qai(&g_qai)
{
	m_max_ply = 1;
	m_max_branch = 5000;
	m_max_sims = BMD_DEFAULT_SIMS;
	m_min_sims = BMD_MIN_SIMS;
}

// DESC: every AI action increments level to represent recursive depth in BMAI evaluations.  Level is
// relative to the original non-simulation game which sparked the first GetAction().  This method
// also saves the level at the start of the BMAI evaluation in '_enter_level'.  The reason for
// incrementing 'level' is so that max_ply logic works. Also useful for debugging.
void BMC_BMAI::OnStartEvaluation(BMC_Game *_game, INT & _enter_level)
{
	_enter_level = sm_level++;
}

// otherwise is effectively LEVEL_INCREMENT_IN_SIM
//#define LEVEL_INCREMENT_RECURSIVE


// DESC:for non-simulation, at the end of the evaluation restore level so that the next GetAction() in the game uses BMAI
//		for simulation, switch to QAI at max ply for the rest of the simulation
void BMC_BMAI::OnEndEvaluation(BMC_Game *_game, INT _enter_level)
{
#ifdef LEVEL_INCREMENT_RECURSIVE
	sm_level = _enter_level;
#else
	if (!_game->IsSimulation())
		sm_level = _enter_level;

	// In a simulation game, if level has gone past max_ply, ensure BMAI is not called further
	else if (sm_level>=m_max_ply)
	{
		INT pl;
		for (pl=0; pl<2; pl++)
			_game->SetAI(pl, m_qai);
	}
#endif
}

// DESC: if level has gone past max_ply, then ensure BMAI is not used in this simulation.  Otherwise, ensure to
// continue to use BMAI.
void BMC_BMAI::OnPreSimulation(BMC_Game &_sim)
{
	BM_ASSERT(_sim.IsSimulation());

	// use QAI for later actions
	INT p;
	if (sm_level >= m_max_ply)
	{
		g_stats.OnFullSimulation();
		for (p=0; p<2; p++)
			_sim.SetAI(p, m_qai);
	}
	else
	{
		for (p=0; p<2; p++)
			_sim.SetAI(p, this);
	}
}

// DESC: this simulation is done.  Restore level so that it uses BMAI correctly for the next simulation
// drp063002 - there was a bug where level was not restored, which meant that level increased every simulation.  So
// a level 2 BMAI would only use a level 3 BMAI for the first simulation.  I.e. ply 3+ BMAI did not work.
void BMC_BMAI::OnPostSimulation(BMC_Game *_game, INT _enter_level)
{
#ifdef LEVEL_INCREMENT_RECURSIVE
#else
	//if (!_game->IsSimulation())
	sm_level = _enter_level + 1;
#endif
}

// DESC: based on m_max_branch and m_max_sims, determine a correct number of sims to not exceed m_max_branch
// PRE: must have called OnStartEvaluation so that sm_level is updated
INT BMC_BMAI::ComputeNumberSims(INT _moves)
{
	float decay_factor = (float)pow(s_ply_decay, sm_level-1);
	INT sims = (INT)(m_max_branch * decay_factor / (float)_moves);

	int adjusted_min_sims = (int)(m_min_sims * decay_factor + 0.99f);
	if (adjusted_min_sims<1)
		adjusted_min_sims = 1;

	if (sims < adjusted_min_sims)
		return adjusted_min_sims;

	int adjusted_max_sims = (int)(m_max_sims * decay_factor + 0.99f);
	if (adjusted_max_sims<0)
		adjusted_max_sims = 1;

	if (sims > adjusted_max_sims)
		return adjusted_max_sims;

	return sims;
}

// PRE: this is the phasing player
void BMC_BMAI::GetAttackAction(BMC_Game *_game, BMC_Move &_move)
{
	BMC_MoveList	movelist;
	_game->GenerateValidAttacks(movelist);

	INT enter_level;
	OnStartEvaluation(_game, enter_level);

	INT			sims = ComputeNumberSims(movelist.Size());
	g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d Valid Moves %d Sims %d\n", sm_level, _game->GetPhasePlayerID(), movelist.Size(), sims);

	INT i, s;
	BMC_Game	sim(true);
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

		g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d m%d: ", sm_level, _game->GetPhasePlayerID(), i);
		attack->Debug(BME_DEBUG_SIMULATION);

		score = 0;
		for (s=0; s<sims; s++)
		{
			//BMF_Log(BME_DEBUG_SIMULATION, "l%d m%d sim #%d: ", sm_level, i, s);	attack->Debug();
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

		g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d m%d: score %.1f - ", sm_level, _game->GetPhasePlayerID(), i, score);
		attack->Debug(BME_DEBUG_SIMULATION);

		//printf("l%d p%d m%d score %f\n", sm_level, _game->GetPhasePlayerID(), i, score);
		if (!best_move || score > best_score)
		{
			best_score = score;
			best_move = attack;
		}
	}

	g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d best move (%.1f points, %.1f%% win) ",
		sm_level,
		_game->GetPhasePlayerID(),
		best_score,
		best_score / sims * 100);
	best_move->m_game = _game;
	best_move->Debug(BME_DEBUG_SIMULATION);

	OnEndEvaluation(_game, enter_level);

	_move = *best_move;
}


void BMC_BMAI::GetSetSwingAction(BMC_Game *_game, BMC_Move &_move)
{
	// array of valid values
	struct SWING_ACTION {
		bool		swing;	// or option
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
			swing_action[actions].swing = true;
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
			swing_action[actions].swing = false;
			swing_action[actions].index = i;
			swing_action[actions].value = 0;	// initialize to first die
			actions++;
			combinations *= 2;
			_move.m_option_die.Set(i,0);
		}
	}

	if (!actions)
	{
		_move.m_action = BME_ACTION_PASS;
		return;
	}

	INT enter_level;
	OnStartEvaluation(_game, enter_level);

	INT			sims = ComputeNumberSims(combinations);

	if (enter_level<sm_debug_level)
		g_logger.Log(BME_DEBUG_SIMULATION, "swing action combinations %d sims %d\n", combinations, sims);

	// now iterate over all combinations of actions
	// TODO: stratify, at least in situations with a lot of moves
	INT max;
	BMC_Game	sim(true);
	BMC_Move 	best_move;
	float		best_score = -1, score;
	do
	{
		// only try case if valid (UNIQUE check)
		if (_game->ValidSetSwing(_move))
		{
			// try case
			score = 0;
			for (s=0; s<sims; s++)
			{
				//BMF_Log(BME_DEBUG_SIMULATION, "l%d m%d sim #%d: ", sm_level, i, s);	attack->Debug();
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

			if (enter_level<sm_debug_level)
			{
				g_logger.Log(BME_DEBUG_SIMULATION, "swing sims over - score %.1f - ", score);
				_move.Debug(BME_DEBUG_SIMULATION);
			}

			//printf("l%d p%d m%d score %f\n", sm_level, _game->GetPhasePlayerID(), i, score);
			if (score > best_score)
			{
				best_score = score;
				best_move = _move;
			}
		} // end if(ValidSetSwing)

		// increment step
		p = actions-1;
		while (1)
		{
			swing_action[p].value++;

			// update move
			if (swing_action[p].swing)
				_move.m_swing_value[swing_action[p].index]++;
			else
				_move.m_option_die.Set(swing_action[p].index,1);

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
				_move.m_option_die.Set(swing_action[p].index,0);
			}

			p--;

			// finished?
			if (p<0)
				break;
		} // end while(1) - increment step
	} while (p>=0);

	if (enter_level < sm_debug_level)
	{
		g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d best move swing (%.1f points, %.1f%% win) ",
			sm_level,
			_game->GetPhasePlayerID(),
			best_score,
			best_score / sims * 100);
		best_move.m_game = _game;
		best_move.Debug(BME_DEBUG_SIMULATION);
	}

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
	BMC_Game	sim(true);

	INT enter_level;
	OnStartEvaluation(_game, enter_level);

	INT combinations = 1;	// count "no reserve" move

	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!p->GetDie(i)->IsInReserve())
			continue;

		combinations++;
	}

	INT sims = ComputeNumberSims(combinations);

	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!p->GetDie(i)->IsInReserve())
			continue;

		// found an action
		_move.m_use_reserve = i;

		// evaluate action
		score = 0;
		for (s=0; s<sims; s++)
		{
			//BMF_Log(BME_DEBUG_SIMULATION, "l%d m%d sim #%d: ", sm_level, i, s);	attack->Debug();
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

		if (enter_level<sm_debug_level)
		{
			g_logger.Log(BME_DEBUG_SIMULATION, "reserve sims over - score %.1f - ", score);
			_move.Debug(BME_DEBUG_SIMULATION);
		}

		//printf("l%d p%d m%d score %f\n", sm_level, _game->GetPhasePlayerID(), i, score);
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
	for (s=0; s<sims; s++)
	{
		//BMF_Log(BME_DEBUG_SIMULATION, "l%d m%d sim #%d: ", sm_level, i, s);	attack->Debug();
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

	if (enter_level<sm_debug_level)
	{
		g_logger.Log(BME_DEBUG_SIMULATION, "reserve pass sims over - score %.1f - ", score);
		_move.Debug(BME_DEBUG_SIMULATION);
	}

	//printf("l%d p%d m%d score %f\n", sm_level, _game->GetPhasePlayerID(), i, score);
	if (score > best_score)
	{
		best_score = score;
		best_move = _move;
	}

	g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d best move reserve (%.1f points, %.1f%% win) ",
		sm_level,
		_game->GetPhasePlayerID(),
		best_score,
		best_score / sims * 100);
	best_move.m_game = _game;
	best_move.Debug(BME_DEBUG_SIMULATION);

	OnEndEvaluation(_game, enter_level);

	_move = best_move;
}

// PRE: we are m_phase_player (but don't have initiative)
void BMC_BMAI::GetUseFocusAction(BMC_Game *_game, BMC_Move &_move)
{
	BM_ASSERT(_game->GetPhase() == BME_PHASE_INITIATIVE_FOCUS);

	_move.m_game = _game;

	//BMC_Player *p = _game->GetPhasePlayer();

	BMC_MoveList	movelist;
	_game->GenerateValidFocus(movelist);

	BM_ASSERT(movelist.Size()>0);

	INT enter_level;
	OnStartEvaluation(_game, enter_level);

	INT			sims = ComputeNumberSims(movelist.Size());

	INT i, s;
	BMC_Game	sim(true);
	BMC_Move *	best_move = NULL;
	float		best_score = -1, score;

	for (i=0; i<movelist.Size(); i++)
	{
		BMC_Move * move = movelist.Get(i);

		BMF_Log(BME_DEBUG_SIMULATION, "l%d p%d focus: ", sm_level, _game->GetPhasePlayerID()); move->Debug();

		// evaluate action
		score = 0;
		for (s=0; s<sims; s++)
		{
			//BMF_Log(BME_DEBUG_SIMULATION, "l%d m%d sim #%d: ", sm_level, i, s);	attack->Debug();
			sim = *_game;

			OnPreSimulation(sim);
			sim.ApplyUseFocus(*move);
			BME_WLT rv = sim.PlayRound(NULL);
			OnPostSimulation(_game, enter_level);

			// reverse score if this is player 1, since WLT is wrt player 0
			if (rv==BME_WLT_TIE)
				score += 0.5f;
			else if ((rv==BME_WLT_WIN) ^ (_game->GetPhasePlayerID()!=0))
				score += 1.0f;
		}

		if (enter_level<sm_debug_level)
		{
			g_logger.Log(BME_DEBUG_SIMULATION, "focus sims over - score %.1f - ", score);
			move->Debug(BME_DEBUG_SIMULATION);
		}

		//printf("l%d p%d m%d score %f\n", sm_level, _game->GetPhasePlayerID(), i, score);
		if (score > best_score)
		{
			best_score = score;
			best_move = move;
		}
	}

	if (enter_level < sm_debug_level)
	{
	g_logger.Log(BME_DEBUG_SIMULATION, "p%d best move focus (%.1f points, %.1f%% win) ",
		_game->GetPhasePlayerID(),
		best_score,
		best_score / sims * 100);
	best_move->m_game = _game;
	best_move->Debug(BME_DEBUG_SIMULATION);
	}

	OnEndEvaluation(_game, enter_level);

	_move = *best_move;
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
	BMC_Game	sim(true);

	INT enter_level;
	OnStartEvaluation(_game, enter_level);

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

	INT sims = ComputeNumberSims(combinations);

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

		BMF_Log(BME_DEBUG_SIMULATION, "l%d chance mask %x: ", sm_level, i); _move.Debug();

		// evaluate action
		score = 0;
		for (s=0; s<sims; s++)
		{
			//BMF_Log(BME_DEBUG_SIMULATION, "l%d m%d sim #%d: ", sm_level, i, s);	attack->Debug();
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

		if (enter_level<sm_debug_level)
		{
			g_logger.Log(BME_DEBUG_SIMULATION, "chance sims over - score %.1f - ", score);
			_move.Debug(BME_DEBUG_SIMULATION);
		}

		//printf("l%d p%d m%d score %f\n", sm_level, _game->GetPhasePlayerID(), i, score);
		if (score > best_score)
		{
			best_score = score;
			best_move = _move;
		}
	}

	g_logger.Log(BME_DEBUG_SIMULATION, "p%d best move chance (%.1f points, %.1f%% win) ",
		_game->GetPhasePlayerID(),
		best_score,
		best_score / sims * 100);
	best_move.m_game = _game;
	best_move.Debug(BME_DEBUG_SIMULATION);

	OnEndEvaluation(_game, enter_level);

	_move = best_move;
}
