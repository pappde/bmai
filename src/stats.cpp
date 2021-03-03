#include "bmai.h"
#include "stats.h"

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Stats
///////////////////////////////////////////////////////////////////////////////////////////

BMC_Stats::BMC_Stats()
{
	m_start = m_end = 0;
	m_sims = 0;
	for (int i = 0; i < BMD_MAX_PLY; i++)
		m_total_sims[i] = m_total_moves[i] = m_total_samples[i] = 0;
}

void BMC_Stats::DisplayStats()
{
	double diff = difftime(time(NULL), m_start);
	printf("Time: %lf s ", diff);
	printf("Sim: %d  Sims/Sec: %f  Mvs/Sms ", m_sims, diff > 0 ? m_sims / diff : 0);
	float leaves = 1;
	for (int i = 1; i < BMD_MAX_PLY; i++)
	{
		if (m_total_samples[i] == 0)
			break;
		printf("| ");
		float avg_moves = (float)m_total_moves[i] / m_total_samples[i];
		float avg_sims = (float)m_total_sims[i] / m_total_samples[i];
		printf("%.1f/%.1f ", avg_moves, avg_sims);
		leaves *= avg_moves * avg_sims;
	}
	printf("= %.0f\n", leaves);
}