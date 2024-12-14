///////////////////////////////////////////////////////////////////////////////////////////
// BMC_DieIndexStack.cpp
//
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp <denis@accessdenied.net>
//
// REVISION HISTORY:
// dbl100524 - broke this logic out into its own class file
// dbl100524 - broke this logic out into its own class file
///////////////////////////////////////////////////////////////////////////////////////////

#include "BMC_DieIndexStack.h"

#include <cstdio>
#include "BMC_Logger.h"


BMC_DieIndexStack::BMC_DieIndexStack(BMC_Player *_owner)
{
  owner = _owner;
  die_stack_size = 0;
  value_total = 0;
}

void BMC_DieIndexStack::Debug(BME_DEBUG _cat)
{
  printf("STACK: ");
  for (INT i=0; i<die_stack_size; i++)
    printf("%d ", die_stack[i]);
  printf("\n");
}

void BMC_DieIndexStack::SetBits(BMC_BitArray<BMD_MAX_DICE> & _bits)
{
  _bits.Clear();
  INT i;
  for (i=0; i<die_stack_size; i++)
    _bits.Set(die_stack[i]);
}

// DESC: add the next die to the stack.  If we can't, then remove the top die in the stack
//       and replace it with the next available die
// PARAM: _add_die: if specified, then force cycling the top die
// RETURNS: if finished (couldn't cycle)
bool BMC_DieIndexStack::Cycle(bool _add_die)
{
  // if at end... (stack top == last die)
  if (ContainsLastDie())
  {
    Pop();	// always pop since cant cycle last die

    // if empty - give up
    if (Empty())
      return true;

    _add_die = false;	// cycle the previous die
  }

  if (_add_die)
  {
    Push(GetTopDieIndex() + 1);
  }
  else
  {
    // cycle top die
    BM_ASSERT(!ContainsLastDie());

    value_total -= GetTopDie()->GetValueTotal();
    die_stack[die_stack_size-1]++;
    value_total += GetTopDie()->GetValueTotal();
  }

  return false;	// not finished
}

void BMC_DieIndexStack::Pop()
{
  value_total -= GetTopDie()->GetValueTotal();
  die_stack_size--;
}

void BMC_DieIndexStack::Push(INT _index)
{
  die_stack[die_stack_size++] = _index;
  value_total += GetTopDie()->GetValueTotal();
}

INT BMC_DieIndexStack::CountDiceWithProperty(BME_PROPERTY _property)
{
  INT count = 0;
  INT i;
  for (i=0; i<die_stack_size; i++)
    if (owner->GetDie(die_stack[i])->HasProperty(_property))
      count++;
  return count;
}