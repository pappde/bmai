///////////////////////////////////////////////////////////////////////////////////////////
// bmai_ai.h
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: collection of AI related globals
//
// REVISION HISTORY:
// dbl100824	- created BMC_ AI related classes. using bmai_ai.h for some global references
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "BMC_QAI.h"
#include "BMC_BMAI3.h"


inline BMC_BMAI3  g_ai;

// AI types - for setting up compare games

inline BMC_QAI		g_qai2;
inline BMC_BMAI		g_bmai;
inline BMC_BMAI3	g_bmai3;

// found this one bouncing around somewhere also
// TODO audit g_qai and g_qai2 usage. are both needed?
inline BMC_QAI	g_qai;
