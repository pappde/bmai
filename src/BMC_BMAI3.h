///////////////////////////////////////////////////////////////////////////////////////////
// BMC_BMAI3.h
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
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

#include "bmai_lib.h"
#include "BMC_BMAI.h"


// BMAI v2 for testing strategies
class BMC_BMAI3 : public BMC_BMAI
{
public:
	BMC_BMAI3(BMC_AI * _ai);
	virtual void	GetAttackAction(BMC_Game *_game, BMC_Move &_move);
	virtual	void	GetSetSwingAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetUseFocusAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetUseChanceAction(BMC_Game *_game, BMC_Move &_move);

	// accessors
	float	GetLastProbabilityWin() { return m_last_probability_win; }

	// class testing
	virtual bool	IsBMAI3() { return true; }

protected:
	// getaction state
	class BMC_ThinkState {
	public:
		BMC_ThinkState(BMC_BMAI3 *_ai, BMC_Game *_game, BMC_MoveList &_movelist);
		float			PercSimsRun() { return (float)sims_run / sims; }
		void			SetBestMove(BMC_Move *_move, float _score) { best_move = _move; best_score = _score; }

		int				sims;
		int				sims_run;
		BMC_FloatVector	score;
		float			best_score;
		BMC_Move *		best_move;
		BMC_MoveList &	movelist;
		BMC_Game *		game;
	};
	friend class BMC_ThinkState;

	bool			CullMoves(BMC_ThinkState &_t);
	void			RandomlySelectMoves(BMC_MoveList &_movelist, int _max);

	int				m_sims_per_check;
	float			m_min_best_score_threshold;
	float			m_max_best_score_threshold;
	float			m_last_probability_win;
};
