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
// dbl100824 - pulled a lot out of bmai.h depends on very little and initializes a lot for pre-compilation
//
// TODO:
// 1) drp030321 - setup a main precompiled header that includes everything (bmai.h) vs a header for the key types/enums/classes. Split out modules
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// i like that this include is VERY small and include no BMC_ classes
#include <vector>
#include <cstdint>


// version number
#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "n/a"
#endif

// TODO consider best place to organize globals. should classes host their own?
// note that right now many classes are hosting their own global instance
// see BMC_Logger, BMC_RNG, BMC_Stats, BMC_Game
// and to clean up some circular/referential dependency issues bmai_ai.h holds some AI globals

// typedefs
typedef uint64_t		U64;
typedef unsigned long	U32;
typedef unsigned char	U8;
typedef signed char		S8;
typedef unsigned short	U16;
typedef int				INT;
typedef unsigned int	UINT;
typedef float			F32;

// special types
typedef std::vector<float>	BMC_FloatVector;

// settings
#define BMD_MAX_TWINS			2
#define BMD_MAX_DICE			10
#define BMD_MAX_PLAYERS			2
#define BMD_DEFAULT_WINS		3
#define	BMD_VALUE_OWN_DICE		0.5f
#define BMD_SURRENDERED_SCORE	-1000
#define BMD_MAX_STRING			256
#define BMD_DEFAULT_WINS		3
#define BMD_MAX_PLY				10

// ai settings
#define BMD_DEFAULT_SIMS		500
#define BMD_MIN_SIMS			10
#define BMD_QAI_FUZZINESS		5
#define BMD_MAX_PLY_PREROUND	2
#define BMD_AI_TYPES			3

// MOOD dice - from BM page:
#define BMD_MOOD_SIDES_RANGE_X	6
#define BMD_MOOD_SIDES_RANGE_V	4

extern float s_ply_decay;
extern float s_turbo_accuracy;

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
	BME_PROPERTY_TIME_AND_SPACE	=	  0x0001,	// (N) post-attack effect
	BME_PROPERTY_AUXILIARY		=	  0x0002,	// pre-game action
	BME_PROPERTY_QUEER			=	  0x0004,	// attacks
	BME_PROPERTY_TRIP			=	  0x0008,	// (N) attacks, special attack, initiative effect
	BME_PROPERTY_SPEED			=	  0x0010,	// attacks
	BME_PROPERTY_SHADOW			=	  0x0020,	// attacks
	BME_PROPERTY_BERSERK		=	  0x0040,	// attacks
	BME_PROPERTY_STEALTH		=	  0x0080,	// attacks
	BME_PROPERTY_POISON			=	  0x0100,	// score modifier
	BME_PROPERTY_NULL			=	  0x0200,	// (N) capture modifier
	BME_PROPERTY_MOOD			=	  0x0400,	// (N) post-attack effect
	BME_PROPERTY_TURBO			=	  0x0800,	// (DP) post-attack action
	BME_PROPERTY_OPTION			=	  0x1000,	// swing, uses two dice
	BME_PROPERTY_TWIN			=	  0x2000,	// uses two dice
	BME_PROPERTY_FOCUS			=	  0x4000,	// initiative action
	BME_PROPERTY_VALID			=	  0x8000,	// without this, it isn't a valid die!
	BME_PROPERTY_MIGHTY			=    0x10000,	// (P) pre-roll effect
	BME_PROPERTY_WEAK			=    0x20000,	// (P) pre-roll effect
	BME_PROPERTY_RESERVE		=    0x40000,	// preround (after a loss)
	BME_PROPERTY_ORNERY			=    0x80000,	// (N) post-attack effect on all dice
	BME_PROPERTY_DOPPLEGANGER	=   0x100000,	// (P) post-attack effect
	BME_PROPERTY_CHANCE			=   0x200000,	// initiative action
	BME_PROPERTY_MORPHING		=   0x400000,	// (P) post-attack effect
	BME_PROPERTY_RADIOACTIVE	=   0x800000,	// (P) post-attack effect
	BME_PROPERTY_WARRIOR		=  0x1000000,	// attacks
	BME_PROPERTY_SLOW			=  0x2000000,	// initiative effect
	BME_PROPERTY_UNIQUE			=  0x4000000,	// (D) swing rule
	BME_PROPERTY_UNSKILLED		=  0x8000000,	// attacks [The Flying Squirrel]	// TODO: implement as BM property?
	BME_PROPERTY_STINGER		= 0x10000000,	// initiative effect, (D) skill attacks
	BME_PROPERTY_RAGE			= 0x20000000,	// TODO
	BME_PROPERTY_KONSTANT		= 0x40000000,
	BME_PROPERTY_MAXIMUM		= 0x80000000,
	BME_PROPERTY_INSULT			=0x100000000,
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

extern BME_ATTACK_TYPE	c_attack_type[BME_ATTACK_MAX];

extern INT c_swing_sides_range[BME_SWING_MAX][2];

extern const char *c_attack_name[BME_ATTACK_MAX];
