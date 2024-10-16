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
float s_turbo_accuracy = 1;	// 0 is worst, 1 is best

// global definitions
BME_ATTACK_TYPE	c_attack_type[BME_ATTACK_MAX] =
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
INT c_swing_sides_range[BME_SWING_MAX][2] =
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

// ATTACK names
const char *c_attack_name[BME_ATTACK_MAX] =
{
	"power",
	"skill",
	"berserk",
	"speed",
	"trip",
	"shadow",
	"invalid",
};
