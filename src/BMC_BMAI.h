///////////////////////////////////////////////////////////////////////////////////////////
// BMC_BMAI.h
//
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2021 Denis Papp <denis@accessdenied.net>
//
// DESC:
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
// dbl100824 - migrated this logic from bmai_ai.h
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "BMC_AI.h"


// The real BMAI
class BMC_BMAI : public BMC_AI
{
public:
	BMC_BMAI(BMC_AI * _ai);

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
