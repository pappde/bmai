///////////////////////////////////////////////////////////////////////////////////////////
// BMC_RNG.cpp
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp
// denis@accessdenied.net
// https://github.com/pappde/bmai
//
// DESC: a good RNG, was better and faster than the old standard library random(), but
// have not evaluated it against MSVC 6's standard library random.  This is taken
// from my thesis Poker AI, which in turn took it from somewhere else.
//
// TEST: with 1M samples of FRand and 10 distribution sets, the stddev was 3.7, with
// a maximum "error" of 0.29%.  For the runtime library "rand()" the stddev was 4 with a maximum
// "error" of 0.52%.
//
// REVISION HISTORY:
// dbl100524 - broke this logic out into its own class file
///////////////////////////////////////////////////////////////////////////////////////////

#include "BMC_RNG.h"

#include <ctime>


// global
BMC_RNG		g_rng;

BMC_RNG::BMC_RNG() :
        m_seed(78904497)
{
}

// PARAM: 0 means completely random
void BMC_RNG::SRand(UINT _seed)
{
  if (_seed==0)
    _seed = (UINT)time(NULL);

  // bad arbitrary way of tweaking bad seed values
  if (!_seed)
    m_seed=-1;
  else if((_seed>>16)==0)
    m_seed = _seed | (_seed<<16);
  else
    m_seed = _seed;
}

UINT BMC_RNG::GetRand()
{
  // drp060323 - removed "register" keyword, which is no longer supported in C++ versions after 14. Optimizer should be making it irrelevant.
  long LO, HI;

  LO = ( (m_seed) & 0177777 ) * 16807;
  HI = ( (m_seed) >> 16 ) * 16807 + ( LO >> 16 );
  LO = ( LO & 0177777 ) + ( HI >> 15 );
  HI = ( HI & 0077777 ) + ( LO >> 16 );
  LO = ( LO & 0177777 ) + ( HI >> 15 );
  HI = (( HI & 077777 ) << 16 ) + LO;
  return( (m_seed) = HI );
}
