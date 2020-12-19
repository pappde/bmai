///////////////////////////////////////////////////////////////////////////////////////////
// bmai_ai.cpp
// Copyright (c) 2001-2020 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/hamstercrack/bmai
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
// drp033102 - added BMAI2 which iteratively runs simulations accross all moves and culls moves based on an 
//			   interpolated score threshold (vs the best move).  This allows the ply to be increased to 3. Replaced 'g_ai'
// drp040102 - smarter culling (such as ignoring cases with 0% moves but delta_points was close)
//			 - also culling out moves where percentage is close but there aren't enough sims left to catch up to best_move
//			 - [script] increasing to ply 4
//			 - moved g_sims to m_sims
// drp062702 - more aggressive culling of TRIP, and more aggressive culling of 0-point moves
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
#include "bmai_ai.h"
#include <cmath>

#define BMD_DEFAULT_SIMS	500
#define BMD_MIN_SIMS		10
#define BMD_QAI_FUZZINESS	5
#define BMD_MAX_PLY_PREROUND	2

// globals
BMC_QAI	g_qai;
extern BMC_RNG g_rng;

// HACK: sm_level should be automatically updated in all eval methods, but it is currently only updated
// by proper use of OnStartEvaluation() and OnEndEvaluation() which BMAI and BMAI3 are trusted to call.
INT BMC_BMAI::sm_level = 0;	

// only output certain debug strings when 'sm_level' is <= to this. Trusts AI classes to use before calling Log().
INT BMC_BMAI::sm_debug_level = 2;		// default so only up to level 2 is output

