///////////////////////////////////////////////////////////////////////////////////////////
// BMC_DieData.cpp
//
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2021 Denis Papp <denis@accessdenied.net>
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
// dbl100524 - broke this logic out into its own class file
///////////////////////////////////////////////////////////////////////////////////////////

#include "BMC_DieData.h"

#include "BMC_Logger.h"


BMC_DieData::BMC_DieData()
{
  Reset();
}

void BMC_DieData::Reset()
{
  m_properties = 0;	// note, VALID bit not on!
  INT i;
  for (i=0; i<BMD_MAX_TWINS; i++)
  {
    m_sides[i] = 0;
    m_swing_type[i] = BME_SWING_NOT;
  }
}

void BMC_DieData::Debug(BME_DEBUG _cat)
{
  BMF_Log(_cat,"(%lx)", m_properties & (~BME_PROPERTY_VALID));
}
