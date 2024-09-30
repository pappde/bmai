///////////////////////////////////////////////////////////////////////////////////////////
// bmai.h
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: main header for BMAI
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
// dbl051823 - added P-Swing, Q-Swing support
// drp060323 - added const modifier to vararg format params
//
// TODO:
// 1) drp030321 - setup a main precompiled header that includes everything (bmai.h) vs a header for the key types/enums/classes. Split out modules
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef __BMAI_H__
#define __BMAI_H__

// dependent includes (and high-level)
#include <cassert>
#include <vector>
#include <ctime>

// some compilers want to be very explicit
#include <cstdio>   // FILE

// to help make a cross-platform case insensitive compare
// we need transform and string
// drp022521 - moved to main header
#include <algorithm>
#include <string>

#include "types.h"

int bmai_main(int argc, char *argv[]);

// forward declarations
class BMC_Player;
class BMC_Game;
class BMC_AI;

// version number
#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "n/a"
#endif

// assert
#define BM_ASSERT	assert
#define BM_ERROR(check)	{ if (!(check)) { BMF_Error(#check); } }

// C prototypes
void BMF_Error( const char *_fmt, ... );

// template classes
template <int SIZE>
class BMC_BitArray
{
public:
	static const INT m_bytes;

	// mutators
	void SetAll();
	void Set() { SetAll(); }
	void ClearAll();
	void Clear() { ClearAll(); }
	void Set(int _bit)		{ INT byte = _bit/8; bits[byte] |= (1<<(_bit-byte*8)); }
	void Clear(int _bit)	{ INT byte = _bit/8; bits[byte] &= ~(1<<(_bit-byte*8)); }
	void Set(int _bit, bool _on)	{ if (_on) Set(_bit); else Clear(_bit); }

	// accessors
	bool	IsSet(INT _bit)	{ INT byte = _bit/8; return bits[byte] & (1<<(_bit-byte*8)); }
	bool	operator[](int _bit) { return IsSet(_bit) ? 1 : 0; }

private:
	U8	bits[(SIZE+7)>>3];	
};

template<int SIZE>
const INT BMC_BitArray<SIZE>::m_bytes = (SIZE+7)>>3;

template<int SIZE>
void BMC_BitArray<SIZE>::SetAll()
{
	INT i;
	for (i=0; i<m_bytes; i++)
		bits[i] = 0xFF;
}

template<int SIZE>
void BMC_BitArray<SIZE>::ClearAll()
{
	INT i;
	for (i=0; i<m_bytes; i++)
		bits[i] = 0x00;
}

// utilities

class BMC_RNG
{
public:
	BMC_RNG();

	UINT	GetRand(UINT _i)	{ return GetRand() % _i; }
	F32		GetFRand()			{ return (float)GetRand() / (float)0x80000000; }
	UINT	GetRand();		
	void	SRand(UINT _seed);

private:
	UINT	m_seed;
};

// settings
#define BMD_MAX_TWINS	2
#define BMD_MAX_DICE	10
#define BMD_MAX_PLAYERS	2
#define BMD_DEFAULT_WINS	3
#define	BMD_VALUE_OWN_DICE	0.5f
#define BMD_SURRENDERED_SCORE	-1000
#define BMD_MAX_STRING	256
#define BMD_DEFAULT_WINS	3
#define BMD_MAX_PLY		10

// debug categories
enum BME_DEBUG
{
	BME_DEBUG_ALWAYS,
	BME_DEBUG_WARNING,
	BME_DEBUG_PARSER,
	BME_DEBUG_SIMULATION,
	BME_DEBUG_ROUND,
	BME_DEBUG_GAME,
	BME_DEBUG_QAI,
	BME_DEBUG_BMAI,
	BME_DEBUG_MAX
};

