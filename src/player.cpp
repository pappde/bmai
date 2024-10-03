///////////////////////////////////////////////////////////////////////////////////////////
// bmai_player.cpp
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: BMAI code for managing the BMC_Player class
//
// REVISION HISTORY:
//
///////////////////////////////////////////////////////////////////////////////////////////

// includes
#include "bmai.h"
#include "player.h"
#include "logger.h"
#include <cassert>
#include <climits> // for INT_MAX

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Player methods
///////////////////////////////////////////////////////////////////////////////////////////

BMC_Player::BMC_Player() 
{
	Reset();
}

void BMC_Player::Reset()
{
	INT i;

	m_man = NULL;

	// set up swing dice
	for (i=0; i<BME_SWING_MAX; i++)
	{
		m_swing_value[i] = 0;	// 0 means undefined
		m_swing_dice[i] = 0;
	}

	for (i=0; i<BMD_MAX_DICE; i++)
	{
		m_die[i].Reset();
	}

	m_swing_set = SWING_SET_NOT;
	m_score = 0;
	m_available_dice = 0;
}

void BMC_Player::SetButtonMan(BMC_Man *_man)
{
	INT i, j;

	Reset();

	m_man = _man;

	// set up dice, and update m_swing_dice
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		m_die[i].SetDie(m_man->GetDieData(i));
		for (j=0; j<m_die[i].Dice(); j++)
			m_swing_dice[m_die[i].GetSwingType(j)]++;
	}
}

// DESC: also verifies only one TURBO, and it is TURBO OPTION
// POST: updates m_swing_dice and m_score
void BMC_Player::OnDiceParsed()
{
	INT i, j;

	// update m_swing_dice
	bool have_turbo = false;
	m_score = 0;
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!m_die[i].IsUsed())
			continue;

		for (j=0; j<m_die[i].Dice(); j++)
			m_swing_dice[m_die[i].GetSwingType(j)]++;

		m_score += m_die[i].GetScore(true);

		if (m_die[i].HasProperty(BME_PROPERTY_TURBO))
		{
			BM_ERROR(m_die[i].HasProperty(BME_PROPERTY_OPTION) || m_die[i].GetSwingType(0)!=BME_SWING_NOT);
			// TODO: reenable this error check BM_ERROR(!have_turbo);
			have_turbo = true;
		}
	}

	// optimize dice
	OptimizeDice();
}	

void BMC_Player::SetSwingDice(INT _swing, U8 _value, bool _from_turbo)
{
	BM_ASSERT(!m_swing_set || _from_turbo);	

	m_swing_value[_swing] = _value;

	// update all dice
	INT i;
	for (i=0; i<BMD_MAX_DICE; i++)
		m_die[i].OnSwingSet(_swing, _value);
}

void BMC_Player::SetOptionDie(INT _i, INT _d)
{
	BM_ASSERT(!m_swing_set);

	m_die[_i].SetOption(_d);
}

// POST: recomputes m_score
void BMC_Player::RollDice()
{
	//BM_ASSERT(m_man != NULL);

	m_score = 0;

	INT i;
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!m_die[i].IsUsed())
			continue;
		m_die[i].Roll();
		BM_ASSERT(m_die[i].GetValueTotal()>0);

		m_score += m_die[i].GetScore(true);
	}

	OptimizeDice();

	g_logger.Log(BME_DEBUG_ROUND, "rolling ");
	Debug(BME_DEBUG_ROUND);
}

void BMC_Player::Debug(BME_DEBUG _cat)
{
	if (!g_logger.IsLogging(_cat))
		return;

	INT i;

	printf("p%d s%.1f Dice ", m_id, m_score);
	for (i=0; i<GetAvailableDice(); i++)
		m_die[i].Debug();
	printf("\n");
}

void BMC_Player::DebugAllDice(BME_DEBUG _cat)
{
	INT i;

	printf("s %.1f Dice ", m_score);
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!m_die[i].IsUsed())
			continue;
		m_die[i].Debug();
	}
	printf("\n");
}

// POST: 
// - all READY dice are at the front, sorted largest to smallest.  These are followed by
//   all other dice that are not NOTUSED (not sorted by value)
// - m_available_dice is set
// - m_max_value and m_min_value are set 
void BMC_Player::OptimizeDice()
{
	m_available_dice = 0;
	m_max_value = 0;
	m_min_value = INT_MAX;

	// bubble-sort
	// TODO: do better
	INT i,j;
	BMC_Die	* die_i, *die_j;
	bool swap;
	BMC_Die temp_die;
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		die_i = GetDie(i);
		for (j=i+1; j<BMD_MAX_DICE; j++)
		{
			die_j = GetDie(j);
			// less-than check: 
			//    NOTUSED vs not NOTUSED
			// OR not READY vs READY
			// OR value less than value
			swap = false;
			if (!die_i->IsUsed() && die_j->IsUsed())
				swap = true;
			else if (die_j->IsAvailable())
			{
				if (!die_i->IsAvailable() || die_i->GetValueTotal()<die_j->GetValueTotal())
					swap = true;
			}

			if (swap)
			{
				temp_die = *die_i;
				*die_i = *die_j;
				*die_j = temp_die;
			}
		}
	}

	// compute available dice
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!GetDie(i)->IsAvailable())
			break;
		m_available_dice++;
		if (GetDie(i)->GetValueTotal() > m_max_value)
			m_max_value = GetDie(i)->GetValueTotal();
		else if (GetDie(i)->GetValueTotal() < m_min_value)
			m_min_value = GetDie(i)->GetValueTotal();
	}
}

