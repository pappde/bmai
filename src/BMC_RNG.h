///////////////////////////////////////////////////////////////////////////////////////////
// BMC_RNG.h
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
//
// DESC: track performance stats
//
// REVISION HISTORY:
// dbl100524 - further split out of individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once


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
inline BMC_RNG		g_rng;