float s_ply_decay = 0.5f;

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
		bool extra_turn = false;
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

	best_move->m_game = _game;
	g_logger.Log(BME_DEBUG_QAI, "QAI p%d best move (%.1f) ", 	_game->GetPhasePlayerID(), 	best_score);	best_move->Debug(BME_DEBUG_QAI);

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

	// reset all swing/option settings so can use ValidSetSwing() 
	for (i=0; i<BME_SWING_MAX; i++)
		_move.m_swing_value[i] = 0;

	for (i=0; i<BMD_MAX_DICE; i++)
		_move.m_option_die.Set(i, 0);

	// check swing dice
	// PRE: since using ValidSetSwing() all swing/option dice need to have valid settings
	for (i=0; i<BME_SWING_MAX; i++)
	{
		if (p->GetTotalSwingDice(i)>0 && g_swing_sides_range[i][0]>0)
		{
			_move.m_swing_value[i] = g_swing_sides_range[i][0];	// set first value
			actions++;
		}
	}

	// if necessary cycle through all options - necessary for UNIQUE
	if (!_game->ValidSetSwing(_move))
	{
		for (i=0; i<BME_SWING_MAX; i++)
		{
			if (p->GetTotalSwingDice(i)>0 && g_swing_sides_range[i][0]>0)
			{
				int v = g_swing_sides_range[i][0];
				do 
				{
					BM_ASSERT(v<=g_swing_sides_range[i][1]);
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
#endif;
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
	INT sims = m_max_branch * decay_factor / _moves;

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

BMC_BMAI3::BMC_BMAI3()
{
	// SETTINGS
	m_sims_per_check = m_min_sims;
	m_min_best_score_threshold = 0.25f;
	m_max_best_score_threshold = 0.90f;

	//m_min_best_score_points_threshold = 
	//m_max_best_score_points_threshold = 
}

// PRE: we are m_phase_player 
void BMC_BMAI3::GetUseChanceAction(BMC_Game *_game, BMC_Move &_move)
{
	BM_ASSERT(_game->GetPhase() == BME_PHASE_INITIATIVE_CHANCE);

	m_last_probability_win = 1000;

	_move.m_game = _game;

	// build movelist
	BMC_MoveList	movelist;
	_game->GenerateValidChance(movelist);

	BM_ASSERT(movelist.Size()>0);
	/* always play out action for accurate probability measurement
	if (movelist.Size()==1)
	{
		_move = *movelist.Get(0);
		return;
	}
	*/

	INT enter_level;
	OnStartEvaluation(_game, enter_level);

	INT i, s;
	BMC_Game	sim(true);
	BMC_ThinkState	t(this,_game,movelist);

	while (t.sims_run < t.sims)
	{
		int check_sims = std::min(m_sims_per_check, (t.sims-t.sims_run));

		for (i=0; i<movelist.Size(); i++)
		{
			BMC_Move * move = movelist.Get(i);

			BMF_Log(BME_DEBUG_SIMULATION, "l%d p%d chance: ", sm_level, _game->GetPhasePlayerID()); move->Debug();

			// evaluate action
			for (s=0; s<check_sims; s++)
			{
				sim = *_game;
				OnPreSimulation(sim);
				sim.ApplyUseChance(*move);

				// at max_ply, play the game out and score it as "win/tie/loss" (1/0.5/0)
				if (sm_level >= m_max_ply)
				{
					BME_WLT rv = sim.PlayRound(NULL);

					// reverse score if this is player 1, since WLT is wrt player 0
					if (rv==BME_WLT_TIE)
						t.score[i] += 0.5f;
					else if ((rv==BME_WLT_WIN) ^ (_game->GetPhasePlayerID()!=0))
						t.score[i] += 1.0f;
				}

				// before max_ply, the next "GetAction" will be BMAI3.  Use "PlayFight_EvaluateMove" to simply play to that
				// move and then use its estimate of winning chances as a more accurate score.
				else
					t.score[i] += sim.PlayRound_EvaluateMove(_game->GetPhasePlayerID());

				OnPostSimulation(_game, enter_level);
			}

			move->m_game = _game;
			
			if (enter_level<sm_debug_level)
			{
				g_logger.Log(BME_DEBUG_SIMULATION, "chance sims over - score %.1f - ", t.score[i]);	
				move->Debug(BME_DEBUG_SIMULATION);
			}

			if (t.score[i] > t.best_score)
				t.SetBestMove(move, t.score[i]);

		} // end for each move

		t.sims_run += check_sims;

		if (t.sims_run >= t.sims)
			break;

		if (!CullMoves(t))
			break;

	} // end while t.sims_run
		
	g_logger.Log(BME_DEBUG_SIMULATION, "p%d best move chance (%.1f points, %.1f%% win) ", 
		_game->GetPhasePlayerID(), 
		t.best_score,
		t.best_score / t.sims_run * 100);
	t.best_move->m_game = _game;
	t.best_move->Debug(BME_DEBUG_SIMULATION);

	OnEndEvaluation(_game, enter_level);

	_move = *t.best_move;
	m_last_probability_win = t.best_score / t.sims_run;
}


// PRE: we are m_phase_player (but don't have initiative)
void BMC_BMAI3::GetUseFocusAction(BMC_Game *_game, BMC_Move &_move)
{
	BM_ASSERT(_game->GetPhase() == BME_PHASE_INITIATIVE_FOCUS);

	m_last_probability_win = 1000;

	_move.m_game = _game;

	BMC_MoveList	movelist;
	_game->GenerateValidFocus(movelist);

	BM_ASSERT(movelist.Size()>0);
		/* always play out action for accurate probability measurement
	if (movelist.Size()==1)
	{
		_move = *movelist.Get(0);
		return;
	}
	*/

	INT enter_level;
	OnStartEvaluation(_game, enter_level);

	INT i, s;
	INT pass = 0;
	BMC_Game	sim(true);
	BMC_ThinkState	t(this,_game,movelist);

	while (t.sims_run < t.sims)
	{
		int check_sims = std::min(m_sims_per_check, (t.sims-t.sims_run));

		for (i=0; i<movelist.Size(); i++)
		{
			BMC_Move * move = movelist.Get(i);

			if (enter_level<sm_debug_level)
			{
			BMF_Log(BME_DEBUG_SIMULATION, "%sl%d p%d focus: ", 
				pass>0 ? "+ " : "",
				sm_level, _game->GetPhasePlayerID()); move->Debug();
			}

			// evaluate action
			for (s=0; s<check_sims; s++)
			{
				sim = *_game;
				OnPreSimulation(sim);
				sim.ApplyUseFocus(*move);

				// at max_ply, play the game out and score it as "win/tie/loss" (1/0.5/0)
				if (sm_level >= m_max_ply)
				{
					BME_WLT rv = sim.PlayRound(NULL);

					// reverse score if this is player 1, since WLT is wrt player 0
					if (rv==BME_WLT_TIE)
						t.score[i] += 0.5f;
					else if ((rv==BME_WLT_WIN) ^ (_game->GetPhasePlayerID()!=0))
						t.score[i] += 1.0f;
				}

				// before max_ply, the next "GetAction" will be BMAI3.  Use "PlayFight_EvaluateMove" to simply play to that
				// move and then use its estimate of winning chances as a more accurate score.
				else
					t.score[i] += sim.PlayRound_EvaluateMove(_game->GetPhasePlayerID());

				OnPostSimulation(_game, enter_level);
			}

			move->m_game = _game;

			if (enter_level<sm_debug_level)
			{
				g_logger.Log(BME_DEBUG_SIMULATION, "%sfocus sims over - score %.1f - ", 
					pass>0 ? "+ " : "",
					t.score[i]);	
				move->Debug(BME_DEBUG_SIMULATION);
			}

			if (t.score[i] > t.best_score)
				t.SetBestMove(move, t.score[i]);

		} // end for each move

		t.sims_run += check_sims;

		if (t.sims_run >= t.sims)
			break;

		if (!CullMoves(t))
			break;

		pass++;

	} // end while t.sims_run

	if (enter_level < sm_debug_level)
	{
	g_logger.Log(BME_DEBUG_SIMULATION, "p%d best move focus (%.1f points, %.1f%% win) ", 
		_game->GetPhasePlayerID(), 
		t.best_score,
		t.best_score / t.sims_run * 100);
	t.best_move->m_game = _game;
	t.best_move->Debug(BME_DEBUG_SIMULATION);
	}

	OnEndEvaluation(_game, enter_level);

	_move = *t.best_move;
	m_last_probability_win = t.best_score / t.sims_run;
}

// POST: movelist randomly cut down to the given _max. 
// NOTE: it retains setswing moves that use the extreme values
void BMC_BMAI3::RandomlySelectMoves(BMC_MoveList &_list, int _max)
{
	int m, i;
	int swing_dice = 0;
	int extreme_moves = 0;	// number of moves that have all swing dice set to extreme values
	for (m=0; m<_list.Size(); m++)
	{
		BMC_Move *move = _list.Get(m);
		if (move->m_action != BME_ACTION_SET_SWING_AND_OPTION)
			continue;

		// compute the number of swing dice set to the extreme values
		BMC_Player *pl = move->m_game->GetPhasePlayer();
		move->m_extreme_settings = 0;
		swing_dice = 0;	// TODO: don't recompute every time
		for (i=0; i<BME_SWING_MAX; i++)
		{
			if (pl->GetTotalSwingDice(i)>0 && g_swing_sides_range[i][0]>0)
			{
				swing_dice++;
				if (move->m_swing_value[i]==g_swing_sides_range[i][0]
					|| move->m_swing_value[i]==g_swing_sides_range[i][1])
				{
					move->m_extreme_settings++;
				}
			}
		}

		if (move->m_extreme_settings==swing_dice)
			extreme_moves++;
	}

	// PRE: m_extreme_settings computed for all moves

	// if there are too many moves with all dice set to extreme values, then just cut out all
	// but those
	if (extreme_moves>=_max)
	{
		for (m=0; m<_list.Size(); )
		{
			BMC_Move *move = _list.Get(m);
			if (move->m_extreme_settings == swing_dice)
			{
				m++;
				continue;
			}
			_list.Remove(m);
		}

		return;
	}

	// now randomly cut out moves until we are at max.  However, the
	// probability of a move being cut is determined by the percentage
	// of swing dice set to extreme
	while (_list.Size() > _max)
	{
		int m = g_rng.GetRand(_list.Size());
		BMC_Move *move = _list.Get(m);
		float percentage_extreme = (float)move->m_extreme_settings / swing_dice;
		float p = g_rng.GetFRand();
		if (p>=percentage_extreme)
		{
			_list.Remove(m);
		}
	}
}

// TODO: stratify, at least in situations with a lot of moves
void BMC_BMAI3::GetSetSwingAction(BMC_Game *_game, BMC_Move &_move)
{
	m_last_probability_win = 1000; //_game->ConvertWLTToWinProbability();

	BMC_MoveList	movelist;
	_game->GenerateValidSetSwing(movelist);

	BM_ASSERT(movelist.Size()>0);

	INT enter_level;
	OnStartEvaluation(_game, enter_level);

	INT i,s;
	BMC_Game	sim(true);

	// drp022203 - if there are simply far too many moves then randomly cut out moves
	//  (not the extreme values). [Gordo has over 570k setswing moves, 160 days on ply 4]
	BMC_Player *pl = _game->GetPhasePlayer();
	INT m_max_moves = m_max_branch / m_min_sims;
	if (movelist.Size() > m_max_moves)
	{
		g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d Valid SetSwing %d Max %d\n", sm_level, _game->GetPhasePlayerID(), 
			movelist.Size(), 
			m_max_moves);

		RandomlySelectMoves(movelist, m_max_moves);		
	}

	BMC_ThinkState	t(this,_game,movelist);

	g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d Valid SetSwing %d Sims %d\n", sm_level, _game->GetPhasePlayerID(), 
		movelist.Size(), 
		t.sims);

	while (t.sims_run < t.sims)
	{
		int check_sims = std::min(m_sims_per_check, (t.sims-t.sims_run));

		for (i=0; i<movelist.Size(); i++)
		{
			BMC_Move * move = movelist.Get(i);
			//BM_ASSERT(move->m_action == BME_ACTION_SET_SWING_AND_OPTION);

			if (enter_level<sm_debug_level)
			{
				g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d - ", 
					sm_level,
					_game->GetPhasePlayerID() );	
				move->Debug(BME_DEBUG_SIMULATION);
			}

			// try case
			for (s=0; s<check_sims; s++)
			{
				sim = *_game;
				OnPreSimulation(sim);

				/*
				// for max ply 3:
					// at ply 1, set swing status to "READY" which means that, if our opponent hasn't set swing, then the 
					// opponent's GetSetSwingAction() will be done without the knowledge of what our swing action is.  This
					// means the opponent will recursively call GetSetSwingAction() for us.  This is more realistic, since 
					// our move will then be the "counter-move" to the "safest opening move," instead of being the 
					// "safest opening move."  In Hope-Hope, if you know the other player's swing you have a 65% chance of winning.
					// Of course, then someone could play one level ahead of BMAI and counter the counter to the safest...

					// TODO: throw randomness in the mix
					// NOTE:	Hope vs Hope	ply 2, without this, says	Y2-5, 35%
					//							ply 2, with this			Y4, 40%
					//							ply 3, without this
					//							ply 3, with this			Y6, 75%

				// we don't do this for max ply 2, since it would make the opponent assume we are using QAI to pick our swing
				*/
#ifdef _DEBUG
				sim.ApplySetSwing(*move, sm_level>1); 
#else
				sim.ApplySetSwing(*move);
#endif

				// at max_ply, play the game out and score it as "win/tie/loss" (1/0.5/0)
				if (sm_level >= m_max_ply)
				{
					BME_WLT rv = sim.PlayRound(NULL);

					// reverse score if this is player 1, since WLT is wrt player 0
					if (rv==BME_WLT_TIE)
						t.score[i] += 0.5f;
					else if ((rv==BME_WLT_WIN) ^ (_game->GetPhasePlayerID()!=0))
						t.score[i] += 1.0f;
				}

				// before max_ply, the next "GetAction" will be BMAI3.  Use "PlayFight_EvaluateMove" to simply play to that
				// move and then use its estimate of winning chances as a more accurate score.
				else
					t.score[i] += sim.PlayRound_EvaluateMove(_game->GetPhasePlayerID());

				OnPostSimulation(_game, enter_level);
			}

			move->m_game = _game;

			if (enter_level<sm_debug_level)
			{
				g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d swing sims %d - score %.1f - ", 
					sm_level,
					_game->GetPhasePlayerID(),		
					t.sims_run + check_sims,
					t.score[i]);	
				move->Debug(BME_DEBUG_SIMULATION);
				g_stats.DisplayStats();
			}

			//printf("l%d p%d m%d score %f\n", sm_level, _game->GetPhasePlayerID(), i, score);
			if (t.score[i] > t.best_score)
				t.SetBestMove(move, t.score[i]);

		}

		t.sims_run += check_sims;

		if (t.sims_run >= t.sims)
			break;

		if (!CullMoves(t))
			break;
	}

	if (enter_level < sm_debug_level)
	{
		g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d best move swing (%.1f points, %.1f%% win) ", 
			sm_level,
			_game->GetPhasePlayerID(), 
			t.best_score,
			t.best_score / t.sims_run * 100);
		t.best_move->m_game = _game;
		t.best_move->Debug(BME_DEBUG_SIMULATION);
	}

	OnEndEvaluation(_game, enter_level);

	_move = *t.best_move;

	m_last_probability_win = t.best_score / t.sims_run;
}