// all the different "actions" that can be submitted 
enum BME_ACTION
{
	xBME_ACTION_USE_AUXILIARY,
	BME_ACTION_USE_CHANCE,
	BME_ACTION_USE_FOCUS,
	BME_ACTION_SET_SWING_AND_OPTION,
	BME_ACTION_USE_RESERVE,
	BME_ACTION_ATTACK,
	BME_ACTION_PASS,
	BME_ACTION_SURRENDER,
	BME_ACTION_MAX
};

// attack-related dice are specially indicated
// D = attack defining stage
// P = apply player attack stage
// N = apply nature attack stage
enum BME_PROPERTY
{
	BME_PROPERTY_TIME_AND_SPACE	=	 0x0001, // (N) post-attack effect
	BME_PROPERTY_AUXILIARY		=	 0x0002, // pre-game action
	BME_PROPERTY_QUEER			=	 0x0004, // attacks
	BME_PROPERTY_TRIP			=	 0x0008, // (N) attacks, special attack, initiative effect
	BME_PROPERTY_SPEED			=	 0x0010, // attacks
	BME_PROPERTY_SHADOW			=	 0x0020, // attacks
	BME_PROPERTY_BERSERK		=	 0x0040, // attacks
	BME_PROPERTY_STEALTH		=	 0x0080, // attacks
	BME_PROPERTY_POISON			=	 0x0100, // score modifier
	BME_PROPERTY_NULL			=	 0x0200,	// (N) capture modifier
	BME_PROPERTY_MOOD			=	 0x0400,	// (N) post-attack effect
	BME_PROPERTY_TURBO			=	 0x0800,	// (DP) post-attack action
	BME_PROPERTY_OPTION			=	 0x1000,	// swing, uses two dice
	BME_PROPERTY_TWIN			=	 0x2000,	// uses two dice
	BME_PROPERTY_FOCUS			=	 0x4000,	// initiative action
	BME_PROPERTY_VALID			=	 0x8000,	// without this, it isn't a valid die!
	BME_PROPERTY_MIGHTY			=   0x10000,	// (P) pre-roll effect
	BME_PROPERTY_WEAK			=   0x20000,	// (P) pre-roll effect
	BME_PROPERTY_RESERVE		=   0x40000,	// preround (after a loss)
	BME_PROPERTY_ORNERY			=   0x80000,	// (N) post-attack effect on all dice
	BME_PROPERTY_DOPPLEGANGER	=  0x100000,	// (P) post-attack effect
	BME_PROPERTY_CHANCE			=  0x200000, // initiative action
	BME_PROPERTY_MORPHING		=  0x400000, // (P) post-attack effect
	BME_PROPERTY_RADIOACTIVE	=  0x800000, // (P) post-attack effect
	BME_PROPERTY_WARRIOR		= 0x1000000, // attacks
	BME_PROPERTY_SLOW			= 0x2000000,	// initiative effect
	BME_PROPERTY_UNIQUE			= 0x4000000,	// (D) swing rule
	BME_PROPERTY_UNSKILLED		= 0x8000000,	// attacks [The Flying Squirrel]	// TODO: implement as BM property?
	BME_PROPERTY_STINGER		=0x10000000,	// initiative effect, (D) skill attacks
	BME_PROPERTY_RAGE			=0x20000000,	// TODO
	BME_PROPERTY_KONSTANT			=0x40000000,	
	BME_PROPERTY_MAXIMUM			=0x80000000,
	BME_PROPERTY_MAX
};

enum BME_SWING
{
	BME_SWING_NOT,
	BME_SWING_P,
	BME_SWING_Q,
	BME_SWING_R,
	BME_SWING_S,
	BME_SWING_T,
	BME_SWING_U,
	BME_SWING_V,
	BME_SWING_W,
	BME_SWING_X,
	BME_SWING_Y,
	BME_SWING_Z,
	BME_SWING_MAX,
	BME_SWING_FIRST = BME_SWING_NOT+1
};

#define	BMD_FIRST_SWING_CHAR	'P'
#define BMD_LAST_SWING_CHAR		'Z'