void BMC_Player::OnDieSidesChanging(BMC_Die *_die)
{
	// KONSTANT: if die was set to not reroll, but we're about to change the number of sides, then force a reroll
	if (_die->GetState()!=BME_STATE_NOTSET)
	{
		BM_ERROR(_die->HasProperty(BME_PROPERTY_KONSTANT));
		_die->SetState(BME_STATE_NOTSET);
	}
	m_score -= _die->GetScore(true);
}

void BMC_Player::OnDieSidesChanged(BMC_Die *_die)
{
	m_score += _die->GetScore(true);

	//OptimizeDice();
}

// POST:
// - captured die moved after ready dice, invalidating pointers (*)
// - m_available_dice is updated
// - m_max_value and m_min_value are updated
// RETURNS:
// - pointer to the die in it's new position
BMC_Die * BMC_Player::OnDieLost(INT _d)
{
	BM_ASSERT(m_die[_d].IsAvailable());

	m_score -= m_die[_d].GetScore(true);

	// shuffle over dice instead of calling optimize
	INT i;
	if (_d<GetAvailableDice()-1)	// OPTIMIZATION: only bother if not at end
	{
		BMC_Die lost_die = m_die[_d];
		for (i=_d; i<GetAvailableDice()-1; i++)
			m_die[i] = m_die[i+1];
		m_die[i] = lost_die;
		_d = i;	// update _d so it points to the die that was removed
	}

	// update available dice
	m_available_dice--;

	// update m_min/max_value if appropriate

	if (m_die[_d].GetValueTotal() >= m_max_value) // OPTIMIZATION: only if was max
	{
		m_max_value = 0;
		for (i=0; i<GetAvailableDice(); i++)
		{
			if (m_die[i].GetValueTotal() > m_max_value)
				m_max_value = m_die[i].GetValueTotal();
		}
	}
	if (m_die[_d].GetValueTotal() <= m_min_value) // OPTIMIZATION: only if was min
	{
		m_min_value = INT_MAX;
		for (i = 0; i < GetAvailableDice(); i++)
		{
			if (m_die[i].GetValueTotal() < m_min_value)
				m_min_value = m_die[i].GetValueTotal();
		}
	}

	return &(m_die[_d]);
}

void BMC_Player::OnDieCaptured(BMC_Die *_die)
{
	m_score += _die->GetScore(false);
}

// POST: swing dice have been reset
void BMC_Player::OnRoundLost()
{
	INT i;

	for (i=0; i<BME_SWING_MAX; i++)
		m_swing_value[i] = 0;

	m_swing_set = SWING_SET_NOT;
}

// RETURNS: die index +1 (i.e. >0) if there is one present
INT BMC_Player::HasDieWithProperty(INT _p, bool _check_all_dice)
{
	INT i;
	INT max = _check_all_dice ? BMD_MAX_DICE : GetAvailableDice();

	for (i=0; i<max; i++)
	{
		if (m_die[i].HasProperty(_p))
			return i+1;
	}

	return 0;
}

// POST: m_score is very low
void BMC_Player::OnSurrendered()
{
	m_score = BMD_SURRENDERED_SCORE;
}

// NOTE: copies logic from BMC_Game::GenerateValidSetSwing()
bool BMC_Player::NeedsSetSwing()
{
	INT i;

	for (i=0; i<BME_SWING_MAX; i++)
	{
		if (GetTotalSwingDice(i)>0 && g_swing_sides_range[i][0]>0)
			return true;
	}

	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!GetDie(i)->IsUsed())
			continue;

		if (GetDie(i)->HasProperty(BME_PROPERTY_OPTION))
			return true;
	}

	return false;
}

// INVOKED: by BMC_Game when calling BMC_Die::OnUseReserve()
// CODEP: same as what OnDiceParsed() does - we are keeping m_swing_dice[] counts in sync with the "BMC_Die::IsUsed()" state. Would be more robust if was 
//  done through a pattern like "OnDieStateChanged()"
void BMC_Player::OnReserveDieUsed(BMC_Die *_die)
{
	for (INT j = 0; j < _die->Dice(); j++)
		m_swing_dice[_die->GetSwingType(j)]++;
}
