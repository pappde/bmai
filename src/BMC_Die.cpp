///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Die.cpp
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
// dbl100524 - broke this logic out into its own class file
///////////////////////////////////////////////////////////////////////////////////////////

#include "BMC_Die.h"

#include <cstdio>
#include "BMC_Game.h"
#include "BMC_Logger.h"
#include "BMC_Player.h"
#include "BMC_RNG.h"

//X?: Roll a d6. 1: d4; 2: d6; 3: d8; 4: d10; 5: d12; 6: d20.
//V?: Roll a d4. 1: d6; 2: d8; 3: d10; 4: d12.
INT c_mood_sides_X[BMD_MOOD_SIDES_RANGE_X] = { 4, 6, 8,  10, 12, 20 };
INT c_mood_sides_V[BMD_MOOD_SIDES_RANGE_V] = { 6, 8, 10, 12 };

// MIGHTY dice - index by old number of sides
INT c_mighty_sides[20] = { 1, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 16, 16, 16, 16, 20, 20, 20, 20 };
// WEAK dice - index by old number of sides
INT c_weak_sides[20] = { 1, 1, 1, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 12, 12, 16, 16, 16 };

void BMC_Die::Reset()
{
	BMC_DieData::Reset();

	m_state = BME_STATE_NOTUSED;
	m_value_total = 0;
	m_sides_max = 0;
}

void BMC_Die::RecomputeAttacks()
{
	BM_ASSERT(m_state!=BME_STATE_NOTSET);

	// by default vulnerable to all
	m_vulnerabilities.SetAll();

	m_attacks.Clear();

	// POWER and SKILL - by default for all dice
	m_attacks.Set(BME_ATTACK_POWER);
	m_attacks.Set(BME_ATTACK_SKILL);

	// Handle specific dice types

	// UNSKILLED [The Flying Squirrel]
	// TODO: does WARRIOR override this? (currently)
	if (HasProperty(BME_PROPERTY_UNSKILLED))
		m_attacks.Clear(BME_ATTACK_SKILL);

	// SPEED
	if (HasProperty(BME_PROPERTY_SPEED))
		m_attacks.Set(BME_ATTACK_SPEED);

	// TRIP
	if (HasProperty(BME_PROPERTY_TRIP))
		m_attacks.Set(BME_ATTACK_TRIP);

	// SHADOW
	if (HasProperty(BME_PROPERTY_SHADOW))
	{
		m_attacks.Set(BME_ATTACK_SHADOW);
		m_attacks.Clear(BME_ATTACK_POWER);
	}

	// KONSTANT
	if (HasProperty(BME_PROPERTY_KONSTANT))
		m_attacks.Clear(BME_ATTACK_POWER);

	// INSULT
	if (HasProperty(BME_PROPERTY_INSULT))
		m_vulnerabilities.Clear(BME_ATTACK_SKILL);

	// BERSERK
	if (HasProperty(BME_PROPERTY_BERSERK))
	{
		m_attacks.Set(BME_ATTACK_BERSERK);
		m_attacks.Clear(BME_ATTACK_SKILL);
	}

	// stealth dice
	if (HasProperty(BME_PROPERTY_STEALTH))
	{
		m_attacks.Clear(BME_ATTACK_POWER);
		m_vulnerabilities.Clear();
		m_vulnerabilities.Set(BME_ATTACK_SKILL);
	}

	// WARRIOR
	// - cannot be attacked
	// - can only skill attack (with non-warrior)
	if (HasProperty(BME_PROPERTY_WARRIOR))
	{
		m_vulnerabilities.Clear();
		m_attacks.Clear();
		m_attacks.Set(BME_ATTACK_SKILL);
	}

	// queer dice
	if (HasProperty(BME_PROPERTY_QUEER))
	{
		if (m_value_total % 2 == 1)	// odd means acts as a shadow die
		{
			m_attacks.Set(BME_ATTACK_SHADOW);
			m_attacks.Clear(BME_ATTACK_POWER);
		}
	}

	// FOCUS: if dizzy no attacks
	if (m_state == BME_STATE_DIZZY)
		m_attacks.Clear();

}

// POST: all data related to current value have not been set up
void BMC_Die::SetDie(BMC_DieData *_data)
{
	// dead?
	if (!_data || !_data->Valid())
	{
		m_state = BME_STATE_NOTUSED;
		return;
	}

	// first do default copy constructor to set up base class
	*(BMC_DieData*)this = *_data;

	// now set up state
	m_state = BME_STATE_NOTSET;

	// set up actual die
	INT i;
	m_value_total = 0;
	m_sides_max = 0;
	for (i=0; i<Dice(); i++)
	{
		if (GetSwingType(i) != BME_SWING_NOT)
			m_sides[i] = 0;
		else
			m_sides_max += m_sides[i];
		//m_value[i] = 0;
	}

	// set up attacks/vulnerabilities
	//RecomputeAttacks();
}

