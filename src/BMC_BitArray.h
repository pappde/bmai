///////////////////////////////////////////////////////////////////////////////////////////
// BMC_BitArray.h
//
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp <denis@accessdenied.net>
// SPDX-FileComment: https://github.com/pappde/bmai
//
// REVISION HISTORY:
// dbl100824 - migrated this logic into own class file
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "bmai_lib.h"


// template classes
template <int SIZE>
class BMC_BitArray
{
public:
	static const INT m_bytes;

	// mutators
	void SetAll();
	void Set() { SetAll(); }
	void ClearAll();
	void Clear() { ClearAll(); }
	void Set(int _bit)		{ INT byte = _bit/8; bits[byte] |= (1<<(_bit-byte*8)); }
	void Clear(int _bit)	{ INT byte = _bit/8; bits[byte] &= ~(1<<(_bit-byte*8)); }
	void Set(int _bit, bool _on)	{ if (_on) Set(_bit); else Clear(_bit); }

	// accessors
	bool	IsSet(INT _bit)	{ INT byte = _bit/8; return bits[byte] & (1<<(_bit-byte*8)); }
	bool	operator[](int _bit) { return IsSet(_bit) ? 1 : 0; }

private:
	U8	bits[(SIZE+7)>>3];
};

template<int SIZE>
const INT BMC_BitArray<SIZE>::m_bytes = (SIZE+7)>>3;

template<int SIZE>
void BMC_BitArray<SIZE>::SetAll()
{
	INT i;
	for (i=0; i<m_bytes; i++)
		bits[i] = 0xFF;
}

template<int SIZE>
void BMC_BitArray<SIZE>::ClearAll()
{
	INT i;
	for (i=0; i<m_bytes; i++)
		bits[i] = 0x00;
}