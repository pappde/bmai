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