// DESC: instead of running all simulations for each move one at a time, run a limited number of simulations for
// all moves and cull out
// PRE: this is the phasing player
void BMC_BMAI3::GetAttackAction(BMC_Game *_game, BMC_Move &_move)
{
	m_last_probability_win = 1000; 

	BMC_MoveList	movelist;
	_game->GenerateValidAttacks(movelist);

	INT enter_level;
	OnStartEvaluation(_game, enter_level);

	INT i, s;
	BMC_Game	sim(true);
	BMC_ThinkState	t(this,_game,movelist);

	if (enter_level < sm_debug_level)
	g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d Valid Moves %d Sims %d\n", sm_level, _game->GetPhasePlayerID(), 
		movelist.Size(), 
		t.sims);

	while (t.sims_run < t.sims)
	{
		int check_sims = std::min(m_sims_per_check, (t.sims-t.sims_run));

		for (i=0; i<movelist.Size(); i++)
		{
			BMC_MoveAttack * attack = movelist.Get(i);
			/*
			if (attack->m_action != BME_ACTION_ATTACK)
			{
				float score = _game->ConvertWLTToWinProbability();
				t.SetBestMove(attack, t.score[i] + score * check_sims);	
				break;
			}
			*/

			for (s=0; s<check_sims; s++)
			{
				sim = *_game;
				OnPreSimulation(sim);

				// at max_ply, play the game out and score it as "win/tie/loss" (1/0.5/0)
				if (sm_level >= m_max_ply)
				{
					BME_WLT rv = sim.PlayRound(attack);

					// reverse score if this is player 1, since WLT is wrt player 0
					if (rv==BME_WLT_TIE)
						t.score[i] += 0.5f;
					else if ((rv==BME_WLT_WIN) ^ (_game->GetPhasePlayerID()!=0))
						t.score[i] += 1.0f;
				}

				// before max_ply, the next "GetAction" will be BMAI3.  Use "PlayFight_EvaluateMove" to simply play to that
				// move and then use its estimate of winning chances as a more accurate score.
				else
					t.score[i] += sim.PlayFight_EvaluateMove(_game->GetPhasePlayerID(),*attack);

				OnPostSimulation(_game, enter_level);
			}

			attack->m_game = _game;

			if (sm_level<=sm_debug_level) 
			{
				g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d m%d sims %d score %f ", sm_level, _game->GetPhasePlayerID(), i, check_sims, t.score[i]);
				attack->Debug(BME_DEBUG_SIMULATION);
				if (sm_level <=1 )
					g_stats.DisplayStats();
			}

			if (t.score[i] > t.best_score)
				t.SetBestMove(attack, t.score[i]);
			
		}
		t.sims_run += check_sims;

		if (t.sims_run >= t.sims)
			break;

		if (!CullMoves(t))
			break;
	}

	// SURRENDER: if best move is 0% win, then surrender
	if (t.best_score==0)
		t.best_move->m_action = BME_ACTION_SURRENDER;

	if (enter_level < sm_debug_level)
	{
	g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d best move (%.1f points, %.1f%% win) ", 
		sm_level,
		_game->GetPhasePlayerID(), 
		t.best_score,
		t.best_score / t.sims_run * 100);
	t.best_move->m_game = _game;
	t.best_move->Debug(BME_DEBUG_SIMULATION);
	}

	OnEndEvaluation(_game, enter_level);

	_move = *t.best_move;

	m_last_probability_win = t.best_score / t.sims_run;
}