enum BME_STATE
{
	BME_STATE_READY,
	BME_STATE_NOTSET,
	BME_STATE_CAPTURED,
	BME_STATE_DIZZY,		// focus dice - one turn paralysis 
	BME_STATE_NULLIFIED,
	BME_STATE_NOTUSED,		// this die (slot) isn't being used at all this game
	BME_STATE_RESERVE,		
	BME_STATE_MAX
};

enum BME_WLT
{
	BME_WLT_WIN,
	BME_WLT_LOSS,
	BME_WLT_TIE,
	BME_WLT_MAX
};

enum BME_PHASE
{
	BME_PHASE_PREROUND,
	BME_PHASE_RESERVE,				// part of PREROUND
	BME_PHASE_INITIATIVE,			
	BME_PHASE_INITIATIVE_CHANCE,	// part of INITIATIVE
	BME_PHASE_INITIATIVE_FOCUS,		// part of INITIATIVE
	BME_PHASE_FIGHT,
	BME_PHASE_GAMEOVER,
	BME_PHASE_MAX
};

enum BME_ATTACK
{
	BME_ATTACK_FIRST = 0,
	BME_ATTACK_POWER = BME_ATTACK_FIRST,	// 1 -> 1
	BME_ATTACK_SKILL,						// N -> 1
	BME_ATTACK_BERSERK,						// 1 -> N
	BME_ATTACK_SPEED,						// 1 -> N
	BME_ATTACK_TRIP,						// 1 -> 1
	BME_ATTACK_SHADOW,						// 1 -> 1
	BME_ATTACK_INVALID,						// 0 -> 0 (like pass)
	BME_ATTACK_MAX
};

enum BME_ATTACK_TYPE
{
	BME_ATTACK_TYPE_1_1,
	BME_ATTACK_TYPE_N_1,
	BME_ATTACK_TYPE_1_N,
	BME_ATTACK_TYPE_0_0,
	BME_ATTACK_TYPE_MAX
};

// global definitions

extern BME_ATTACK_TYPE	g_attack_type[BME_ATTACK_MAX];
extern INT g_swing_sides_range[BME_SWING_MAX][2];

// globals

extern class BMC_Logger	g_logger;
extern class BMC_QAI	g_qai;
extern class BMC_Stats	g_stats;

// MOOD dice
#define BMD_MOOD_SIDES_RANGE_X	6
#define BMD_MOOD_SIDES_RANGE_V	4
extern INT g_mood_sides_X[BMD_MOOD_SIDES_RANGE_X];
extern INT g_mood_sides_V[BMD_MOOD_SIDES_RANGE_V];

///////////////////////////////////////////////////////////////////////////////////////////
// classes
///////////////////////////////////////////////////////////////////////////////////////////

class BMC_Move
{
public:
	BMC_Move() {} //: m_pool_index(-1)	{}

	// methods
	void	Debug(BME_DEBUG _cat = BME_DEBUG_ALWAYS, const char *_postfix = "\n");
	bool	operator<  (const BMC_Move &_m) const  { return this < &_m; }
	bool	operator==	(const BMC_Move &_m) const { return this == &_m; }

	// accessors
	bool	MultipleAttackers() { return g_attack_type[m_attack]==BME_ATTACK_TYPE_N_1; }
	bool	MultipleTargets()	{ return g_attack_type[m_attack]==BME_ATTACK_TYPE_1_N; }
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

class TGC_MovePower
{
public:
protected:
private:
	U8	m_attacker;
	U8	m_target;
};

/*
class TGC_MoveSkill
{
public:
protected:
private:
	BMC_BitArray<BMD_MAX_DICE>	m_attackers;
	U8	m_target;
};

class TGC_MoveSpeed
{
public:
protected:
private:
	U8	m_attacker;
	BMC_BitArray<BMD_MAX_DICE>	m_targets;
};
*/







// Utility functions

void BMF_Error( const char *_fmt, ... );
void BMF_Log(BME_DEBUG _cat, const char *_fmt, ... );

#endif //__BMAI_H__