void BMC_Die::OnUseReserve()
{
	BM_ASSERT(HasProperty(BME_PROPERTY_RESERVE));
	BM_ASSERT(m_state==BME_STATE_RESERVE);

	m_properties &= ~BME_PROPERTY_RESERVE;
	m_state = BME_STATE_NOTSET;
}

void BMC_Die::OnDizzyRecovered()
{
	m_state = BME_STATE_READY;

	RecomputeAttacks();
}

void BMC_Die::OnSwingSet(INT _swing, U8 _value)
{
	if (!IsUsed())
		return;

	BM_ASSERT(m_state=BME_STATE_NOTSET);

	INT i;
	m_sides_max = 0;
	for (i=0; i<Dice(); i++)
	{
		if (GetSwingType(i) == _swing)
			m_sides[i] = _value;
		m_sides_max += m_sides[i];
	}
}

// POST: m_sides[0] is swapped with the desired die
void BMC_Die::SetOption(INT _d)
{
	if (_d != 0)
	{
		INT temp = m_sides[0];
		m_sides[0] = m_sides[_d];
		m_sides[_d] = temp;
	}
	m_sides_max = m_sides[0];
}

void BMC_Die::SetFocus(INT _v)
{
	BM_ASSERT(Dice()==1);
	BM_ASSERT(_v<=m_sides[0]);

	m_value_total = _v;

	m_state = BME_STATE_DIZZY;

	RecomputeAttacks();
}

void BMC_Die::Roll()
{
	if (!IsUsed())
		return;

	BM_ASSERT(m_state=BME_STATE_NOTSET);

	INT i;
	m_value_total = 0;
	for (i=0; i<Dice(); i++)
	{
		BM_ASSERT(i>0 || m_sides[i]>0);	// swing die hasn't been set
		if (m_sides[i]==0)
			break;
		// WARRIOR, MAXIMUM: always rolls highest number
		if (HasProperty(BME_PROPERTY_WARRIOR) || HasProperty(BME_PROPERTY_MAXIMUM))
			m_value_total += m_sides[i];
		else
			m_value_total += g_rng.GetRand(m_sides[i])+1;
	}

	m_state = BME_STATE_READY;

	RecomputeAttacks();
}

float BMC_Die::GetScore(bool _own)
{
	// WARRIOR and NULL: worth 0
	if (HasProperty(BME_PROPERTY_NULL|BME_PROPERTY_WARRIOR))
	{
		return 0;
	}
	else if (HasProperty(BME_PROPERTY_POISON) && HasProperty(BME_PROPERTY_VALUE))
	{
		if (_own)
			return -m_value_total;
		else
			return -m_value_total * BMD_VALUE_OWN_DICE;
	}
	else if (HasProperty(BME_PROPERTY_POISON))
	{
		if (_own)
			return -m_sides_max;
		else
			return -m_sides_max * BMD_VALUE_OWN_DICE;
	}
	else if (HasProperty(BME_PROPERTY_VALUE))
	{
		if (_own)
			return m_value_total * BMD_VALUE_OWN_DICE;
		else
			return m_value_total;
	}
	else
	{
		if (_own)
			return m_sides_max * BMD_VALUE_OWN_DICE;
		else
			return m_sides_max;
	}
}

