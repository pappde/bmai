///////////////////////////////////////////////////////////////////////////////////////////
// dieindexstack.h
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: manage a stack of BMC_Die for a given BMC_Player
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "player.h"

// INVARIANT: only contains dice in increasing order (e.g. 0,2,3 but not 2.0,3)				
class BMC_DieIndexStack
{
public:
	BMC_DieIndexStack(BMC_Player *_owner);

	// mutators
	void			Pop() { die_stack_size--; }
	void			Push(INT _index);
	bool			Cycle(bool _add_die = true);

	// methods 
	void			SetBits(BMC_BitArray<BMD_MAX_DICE> & _bits);
	void			Debug(BME_DEBUG _cat = BME_DEBUG_ALWAYS);

	// accessors
	INT				GetValueTotal() { return value_total; }
	bool			ContainsAllDice() { return die_stack_size == owner->GetAvailableDice(); }
	bool			ContainsLastDie() { return GetTopDieIndex() == owner->GetAvailableDice() - 1; }
	INT				GetTopDieIndex() { BM_ASSERT(die_stack_size > 0); return die_stack[die_stack_size - 1]; }
	BMC_Die *		GetTopDie() { return owner->GetDie(GetTopDieIndex()); }
	bool			Empty() { return die_stack_size == 0; }
	INT				GetStackSize() { return die_stack_size; }
	BMC_Die *		GetDie(int _index) { return owner->GetDie(die_stack[_index]); }
	INT				CountDiceWithProperty(BME_PROPERTY _property);

protected:
private:
	BMC_Player *	owner;
	INT				die_stack[BMD_MAX_DICE];
	INT				die_stack_size;
	INT				value_total;
};