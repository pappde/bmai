///////////////////////////////////////////////////////////////////////////////////////////
// die.h
// Copyright (c) 2001-2023 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: main header for Die classes
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// TODO_HEADERS: drp030321 - cleanup headers
#include "bmai.h"

// NOTES:
// - second die only used for Option and Twin
class BMC_DieData
{
	friend class BMC_Parser;

public:
	BMC_DieData();
	virtual void Reset();

	// methods
	void		Debug(BME_DEBUG _cat = BME_DEBUG_ALWAYS);

	// accessors
	bool		HasProperty(INT _p) { return m_properties & _p; }
	BME_SWING	GetSwingType(INT _d) { return (BME_SWING)m_swing_type[_d]; }
	bool		Valid() { return (m_properties & BME_PROPERTY_VALID); }
	// drp022521 - fixed to return INT instead of bool
	INT			Dice() { return (m_properties & BME_PROPERTY_TWIN) ? 2 : 1; }
	INT			GetSides(INT _t) { return m_sides[_t]; }

protected:
	U32			m_properties;
	U8			m_sides[BMD_MAX_TWINS];			// number of sides if not a swing die, or current number of sides

private:
	U8			m_swing_type[BMD_MAX_TWINS];	// definition number of sides, should be BME_SWING
};

class BMC_Die : public BMC_DieData
{
	friend		class BMC_Parser;

public:
	// setup
	virtual void Reset();
	void		SetDie(BMC_DieData *_data);
	void		Roll();
	void		GameRoll(BMC_Player *_owner);

	// methods
	void		Debug(BME_DEBUG _cat = BME_DEBUG_ALWAYS);
	void		SetOption(INT _d);
	void		SetFocus(INT _v);

	// accessors
	bool		CanDoAttack(BMC_MoveAttack &_move) { return m_attacks.IsSet(_move.m_attack); }
	bool		CanBeAttacked(BMC_MoveAttack &_move) { return m_vulnerabilities.IsSet(_move.m_attack); }
	INT			GetValueTotal() { return m_value_total; }
	INT			GetSidesMax() { return m_sides_max; }
	bool		IsAvailable() { return m_state == BME_STATE_READY || m_state == BME_STATE_DIZZY; }
	bool		IsInReserve() { return m_state == BME_STATE_RESERVE; }
	bool		IsUsed() { return m_state != BME_STATE_NOTUSED && m_state != BME_STATE_RESERVE; }
	float		GetScore(bool _own);
	INT			GetOriginalIndex() { return m_original_index; }
	BME_STATE	GetState() { return (BME_STATE)m_state; }

	// mutators
	void		SetState(BME_STATE _state) { m_state = _state; }
	void		CheatSetValueTotal(INT _v) { m_value_total = _v; }	// used for some functions

	// events
	void		OnDieChanged();
	void		OnSwingSet(INT _swing, U8 _value);
	void		OnApplyAttackPlayer(BMC_Move &_move, BMC_Player *_owner, bool _actually_attacking = true);
	void		OnApplyAttackNatureRollAttacker(BMC_Move &_move, BMC_Player *_owner);
	void		OnApplyAttackNatureRollTripped();
	void		OnBeforeRollInGame(BMC_Player *_owner);
	void		OnUseReserve();
	void		OnDizzyRecovered();

protected:

	// call this once state changes
	void		RecomputeAttacks();

private:
	U8			m_state;				// should be BME_STATE
	U8			m_value_total;			// current total value of all dice, redundant
	U8			m_sides_max;			// max value of dice (based on m_sides), redundant
	U8			m_original_index;		// save original index for interface
	BMC_BitArray<BME_ATTACK_MAX>	m_attacks;
	BMC_BitArray<BME_ATTACK_MAX>	m_vulnerabilities;
	//U8			m_value[BMD_MAX_TWINS];	// current value of dice, not really necessary (and often not known!)
};

