///////////////////////////////////////////////////////////////////////////////////////////
// BMC_DieData.h
//
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2021 Denis Papp <denis@accessdenied.net>
// SPDX-FileComment: https://github.com/pappde/bmai
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
// dbl100524 - further split out of individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "bmai_lib.h"


// NOTES:
// - second die only used for Option and Twin
class BMC_DieData
{
    friend class BMC_Parser;

public:
    BMC_DieData();
    virtual void  Reset();

    // methods
    void		Debug(BME_DEBUG _cat = BME_DEBUG_ALWAYS);

    // accessors
    bool		HasProperty(U64 _p) { return m_properties & _p; }
    BME_SWING	GetSwingType(INT _d) { return (BME_SWING)m_swing_type[_d]; }
    bool		Valid() { return (m_properties & BME_PROPERTY_VALID); }
    // drp022521 - fixed to return INT instead of bool
    INT			Dice() { return (m_properties & BME_PROPERTY_TWIN) ? 2 : 1; }
    INT			GetSides(INT _t) { return m_sides[_t]; }

protected:
    U64			m_properties;
    U8			m_sides[BMD_MAX_TWINS];			// number of sides if not a swing die, or current number of sides

private:
    U8			m_swing_type[BMD_MAX_TWINS];	// definition number of sides, should be BME_SWING
};