///////////////////////////////////////////////////////////////////////////////////////////
// BMC_RNG.h
//
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp <denis@accessdenied.net>
//
// DESC: track performance stats
//
// REVISION HISTORY:
// dbl100524 - further split out of individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "bmai_lib.h"


class BMC_RNG
{
public:
  BMC_RNG();

  UINT	GetRand(UINT _i)	{ return GetRand() % _i; }
  F32	GetFRand()		{ return (float)GetRand() / (float)0x80000000; }
  UINT	GetRand();
  void	SRand(UINT _seed);

private:
  UINT	m_seed;
};

// global
extern BMC_RNG		g_rng;