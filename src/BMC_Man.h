///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Man.h
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2021 Denis Papp
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: BMC_Man represents a Button Man (a set of BMC_DieData)
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
// dbl100524 - further split out of individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// TODO_HEADERS: drp030321 - clean up headers
#include "BMC_DieData.h"


class BMC_Man
{
public:
	// accessors
	BMC_DieData *GetDieData(INT _d) { return &m_die[_d]; }

protected:
private:
	BMC_DieData	m_die[BMD_MAX_DICE];
};