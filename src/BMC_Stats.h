///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Stats.h
// SPDX-FileCopyrightText: Copyright © 2021 Denis Papp
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: track performance stats
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ctime>
#include "bmai_lib.h"


class BMC_Stats
{
public:
	BMC_Stats();

	// methods
	void			DisplayStats();

	// events
	void			OnAppStarted() { m_start = time(NULL); }
	void			OnFullSimulation() { m_sims++; }

	// bmai-specific
	void			OnPlyAction(int _ply, int _moves, int _sims) { m_total_sims[_ply] += _sims; m_total_moves[_ply] += _moves; m_total_samples[_ply]++; }

private:
	time_t			m_start, m_end;
	int				m_sims;
	int				m_total_sims[BMD_MAX_PLY];
	int				m_total_moves[BMD_MAX_PLY];
	int				m_total_samples[BMD_MAX_PLY];

};

// global
extern BMC_Stats  g_stats;