// DESC: deal with all deterministic effects of attacking, including TURBO
// PARAM: _actually_attacking is normally true, but may be false for forced attack rerolls like ORNERY
void BMC_Die::OnApplyAttackPlayer(BMC_Move &_move, BMC_Player *_owner, bool _actually_attacking)
{
    // TODO determine how this may conflict with the design of `bool _actually_attacking`
    BM_ASSERT(c_attack_type[_move.m_attack]!=BME_ATTACK_TYPE_0_0);

	// clear value
	// KONSTANT: don't reroll
	if (!HasProperty(BME_PROPERTY_KONSTANT))
		SetState(BME_STATE_NOTSET);

	// BERSERK attack - half size and round fractions up
	if (_actually_attacking && _move.m_attack == BME_ATTACK_BERSERK)
	{
		BM_ASSERT(c_attack_type[_move.m_attack]==BME_ATTACK_TYPE_1_N);
		BM_ASSERT(Dice()==1);

		_owner->OnDieSidesChanging(this);
		m_sides[0] = (m_sides[0]+1) / 2;
		m_sides_max = m_sides[0];
		m_properties &= ~BME_PROPERTY_BERSERK;
		_owner->OnDieSidesChanged(this);
	}

	if (GetState()==BME_STATE_NOTSET)
		OnBeforeRollInGame(_owner);

	// MORPHING - assume same size as target
	// drp022521 - this rule only applies if actually going to capture a die
	if (HasProperty(BME_PROPERTY_MORPHING) && _move.m_attack != BME_ATTACK_INVALID)
	{
		BM_ASSERT(c_attack_type[_move.m_attack]==BME_ATTACK_TYPE_1_1 || c_attack_type[_move.m_attack]==BME_ATTACK_TYPE_N_1);
		BMC_Player *target = _move.m_game->GetPlayer(_move.m_target_player);

		_owner->OnDieSidesChanging(this);
		if (target->GetDie(_move.m_target)->HasProperty(BME_PROPERTY_TWIN) )
		{
			AddProperty(BME_PROPERTY_TWIN);
			m_sides_max = target->GetDie(_move.m_target)->GetSidesMax();
			for (int i = 0; i<target->GetDie(_move.m_target)->Dice(); i++)
				m_sides[i] = target->GetDie(_move.m_target)->GetSides(i);
		}
		else
		{
			RemoveProperty(BME_PROPERTY_TWIN);
			m_sides_max = m_sides[0] = target->GetDie(_move.m_target)->GetSidesMax();
		}
		_owner->OnDieSidesChanged(this);
	}

	// TURBO dice
	if (HasProperty(BME_PROPERTY_TURBO))
	{
		// TURBO OPTION - no change if sticking with '0'
		if (HasProperty(BME_PROPERTY_OPTION) &&  _move.m_turbo_option==1)
		{
			_owner->OnDieSidesChanging(this);
			SetOption(_move.m_turbo_option);
			_owner->OnDieSidesChanged(this);
		}
		// TURBO SWING
		else if (_move.m_turbo_option>0)
		{
			BM_ASSERT(GetSwingType(0)!=BME_SWING_NOT);
			_owner->OnDieSidesChanging(this);
			_owner->SetSwingDice(GetSwingType(0), _move.m_turbo_option, true);
			_owner->OnDieSidesChanged(this);
		}
	}


	// WARRIOR: loses property
	// TODO: KONSTANT will be forced to reroll but shouldn't (OnDieSidesChanging misinterpreting situation)
	if (HasProperty(BME_PROPERTY_WARRIOR))
	{
		_owner->OnDieSidesChanging(this);
		m_properties &= ~BME_PROPERTY_WARRIOR;
		_owner->OnDieSidesChanged(this);
	}
}

// DESC: effects that happen whenever rerolling die (for whatever reason)
void BMC_Die::OnBeforeRollInGame(BMC_Player *_owner)
{
	// MIGHTY
	if (HasProperty(BME_PROPERTY_MIGHTY))
	{
		_owner->OnDieSidesChanging(this);
		m_sides_max = 0;
		for (int d=0; d<Dice(); d++)
		{
			if (m_sides[d]>=20)
				m_sides_max += (m_sides[d] = 30);
			else
				m_sides_max += (m_sides[d] = c_mighty_sides[m_sides[d]]);
		}
		_owner->OnDieSidesChanged(this);
	}

	// WEAK
	if (HasProperty(BME_PROPERTY_WEAK))
	{
		_owner->OnDieSidesChanging(this);
		m_sides_max = 0;
		for (int d=0; d<Dice(); d++)
		{
			if (m_sides[d]>30)
				m_sides_max += (m_sides[d] = 30);
			else if (m_sides[d]>20)
				m_sides_max += (m_sides[d] = 20);
			else if (m_sides[d]==20)
				m_sides_max += (m_sides[d] = 16);
			else
				m_sides_max += (m_sides[d] = c_weak_sides[m_sides[d]]);
		}
		_owner->OnDieSidesChanged(this);
	}
}

void BMC_Die::OnApplyAttackNatureRollAttacker(BMC_Move &_move, BMC_Player *_owner)
{
	INT i;

	// MOOD dice
	if (HasProperty(BME_PROPERTY_MOOD))
	{
		_owner->OnDieSidesChanging(this);
		m_sides_max = 0;
		INT delta;
		INT swing;
		for (i=0; i<Dice(); i++)
		{
			swing = GetSwingType(i);
			switch (swing)
			{
			case BME_SWING_X:
				m_sides[i] = c_mood_sides_X[g_rng.GetRand(BMD_MOOD_SIDES_RANGE_X)];
				break;
			case BME_SWING_V:
				m_sides[i] = c_mood_sides_V[g_rng.GetRand(BMD_MOOD_SIDES_RANGE_V)];
				break;
			default:
				// some BM use MOOD SWING on other than X and V
				delta = c_swing_sides_range[swing][1] - c_swing_sides_range[swing][0];
				m_sides[i] = g_rng.GetRand(delta+1)+ c_swing_sides_range[swing][0];
				break;
			}
			m_sides_max += m_sides[i];

		}
		_owner->OnDieSidesChanged(this);
	}

	// reroll
	Roll();
}

void BMC_Die::OnApplyAttackNatureRollTripped()
{
	// reroll
	Roll();
}

void BMC_Die::Debug(BME_DEBUG _cat)
{
	BMC_DieData::Debug(_cat);

	printf("%d:%d ", m_sides_max, m_value_total);
}
