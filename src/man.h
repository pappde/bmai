///////////////////////////////////////////////////////////////////////////////////////////
// man.h
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: BMC_Man represents a Button Man (a set of BMC_DieData)
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// TODO_HEADERS: drp030321 - clean up headers
#include "bmai.h"
#include "die.h"

class BMC_Man
{
public:
	// accessors
	BMC_DieData *GetDieData(INT _d) { return &m_die[_d]; }

protected:
private:
	BMC_DieData	m_die[BMD_MAX_DICE];
};