// RETURN: true to continue running simulations, false if there is no point in continuing simulations (one move left)
bool BMC_BMAI3::CullMoves(BMC_ThinkState &t)
{
	if (t.movelist.Size()==1)
		return false;

	//// compare and cull any moves that are not doing well

	// cull threshold (percentage below best score)
	// at 0% of sims, must be 25% of best_score to be cut
	// at 100% of sims, must be 95% of best_score to be cut
	float best_score_threshold_delta = m_max_best_score_threshold - m_min_best_score_threshold;
	float best_score_threshold = m_min_best_score_threshold + t.PercSimsRun() * best_score_threshold_delta;

	// delta points threshold - half of 'sims per check' at start, 0 at end
	float delta_points_threshold = ( 1.0f - t.PercSimsRun() ) * m_sims_per_check * 0.5f;

	// [adhoc] clamp the points threshold at best_score, so we cull 0-point moves, unless the best move has only 0/1 points
	if (t.best_score>1 && delta_points_threshold>=t.best_score)
		delta_points_threshold = t.best_score;

	if (sm_level<=sm_debug_level)
	{
	g_logger.Log(BME_DEBUG_BMAI, "l%d p%d cullcheck mvs %d sims %d/%d thresh %f\n", 
		sm_level,
		t.game->GetPhasePlayerID(),
		t.movelist.Size(), 
		t.sims_run,t.sims,
		best_score_threshold);
	}

	// cull moves that are one stddev below AND less than half of the best move (since we probably don't want to
	// cut any if stddev is low)
	int i;
	for (i=0; i<t.movelist.Size(); i++)
	{
		//// reasons to cull
		int cull = 0;
		float delta_points = t.best_score - t.score[i];
		float min_delta_points = delta_points_threshold;

		// [adhoc] TRIP: these tend to really fill the movelist, and heuristically tend to be of lesser value, so reduce the 'dpt' for them
		BMC_MoveAttack * attack = t.movelist.Get(i);
		if (attack->m_action == BME_ACTION_ATTACK && attack->m_attack == BME_ATTACK_TRIP)
			min_delta_points *= 0.5f;

		// 1) not enough sims left to catchup to best_move
		if (delta_points>=(t.sims-t.sims_run))
			cull = 1;

		// 2) not at threshold score (relative to best_score - increasing threshold)
		//    AND delta_points big enough (or sims run big enough)
		else if (t.score[i]<t.best_score*best_score_threshold && delta_points>=min_delta_points)
			cull = 2;

		if (cull)
		{
			if (sm_level<=sm_debug_level)
			{
			g_logger.Log(BME_DEBUG_BMAI, "l%d p%d CULL%d m%d sims %d perc %.1f score %.1f best %.1f - ",
				sm_level,
				t.game->GetPhasePlayerID(), 
				cull,
				i, 
				t.sims_run, 
				t.score[i] / t.sims_run * 100,
				t.score[i],
				t.best_score);
			t.movelist.Get(i)->Debug(BME_DEBUG_BMAI);
			}

			// cull move - this swaps in last move
			t.score[i] = t.score[t.movelist.Size()-1];
			t.movelist.Remove(i);
			i--;
		}
	}

	if (t.movelist.Size()==1)
		return false;

	return true;
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

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_ThinkState
///////////////////////////////////////////////////////////////////////////////////////////

BMC_BMAI3::BMC_ThinkState::BMC_ThinkState(BMC_BMAI3 *_ai, BMC_Game *_game, BMC_MoveList &_movelist) : 
	score(_movelist.Size()), sims_run(0), best_move(NULL), movelist(_movelist),
	game(_game)
{
	best_score = -1;
	for (int i=0; i<_movelist.Size(); i++)
		score[i] = 0;
	sims = _ai->ComputeNumberSims(_movelist.Size());
	g_stats.OnPlyAction(_ai->GetLevel(), _movelist.Size(), sims);
}

