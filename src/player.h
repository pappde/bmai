///////////////////////////////////////////////////////////////////////////////////////////
// player.h
// Copyright (c) 2001-2023 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: player class
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// TODO_HEADERS: drp030321 - clean up headers
#include "bmai.h"
#include "man.h"

class BMC_Player	// 204b
{
	friend class BMC_Parser;

public:
	typedef enum {
		SWING_SET_NOT,
		SWING_SET_READY,	// set this round, esults should not be "known" to opponent - to simulate simultaneous swing set
		SWING_SET_LOCKED,	// set from previous round
	} SWING_SET;

	BMC_Player();
	void		SetID(INT _id) { m_id = _id; }
	void		Reset();
	void		OnDiceParsed();

	// playing with normal rules
	void		SetButtonMan(BMC_Man *_man);
	void		SetSwingDice(INT _swing, U8 _value, bool _from_turbo = false);
	void		SetOptionDie(INT _i, INT _d);
	void		RollDice();

	// methods
	void		Debug(BME_DEBUG _cat = BME_DEBUG_ALWAYS);
	void		DebugAllDice(BME_DEBUG _cat = BME_DEBUG_ALWAYS);
	void		SetSwingDiceStatus(SWING_SET _swing) { m_swing_set = _swing; }
	bool		NeedsSetSwing();

	// events
	void		OnDieSidesChanging(BMC_Die *_die);
	void		OnDieSidesChanged(BMC_Die *_die);
	BMC_Die *	OnDieLost(INT _d);
	void		OnDieCaptured(BMC_Die *_die);
	void		OnRoundLost();
	void		OnSurrendered();
	//void		OnSwingDiceSet() { m_swing_set = true; }
	void		OnSwingDiceReady() { m_swing_set = SWING_SET_READY; }
	void		OnAttackFinished() { OptimizeDice(); }
	void		OnDieTripped() { OptimizeDice(); }
	void		OnChanceDieRolled() { OptimizeDice(); }
	void		OnFocusDieUsed() { OptimizeDice(); }
	void		OnReserveDieUsed(BMC_Die *_die);

	// accessors
	BMC_Die *	GetDie(INT _d) { return &m_die[_d]; }
	INT			GetAvailableDice() { return m_available_dice; }
	INT			GetMaxValue() { return m_max_value; }
	float		GetScore() { return m_score; }
	//bool		SwingDiceSet() { return m_swing_set; }
	SWING_SET	GetSwingDiceSet() { return m_swing_set; }
	INT			HasDieWithProperty(INT _p, bool _check_all_dice = false);
	INT			GetTotalSwingDice(INT _s) { return m_swing_dice[_s]; }
	INT			GetID() { return m_id; }


protected:
	// methods
	void		OptimizeDice();

private:
	BMC_Man	*	m_man;
	INT			m_id;
	SWING_SET	m_swing_set;
	BMC_Die		m_die[BMD_MAX_DICE];			// as long as Optimize was called, these are sorted largest to smallest that are READY
	U8			m_swing_value[BME_SWING_MAX];
	U8			m_swing_dice[BME_SWING_MAX];	// number of dice of each swing type
	INT			m_available_dice;				// only valid after Optimize
	INT			m_max_value;					// only valid after Optimize, useful to know for skill attacks
	float		m_score;
};