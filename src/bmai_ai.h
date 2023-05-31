///////////////////////////////////////////////////////////////////////////////////////////
// bmai_ai.h
// Copyright (c) 2001-2023 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/hamstercrack/bmai
// 
// DESC: main header for BMAI AI classes
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#include "game.h"

#ifndef __BMAI_AI_H__
#define __BMAI_AI_H__

// basic AI class
class BMC_AI
{
public:

	virtual void	GetSetSwingAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetAttackAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetReserveAction(BMC_Game *_game, BMC_Move &_move);		
	virtual void	GetUseChanceAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetUseFocusAction(BMC_Game *_game, BMC_Move &_move);

	void		GetUseAuxiliaryAction(BMC_Game *_game, BMC_Move &_move);
	void		GetSetTurboAction(BMC_Game *_game, BMC_Move &_move);

	F32			ScoreAttack(BMC_Game *_game, BMC_Move &_move);

	// class testing
	virtual		bool	IsBMAI() { return false; }
	virtual		bool	IsBMAI3() { return true; }

protected:


private:
	BMC_AI *	m_qai;
};

// The real BMAI
class BMC_BMAI : public BMC_AI
{
public:
				BMC_BMAI();

	virtual void	GetAttackAction(BMC_Game *_game, BMC_Move &_move);
	virtual	void	GetSetSwingAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetReserveAction(BMC_Game *_game, BMC_Move &_move);		
	virtual void	GetUseChanceAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetUseFocusAction(BMC_Game *_game, BMC_Move &_move);

	virtual		bool	IsBMAI() { return true; }

	//////////////////////////////////////////
	// mutators
	//////////////////////////////////////////

	void		SetQAI(BMC_AI *_ai) { m_qai = _ai; }		// use this to change the QAI used in simulations
	void		SetMaxPly(INT _m)	{ m_max_ply = _m; }
	void		SetMaxBranch(INT _m) { m_max_branch = _m; }
	void		SetMaxSims(INT _s)	{m_max_sims = _s; }
	void		SetMinSims(INT _s)	{m_min_sims = _s; }
	void		CopySettings(BMC_BMAI & _ai) { m_max_ply = _ai.GetMaxPly(); m_max_branch = _ai.GetMaxBranch(); }

	// static mutators
	static void	SetDebugLevel(int _level) { sm_debug_level = _level; }

	//////////////////////////////////////////
	// accessors
	//////////////////////////////////////////

	INT			GetMaxPly() { return m_max_ply; }
	INT			GetMaxBranch() { return m_max_branch; }
	INT			GetMaxSims() { return m_max_sims; }
	INT			GetMinSims() { return m_min_sims; }
	INT			GetLevel() { return sm_level; }

protected:

	// determining number of sims to use
	INT			ComputeNumberSims(INT _moves);

	// for dealing with 'level'
	void		OnStartEvaluation(BMC_Game *_game, INT &_enter_level);
	void		OnPreSimulation(BMC_Game &_sim);
	void		OnPostSimulation(BMC_Game *_game, INT _enter_level);
	void		OnEndEvaluation(BMC_Game *_game, INT _enter_level);

	BMC_AI *	m_qai;
	INT			m_max_ply;
	INT			m_max_branch;
	INT			m_min_sims, m_max_sims;

	// static data to prevent too many BMAI calls in simulation depth
	static int	sm_level;
	static int	sm_debug_level;

};

// BMAI v2 for testing strategies
class BMC_BMAI3 : public BMC_BMAI
{
public:
					BMC_BMAI3();
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

// Heuristic Quick AI that BMAI uses by default
class BMC_QAI : public BMC_AI
{
public:
	virtual void		GetAttackAction(BMC_Game *_game, BMC_Move &_move);


protected:
private:
};

// Selects the maximizing move
class BMC_AI_Maximize : public BMC_AI
{
public:
	virtual void		GetAttackAction(BMC_Game *_game, BMC_Move &_move);
};

// Randomly selects between a basic AI and a maximizing AI
// P(p) use Maximize (mode 1), else Basic (mode 0)
class BMC_AI_MaximizeOrRandom : public BMC_AI
{
public:
	virtual void		GetAttackAction(BMC_Game *_game, BMC_Move &_move);

	void	SetP(F32 _p) { p = _p; }

private:
	F32	p;
};


class BMC_AI_Mode2 : public BMC_AI
{
};


#endif // #ifndef __BMAI_AI_H__