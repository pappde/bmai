///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Move.h
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp
// denis@accessdenied.net
// https://github.com/pappde/bmai
//
// REVISION HISTORY:
// dbl100524 - further split out of individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>
#include "BMC_BitArray.h"


class BMC_Game;
class BMC_Player;

class BMC_Move
{
public:
	BMC_Move() {} //: m_pool_index(-1)	{}

	// methods
	void	Debug(BME_DEBUG _cat = BME_DEBUG_ALWAYS, const char *_postfix = "\n");
	bool	operator<  (const BMC_Move &_m) const  { return this < &_m; }
	bool	operator==	(const BMC_Move &_m) const { return this == &_m; }

	// accessors
	bool	MultipleAttackers() { return c_attack_type[m_attack]==BME_ATTACK_TYPE_N_1; }
	bool	MultipleTargets()	{ return c_attack_type[m_attack]==BME_ATTACK_TYPE_1_N; }
	BMC_Player *	GetAttacker();
	BMC_Player *	GetTarget();

	// data
	BME_ACTION					m_action;
	//INT							m_pool_index;
	BMC_Game					*m_game;
	union
	{
		// BME_ACTION_ATTACK
		struct {
			BME_ATTACK					m_attack;
			BMC_BitArray<BMD_MAX_DICE>	m_attackers;
			BMC_BitArray<BMD_MAX_DICE>	m_targets;
			S8							m_attacker;
			S8							m_target;
			U8							m_attacker_player;
			U8							m_target_player;
			S8							m_turbo_option;	// only supports one die, 0/1 for which sides to go with. -1 means none
		};

		// BME_ACTION_SET_SWING_AND_OPTION: work data for BMAI::RandomlySelectMoves

		// BME_ACTION_SET_SWING_AND_OPTION,
		struct {
			U8			m_swing_value[BME_SWING_MAX];
			//U8			m_option_die[BMD_MAX_DICE];	// use die 0 or 1
			BMC_BitArray<BMD_MAX_DICE>	m_option_die;	// use die 0 or 1
			U8			m_extreme_settings;			// work data for "BMAI::RandomlySelectMoves"
		};

		// BME_ACTION_USE_CHANCE
		struct {
			BMC_BitArray<BMD_MAX_DICE>	m_chance_reroll;
		};

		// BME_ACTION_USE_FOCUS,
		struct {
			U8			m_focus_value[BMD_MAX_DICE];
		};

		// BME_ACTION_USE_RESERVE (use ACTION_PASS to use none)
		struct {
			U8			m_use_reserve;
		};
	};
};

#define	BMC_MoveAttack	BMC_Move

typedef std::vector<BMC_Move>	BMC_MoveVector;

class BMC_MoveList
{
public:
	BMC_MoveList();
	~BMC_MoveList() { Clear(); }
	void	Clear();
	void	Add(BMC_Move  & _move );
	INT		Size() { return (INT)list.size(); }
	BMC_Move *	Get(INT _i) { return &list[_i]; }
	bool	Empty() { return Size()<1; }
	void	Remove(int _index);
	BMC_Move &	operator[](int _index) { return list[_index]; }

protected:
private:
	BMC_MoveVector	list;
};
