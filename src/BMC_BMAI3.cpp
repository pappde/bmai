///////////////////////////////////////////////////////////////////////////////////////////
// BMC_BMAI3.cpp
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
//
// DESC:
//
// REVISION HISTORY:
// drp033102 - added BMAI2 which iteratively runs simulations across all moves and culls moves based on an
//			   interpolated score threshold (vs the best move).  This allows the ply to be increased to 3. Replaced 'g_ai'
// dbl100824 - migrated this logic from bmai_ai.cpp
///////////////////////////////////////////////////////////////////////////////////////////

#include "BMC_BMAI3.h"

#include "BMC_Logger.h"
#include "BMC_RNG.h"
#include "BMC_Stats.h"


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
	{
		g_logger.Log(BME_DEBUG_SIMULATION, "l%d p%d Valid Moves %d Sims %d\n", sm_level, _game->GetPhasePlayerID(),
			movelist.Size(),
			t.sims);
	}

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
	if (t.best_score==0 && _game->IsSurrenderAllowed())
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
