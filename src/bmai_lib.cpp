///////////////////////////////////////////////////////////////////////////////////////////
// bmai.cpp
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: small CPP for some globally defined settings
//
// REVISION HISTORY:
//
///////////////////////////////////////////////////////////////////////////////////////////

#include "bmai_lib.h"


float s_ply_decay = 0.5f;
float g_turbo_accuracy = 1;	// 0 is worst, 1 is best

// global definitions
BME_ATTACK_TYPE	g_attack_type[BME_ATTACK_MAX] =
{
	BME_ATTACK_TYPE_1_1,  // FIRST & POWER 1:1
	BME_ATTACK_TYPE_N_1,  // SKILL N:1
	BME_ATTACK_TYPE_1_N,  // BERSERK
	BME_ATTACK_TYPE_1_N,  // SPEED
	BME_ATTACK_TYPE_1_1,  // TRIP
	BME_ATTACK_TYPE_1_1,  // SHADOW
	BME_ATTACK_TYPE_0_0,  // INVALID
};

// SWING dice - from spindisc's page
INT g_swing_sides_range[BME_SWING_MAX][2] =
{
	{ 0, 0 },
	{ 1, 30 },	// P
	{ 2, 20 },	// Q
	{ 2, 16 },
	{ 6, 20 },
	{ 2, 12 },
	{ 8, 30 },	// U
	{ 6, 12 },
	{ 4, 12 },
	{ 4, 20 },	// X
	{ 1, 20 },
	{ 4, 30 },	// Z
};

//X?: Roll a d6. 1: d4; 2: d6; 3: d8; 4: d10; 5: d12; 6: d20.
//V?: Roll a d4. 1: d6; 2: d8; 3: d10; 4: d12.
INT g_mood_sides_X[BMD_MOOD_SIDES_RANGE_X] = { 4, 6, 8,  10, 12, 20 };
INT g_mood_sides_V[BMD_MOOD_SIDES_RANGE_V] = { 6, 8, 10, 12 };

// MIGHTY dice - index by old number of sides
INT g_mighty_sides[20] = { 1, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 16, 16, 16, 16, 20, 20, 20, 20 };

// WEAK dice - index by old number of sides
INT g_weak_sides[20] = { 1, 1, 1, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 12, 12, 16, 16, 16 };


const char *g_swing_name[BME_SWING_MAX] =
{
	"None",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
};

// PHASE names
const char *g_phase_name[BME_PHASE_MAX] =
{
	"preround",
	"reserve",
	"initiative",
	"chance",
	"focus",
	"fight",
	"gameover"
};

// ATTACK names
const char *g_attack_name[BME_ATTACK_MAX] =
{
	"power",
	"skill",
	"berserk",
	"speed",
	"trip",
	"shadow",
	"invalid",
};

// ACTION names
const char *g_action_name[BME_ACTION_MAX] =
{
	"aux",
	"chance",
	"focus",
	"swing/option",
	"reserve",
	"attack",
	"pass",
	"surrender",
};

// BME_DEBUG setting names
const char *g_debug_name[BME_DEBUG_MAX] =
{
	"ALWAYS",
	"WARNING",
	"PARSER",
	"SIMULATION",
	"ROUND",
	"GAME",
	"QAI",
	"BMAI",
};
