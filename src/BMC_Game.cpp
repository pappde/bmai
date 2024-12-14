///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Game.cpp
//
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2024 Denis Papp <denis@accessdenied.net>
// SPDX-FileComment: https://github.com/pappde/bmai
// 
// DESC: main game logic class
//
// REVISION HISTORY:
// dbl100524 - broke this logic out into its own class file
///////////////////////////////////////////////////////////////////////////////////////////

#include "BMC_Game.h"

#include <cstring>
#include "BMC_AI.h"
#include "BMC_BMAI3.h"
#include "BMC_DieIndexStack.h"
#include "BMC_Logger.h"


BMC_Game::BMC_Game(bool _simulation)
{
	INT i;

	for (i=0; i<BMD_MAX_PLAYERS; i++)
	{
		m_ai[i] = NULL;
		m_player[i].SetID(i);
	}

	m_phase = BME_PHASE_GAMEOVER;
	m_target_wins = BMD_DEFAULT_WINS;
	m_simulation = _simulation;
	m_last_action = BME_ACTION_MAX;
	m_surrender_allowed = true;
}

BMC_Game::BMC_Game(const BMC_Game & _game)
{
	bool save_simulation = m_simulation;
	*this = _game;
	m_simulation = save_simulation;
}

const BMC_Game & BMC_Game::operator=(const BMC_Game & _game)
{
	bool save_simulation = m_simulation;
	std::memcpy(this, &_game, sizeof(BMC_Game));
	m_simulation = save_simulation;
	return *this;
}

void BMC_Game::Setup(BMC_Man *_man1, BMC_Man *_man2)
{
	if (_man1)
		m_player[0].SetButtonMan(_man1);

	if (_man2)
		m_player[1].SetButtonMan(_man2);

	INT i;
	for (i=0; i<BME_WLT_MAX; i++)
		m_standing[i] = 0;

	m_phase = BME_PHASE_PREROUND;
	m_last_action = BME_ACTION_MAX;
}

// PRE: dice are optimized (largest to smallest)
// RETURNS: the player who has initiative (or -1 in a tie, so the calling function can determine how to break ties)
INT BMC_Game::CheckInitiative()
{
	BM_ASSERT(BMD_MAX_PLAYERS==2);

	INT i,j;
	INT delta;

	i = m_player[0].GetAvailableDice() - 1;
	j = m_player[1].GetAvailableDice() - 1;
	BM_ASSERT(i>=0 && j>=0);

	while (1)
	{
		// TRIP or SLOW or STINGER dice don't count for initiative
		while (m_player[0].GetDie(i)->HasProperty(BME_PROPERTY_TRIP|BME_PROPERTY_SLOW|BME_PROPERTY_STINGER) && i>=0)
			i--;
		while (m_player[1].GetDie(j)->HasProperty(BME_PROPERTY_TRIP|BME_PROPERTY_SLOW|BME_PROPERTY_STINGER) && j>=0)
			j--;

		// if no dice remaining - is a tie
		if (i<0 && j<0)
			return -1;

		// drp070701 - in case of a tie, advantage goes to player with more dice
		if (i<0)
			return 1;
		if (j<0)
			return 0;

		// check current lowest dice of each player
		delta = m_player[0].GetDie(i)->GetValueTotal() - m_player[1].GetDie(j)->GetValueTotal();
		if (delta==0)
		{
			// these dice are tied - go to next dice (first few rules will catch if a player has run out of dice)
			i--;
			j--;
		}
		else
		{
			return (delta > 0) ? 1 : 0;
		}
	}
}

// PRE: phase is preround
// POST:
//   - phase is initiative
//   - dice have been rolled
//   - m_phase_player and m_target_player have been setup
void BMC_Game::FinishPreround()
{
	INT i;

	BM_ASSERT(m_phase == BME_PHASE_PREROUND);

	m_phase = BME_PHASE_INITIATIVE;

	// roll die
	for (i=0; i<BMD_MAX_PLAYERS; i++)
		m_player[i].RollDice();

	// determine initiative
	INT initiative = CheckInitiative();

	// check for a TIE - TODO: handle how? currently given to 0
	if (initiative < 0)
		m_phase_player = 0;
	else
		m_phase_player = initiative;

	m_initiative_winner = m_phase_player;
	m_target_player = (m_phase_player==0) ? 1 : 0;
	m_last_action = BME_ACTION_MAX;

	g_logger.Log(BME_DEBUG_ROUND, "initiative p%d\n", m_initiative_winner);
}

// PRE: phase has already been set to FIGHT
void BMC_Game::FinishInitiative()
{
	BM_ASSERT(m_phase == BME_PHASE_FIGHT);
}

// PARAM: was it a win, loss, or tie for player 0?
// PRE: phase is TURN, assumes 2 players
// POST:
//   - phase is PREROUND or GAMEOVER
//   - loser swing is reset
BME_WLT BMC_Game::FinishRound(BME_WLT _wlt_0)
{
	INT i;

	BM_ASSERT(m_phase == BME_PHASE_FIGHT);

	BMF_Log(BME_DEBUG_ROUND, "game over %.1f - %.1f winner %d\n", m_player[0].GetScore(), m_player[1].GetScore(), _wlt_0);

	// update the standings
	m_standing[_wlt_0]++;

	// reset the loser's swing dice
	if (_wlt_0 != BME_WLT_TIE)
	{
		m_player[!_wlt_0].OnRoundLost();
	}

	// check if the game is over
	for (i=0; i<BMD_MAX_PLAYERS; i++)
	{
		if (m_standing[i]>=m_target_wins)
		{
			m_phase = BME_PHASE_GAMEOVER;
			return _wlt_0;
		}
	}

	// move back to PREROUND
	m_phase = BME_PHASE_PREROUND;

	return _wlt_0;
}

// DESC: a chance move is valid (for the non-initiative player) as long as the selected die is a CHANCE die
// PRE: the acting player is m_phase_player
bool BMC_Game::ValidUseChance(BMC_Move &_move)
{
	INT i;
	for (i=0; i<m_player[m_phase_player].GetAvailableDice(); i++)
	{
		if (_move.m_chance_reroll.IsSet(i) && !m_player[m_phase_player].GetDie(i)->HasProperty(BME_PROPERTY_CHANCE))
			return false;
	}

	return true;
}

// DESC: a focus move is valid so long as the selected dice are FOCUS, are being reduced in value, and gains the initiative
bool BMC_Game::ValidUseFocus(BMC_Move &_move)
{
	INT i;

	BMC_Player temp = m_player[m_phase_player];

	// error checking and update focus dice
	BMC_Die *d;
	for (i=0; i<m_player[m_phase_player].GetAvailableDice(); i++)
	{
		if (_move.m_focus_value[i]>0)
		{
			d = m_player[m_phase_player].GetDie(i);
			BM_ASSERT(d->HasProperty(BME_PROPERTY_FOCUS));
			BM_ASSERT(d->GetValueTotal()>_move.m_focus_value[i]);
			d->CheatSetValueTotal(_move.m_focus_value[i]);
		}
	}

	// reoptimize dice (for check initiative)
	m_player[m_phase_player].OnFocusDieUsed();

	// check initiative
	INT init = CheckInitiative();

	m_player[m_phase_player] = temp;

	// focus is only valid if gained initiative
	return (init == m_phase_player);
}

bool BMC_Game::ValidSetSwing(BMC_Move &_move)
{
	INT i;

	// check swing dice
	for (i=0; i<BME_SWING_MAX; i++)
	{
		if (m_player[m_phase_player].GetTotalSwingDice(i)>0 && c_swing_sides_range[i][0]>0)
		{
			// check range
			if (_move.m_swing_value[i]<c_swing_sides_range[i][0] || _move.m_swing_value[i]>c_swing_sides_range[i][1])
				return false;
		}
	}

	// check option and unique
	// drp071505 - OPTION check was wrong (not checking any dice, but probably didn't matter)
	// drp071505 - UNIQUE check was wrong (was not enforced)
	BMC_Die *d;
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		d = m_player[m_phase_player].GetDie(i);
		if (!d->IsUsed())
			continue;

		/* REMOVED: drp060323 - m_option_die[] is now boolean, so check no longer relevant.
		if (d->HasProperty(BME_PROPERTY_OPTION))
		{
			if (_move.m_option_die[i]>1)
				return false;
		}
		*/

		// check UNIQUE:  we check if current swing value>0 because some AIs use this function before having set all swing dice
		// NOTE: does not work for twin
		if (d->HasProperty(BME_PROPERTY_UNIQUE))
		{
			INT swing_type = d->GetSwingType(0);
			BM_ASSERT(swing_type != BME_SWING_NOT);
			BM_ASSERT(_move.m_swing_value[swing_type]>=c_swing_sides_range[swing_type][0] && _move.m_swing_value[swing_type]<=c_swing_sides_range[swing_type][1]);
			for (INT s=0; s<swing_type; s++)
			{
				if (	m_player[m_phase_player].GetTotalSwingDice(s)>0
					&&	c_swing_sides_range[s][0]>0
					&&  _move.m_swing_value[s]>0
					&&	_move.m_swing_value[s]==_move.m_swing_value[swing_type])
				{
					return false;
				}
			}
		}
	}

	return true;
}

bool BMC_Game::ValidAttack(BMC_MoveAttack &_move)
{
	BMC_Die *att_die, *tgt_die;
	BMC_Player *attacker = &(m_player[_move.m_attacker_player]);
	BMC_Player *target = &(m_player[_move.m_target_player]);
	switch (_move.m_attack)
	{
	case BME_ATTACK_POWER:	// 1-> 1
	case BME_ATTACK_TRIP:
	case BME_ATTACK_SHADOW:
		{
			// is the attack type legal based on the given dice
			att_die = attacker->GetDie(_move.m_attacker);
			if (!att_die->CanDoAttack(_move))
				return false;

			tgt_die = target->GetDie(_move.m_target);
			if (!tgt_die->CanBeAttacked(_move))
				return false;

			// for POWER - check value >=
			if (_move.m_attack == BME_ATTACK_POWER)
			{
				return att_die->GetValueTotal() >= tgt_die->GetValueTotal();
			}

			// for SHADOW - check value <= and total
			else if (_move.m_attack == BME_ATTACK_SHADOW)
			{
				return att_die->GetValueTotal() <= tgt_die->GetValueTotal()
					&& att_die->GetSidesMax() >= tgt_die->GetValueTotal();
			}

			// for TRIP - always legal, except when can't capture (one die TRIP against two die)
			else
			{
				if (att_die->Dice()==1 && tgt_die->Dice()>1)
					return false;
				return true;
			}
		}

	case BME_ATTACK_SPEED:	// 1 -> N
	case BME_ATTACK_BERSERK:
		{
			// is the attack type legal based on the given dice
			att_die = attacker->GetDie(_move.m_attacker);
			if (!att_die->CanDoAttack(_move))
				return false;

			// iterate over target dice
			INT	tgt_value_total = 0;
			INT i;
			INT dice = 0;
			for (i=0; i<BMD_MAX_DICE; i++)
			{
				if (_move.m_targets.IsSet(i))
				{
					dice++;
					// can the target die be attacked?
					tgt_die = target->GetDie(i);
					if (!tgt_die->CanBeAttacked(_move))
						return false;
					// count value of target die, and check if gone past limit
					tgt_value_total += tgt_die->GetValueTotal();
					if (tgt_value_total > att_die->GetValueTotal())
						return false;
				}
			}

			// must be capturing more than one
			/* removed - this is legal
			if (dice<2)
				return false;
			*/

			// if match - success
			if (tgt_value_total == att_die->GetValueTotal())
				return true;

			return false;
		}

	case BME_ATTACK_SKILL:	// N -> 1
		{
			// TODO: KONSTANT allow + or -
				// 41 k2 k3
				// 5+2+3 10
				// 5+2-3 4
				// 5-2+3 6
				// 5-2-3 0

			// WARRIOR: can only have one involved
			INT warriors = 0;

			// can the target die be attacked?
			tgt_die = target->GetDie(_move.m_target);
			if (!tgt_die->CanBeAttacked(_move))
				return false;

			// iterate over attack dice
			INT	att_value_total = 0;
			INT i;
			INT dice = 0;
			bool has_stinger = attacker->HasDieWithProperty(BME_PROPERTY_STINGER);
			INT stinger_att_value_minimum = 0;
			INT konstants = 0;
			for (i=0; i<BMD_MAX_DICE; i++)
			{
				if (_move.m_attackers.IsSet(i))
				{
					dice++;
					// is the attack type legal based on the given dice
					att_die = attacker->GetDie(i);
					if (!att_die->CanDoAttack(_move))
						return false;

					// count value of att die, and check if gone past limit
					att_value_total += att_die->GetValueTotal();
					if (!has_stinger && att_value_total > tgt_die->GetValueTotal())
						return false;

					if (att_die->HasProperty(BME_PROPERTY_WARRIOR))
					{
						if (warriors>=1)
							return false;
						warriors++;
					}

					if (att_die->HasProperty(BME_PROPERTY_KONSTANT))
						konstants++;

					if (att_die->HasProperty(BME_PROPERTY_STINGER))
						stinger_att_value_minimum += 1;
					else
						stinger_att_value_minimum += att_die->GetValueTotal();
				}
			}

			// must be using more than one
			// TODO: should allow this (important for FIRE)
			if (dice<2)
				return false;

			// KONSTANT: cannot do with just one die
			if (dice<2 && konstants>0)
				return false;

			// if match - success
			if (att_value_total == tgt_die->GetValueTotal())
				return true;

			// stinger - if within range - success
			if (has_stinger && tgt_die->GetValueTotal() >= stinger_att_value_minimum && tgt_die->GetValueTotal() <= att_value_total)
				return true;

			return false;
		}

	default:
		BM_ASSERT(0);
		return false;
	}
}

void BMC_Game::PlayPreround()
{
	INT i;
	BMC_Move move;

	// check if either player has SWING/OPTION dice to set
	for (i=0; i<BMD_MAX_PLAYERS; i++)
	{
		if (m_player[i].GetSwingDiceSet()==BMC_Player::SWING_SET_NOT)
		{
			// to accurately model simultaneous swing actions (where you aren't supposed to know your opponent's
			// swing action), if the opponent is SWING_SET_READY then temporarily mark him SWING_SET_NOT
			int opp = !i;
			bool opp_was_ready = false;
			if (m_player[opp].GetSwingDiceSet()==BMC_Player::SWING_SET_READY)
			{
				m_player[opp].SetSwingDiceStatus(BMC_Player::SWING_SET_NOT);
				opp_was_ready = true;
			}

			m_phase_player = i;	// setup phase player for the AI

			if (m_player[i].NeedsSetSwing())
				m_ai[i]->GetSetSwingAction(this, move);
			else
				move.m_action = BME_ACTION_PASS;

			move.m_game = this;	// ensure m_game is correct
			if (!ValidSetSwing(move))
			{
				BMF_Error("Player %d using illegal set swing move\n", i);
				return;
			}
            ApplySetSwing(move);

			if (opp_was_ready)
				m_player[opp].SetSwingDiceStatus(BMC_Player::SWING_SET_READY);
		}
	}
}

// DESC: CHANCE - player who does not have initiative has option to reroll one chance die. If takes initiative, give
// opportunity to other player
// PRE: m_target_player is the player who does not have initiative
void BMC_Game::PlayInitiativeChance()
{
	BM_ASSERT(m_phase == BME_PHASE_INITIATIVE_CHANCE);

	// does the non-initiative player have any CHANCE dice?
	while (m_player[m_target_player].HasDieWithProperty(BME_PROPERTY_CHANCE))
	{
		// to make things more consistent, we temporarily swap phase/target (phasing player is the acting player).
		// This is something that ApplyUseChance() expects - and will always correct.
		m_phase_player = !m_phase_player;
		m_target_player = !m_target_player;

		BMC_Move	move;
		m_ai[m_phase_player]->GetUseChanceAction(this, move);

		ApplyUseChance(move);

		// POST: m_phase_player is back to the player with initiative

		// check if the chance move failed (in which case m_phase will have changed).  In this case, simply return, since
		// Finish() has already been called.
		if (m_phase!=BME_PHASE_INITIATIVE_CHANCE)
			return;
	}

	FinishInitiativeChance(false);
}

// PARAM: if ApplyUseChance() was used, then m_phase_player will be the player without initiative
void BMC_Game::FinishInitiativeChance(bool _swap_phase_player)
{
	BM_ASSERT(m_phase == BME_PHASE_INITIATIVE_CHANCE);

	if (_swap_phase_player)
	{
		m_phase_player = !m_phase_player;
		m_target_player = !m_target_player;
	}

	m_phase = BME_PHASE_INITIATIVE_FOCUS;
}

// DESC: FOCUS - player who does not have the initiative has the option to reduce the value of multiple focus dice,
// provided it results in gaining initiative (no ties)
// PRE: m_target_player is the player who does not have initiative
void BMC_Game::PlayInitiativeFocus()
{
	BM_ASSERT(m_phase == BME_PHASE_INITIATIVE_FOCUS);

	// does the non-initiative player have any FOCUS dice?
	while (m_player[m_target_player].HasDieWithProperty(BME_PROPERTY_FOCUS))
	{
		// to make things more consistent, we temporarily swap phase/target (phasing player is the acting player).
		// This is something that ApplyUseFocus() expects - and will always correct.
		m_phase_player = !m_phase_player;
		m_target_player = !m_target_player;

		BMC_Move	move;
		m_ai[m_phase_player]->GetUseFocusAction(this,move);

		ApplyUseFocus(move);

		// POST: m_phase_player is back to the player with initiative

		// check if FOCUS was not used (in which case m_phase will have changed).  Simply return, since Finish() already called
		if (m_phase!=BME_PHASE_INITIATIVE_FOCUS)
			return;
	}

	FinishInitiativeFocus(false);
}

// DESC: if ApplyUseFocus() was used, then m_phase_player will be the player without initiative
void BMC_Game::FinishInitiativeFocus(bool _swap_phase_player)
{
	BM_ASSERT(m_phase == BME_PHASE_INITIATIVE_FOCUS);

	if (_swap_phase_player)
	{
		m_phase_player = !m_phase_player;
		m_target_player = !m_target_player;
	}

	m_phase = BME_PHASE_FIGHT;
}

// PRE: dice rolled and initiative determined
void BMC_Game::PlayInitiative()
{
	if (m_phase==BME_PHASE_INITIATIVE)
	{
		m_phase = BME_PHASE_INITIATIVE_CHANCE;
	}

	if (m_phase == BME_PHASE_INITIATIVE_CHANCE)
	{
		PlayInitiativeChance();
	}

	if (m_phase == BME_PHASE_INITIATIVE_FOCUS)
	{
		PlayInitiativeFocus();
	}
}

bool BMC_Game::FightOver()
{
	if (m_player[m_phase_player].GetAvailableDice()<1)
		return true;
	if (m_player[m_target_player].GetAvailableDice()<1)
		return true;
	return false;
}

// PARAM: extra_turn is true if the phasing player should go again
// POST: updates m_phase_player and m_target_player
// ASSUME: only two players
void BMC_Game::FinishTurn(bool extra_turn)
{
	if (extra_turn)
		return;

	m_phase_player = !m_phase_player;
	m_target_player = !m_target_player;
	BM_ASSERT(m_phase_player != m_target_player);
}

// RETURNS: 0/0.5/1 from m_phase_player point of view
float BMC_Game::ConvertWLTToWinProbability()
{
	if (m_player[m_phase_player].GetScore() > m_player[m_target_player].GetScore())
		return 1;
	else if (m_player[m_target_player].GetScore() > m_player[m_phase_player].GetScore())
		return 0;
	else
		return 0.5;
}

BME_WLT BMC_Game::PlayRound(BMC_Move *_start_action)
{
	if (_start_action == NULL)
	{
		if (m_phase==BME_PHASE_PREROUND)
		{
			PlayPreround();
			FinishPreround();
		}

		PlayInitiative();

		FinishInitiative();
	}

	PlayFight(_start_action);

	// finish
	// ASSUME: two players
	if (m_player[0].GetScore() > m_player[1].GetScore())
		return FinishRound(BME_WLT_WIN);
	else if (m_player[1].GetScore() > m_player[0].GetScore())
		return FinishRound(BME_WLT_LOSS);
	else
		return FinishRound(BME_WLT_TIE);
}

void BMC_Game::PlayGame(BMC_Man *_man1, BMC_Man *_man2)
{
	Setup(_man1, _man2);

	while (m_phase != BME_PHASE_GAMEOVER)
	{
		BME_WLT wlt;

		wlt = PlayRound();

		// if not gameover, do RESERVE for loser
		if (m_phase != BME_PHASE_GAMEOVER)
		{
			BMC_Move	move;
			INT			loser = -1;
			if (wlt == BME_WLT_WIN)
				loser = 1;
			else if (wlt == BME_WLT_LOSS)
				loser = 0;

			// only do reserve if was not a tie, and loser has reserve dice
			if (loser>=0 && m_player[loser].HasDieWithProperty(BME_PROPERTY_RESERVE,true))
			{
				m_phase = BME_PHASE_RESERVE;
				m_ai[loser]->GetReserveAction(this, move);
				ApplyUseReserve(move);
			}
		}
	}

	BMF_Log(BME_DEBUG_GAME, "game over %d - %d - %d\n", m_standing[0], m_standing[1], m_standing[2]);
}

// PRE: m_phase_player is set as the player who has the opportunity to use chance
void BMC_Game::GenerateValidChance(BMC_MoveList & _movelist)
{
	BM_ASSERT(m_phase == BME_PHASE_INITIATIVE_CHANCE);

	BMC_Player *p = &(m_player[m_phase_player]);
	BMC_Move	move;
	INT			i,j;

	_movelist.Clear();

	move.m_game = this;

	// now iterate over each possible chance action
	INT num_chance_dice = 0;
	INT chance_die_index[BMD_MAX_DICE];
	INT combinations = 1;
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!p->GetDie(i)->IsUsed())
			continue;
		if (!p->GetDie(i)->HasProperty(BME_PROPERTY_CHANCE))
			continue;
		chance_die_index[num_chance_dice++] = i;
		combinations *= 2;
	}

	BM_ASSERT(num_chance_dice>0);


	// always leave PASS
	move.m_action = BME_ACTION_PASS;
	_movelist.Add(move);

	// iterate over each action (i represents the 'bits' of which chance dice to reroll)
	for (i=1; i<combinations; i++)
	{
		move.m_action = BME_ACTION_USE_CHANCE;
		move.m_chance_reroll.Clear();

		for (j=0; j<num_chance_dice; j++)
		{
			if (i & (1<<j))
				move.m_chance_reroll.Set(chance_die_index[j]);
		}

		BM_ASSERT(ValidUseChance(move));

		_movelist.Add(move);
	}
}

// PRE: m_phase_player is set as the player who has the opportunity to use focus
void BMC_Game::GenerateValidFocus(BMC_MoveList & _movelist)
{
	BM_ASSERT(m_phase == BME_PHASE_INITIATIVE_FOCUS);

	BMC_Player *phaser = &(m_player[m_phase_player]);
	BMC_Move	move;
	INT			i,c;

	_movelist.Clear();

	move.m_game = this;

	// always leave PASS
	move.m_action = BME_ACTION_PASS;
	_movelist.Add(move);

	// now iterate over each possible focus action

	// update unchanging data
	move.m_action = BME_ACTION_USE_FOCUS;
	for (i=0; i<BMD_MAX_DICE; i++)
		move.m_focus_value[i] = 0;

	// n = number of focus dice (i = 0..n-1)
	// S[i] = current value of die 'i'
	// V[i] = desired value of die 'i' (dizzied)
	// M = S1 * ... * Sn (total combinations)
	// index[i] = the focus die's index
	INT n = 0;
	INT S[BMD_MAX_DICE];
	INT index[BMD_MAX_DICE];
	INT M = 1;

	// now compute n, S[], and M
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		BMC_Die *d = phaser->GetDie(i);
		if (!d->IsUsed())
			continue;
		if (!d->HasProperty(BME_PROPERTY_FOCUS))
			continue;
		// OPT: ignore value of 1
		if (d->GetValueTotal()<=1)
			continue;
		S[n] = d->GetValueTotal();
		index[n] = i;
		M *= S[n++];
	}

	// iterate the combinations.  We ignore 'M-1' since that is the same as PASS which has
	// already been included in the list
	for (c=0; c<M-1; c++)
	{
		// V[i] = (c/D % S[i]) + 1, where D = 1 * S[1] * ... S[i-1]
		INT D = 1;
		for (i=0; i<n; i++)	// iterate over the focus dice
		{
			INT idx = index[i];
			INT val = ((c / D) % S[i]) + 1;
			move.m_focus_value[idx] = (val>=S[i]) ? 0 : val;	// if not reducing sides, set 0
			D *= S[i];											// update the divisor for the next die
		}

		// is this a valid focus attack
		// TODO: room for early aborts somewhere here (optimization) but would need to alter walk through combinations
		if (ValidUseFocus(move))
			_movelist.Add(move);
	}
}

void BMC_Game::GenerateValidSetSwing(BMC_MoveList & _movelist)
{
	BM_ASSERT(m_phase == BME_PHASE_PREROUND);

	BMC_Player *pl = GetPhasePlayer();
	BMC_Move	move;

	// array of valid values
	struct SWING_ACTION {
		bool		swing;	// or option
		INT			index;	// swing type or die
		INT			value;	// from [min..max] (or [0..1] for option)
	};

	SWING_ACTION	swing_action[BME_SWING_MAX + BMD_MAX_DICE];
	INT				actions = 0;
	//INT				combinations = 1;
	INT i, p;

	move.m_game = this;
	move.m_action = BME_ACTION_SET_SWING_AND_OPTION;

	// walk dice and determine possible things to set
	for (i=0; i<BME_SWING_MAX; i++)
	{
		if (pl->GetTotalSwingDice(i)>0 && c_swing_sides_range[i][0]>0)
		{
			swing_action[actions].swing = true;
			swing_action[actions].index = i;
			swing_action[actions].value = c_swing_sides_range[i][0];	// initialize to min
			actions++;
			//combinations *= (g_swing_sides_range[i][1]-g_swing_sides_range[i][0])+1;
			move.m_swing_value[i] = c_swing_sides_range[i][0];
		}
	}

	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!pl->GetDie(i)->IsUsed())
			continue;

		if (pl->GetDie(i)->HasProperty(BME_PROPERTY_OPTION))
		{
			swing_action[actions].swing = false;
			swing_action[actions].index = i;
			swing_action[actions].value = 0;	// initialize to first die
			actions++;
			//combinations *= 2;
			move.m_option_die.Set(i,0);
		}
	}

	if (actions==0)
	{
		move.m_action = BME_ACTION_PASS;
		_movelist.Add(move);
		return;
	}

	// now iterate over all combinations of actions
	do
	{
		// only add case if valid (UNIQUE check)
		if (ValidSetSwing(move))
			_movelist.Add(move);

		// increment step
		BM_ASSERT(actions>=0);
		p = actions-1;
		while (1)
		{
			swing_action[p].value++;

			// update move
			if (swing_action[p].swing)
				move.m_swing_value[swing_action[p].index]++;
			else
				move.m_option_die.Set(swing_action[p].index, 1);

			// past max?
			INT max;
			if (swing_action[p].swing)
				max = c_swing_sides_range[swing_action[p].index][1];
			else
				max = 1;
			if (swing_action[p].value <= max)
				break;

			// we are past max - reset to min and cycle the next value
			if (swing_action[p].swing)
			{
				swing_action[p].value = c_swing_sides_range[swing_action[p].index][0];
				move.m_swing_value[swing_action[p].index] = swing_action[p].value;
			}
			else
			{
				swing_action[p].value = 0;
				move.m_option_die.Set(swing_action[p].index, 0);
			}

			p--;

			// finished?
			if (p<0)
				break;
		} // end while(1) - increment step
	} while (p>=0);

	// if actions found, generate a pass move
	if (_movelist.Empty())
	{
		move.m_action = BME_ACTION_PASS;
		_movelist.Add(move);
	}
}


// PRE: this is the TURN phaes, where we are doing ATTACK actions
// POST: movelist contains at least one move
void BMC_Game::GenerateValidAttacks(BMC_MoveList & _movelist)
{
	BM_ASSERT(m_phase == BME_PHASE_FIGHT);

	BMC_Player *attacker = &(m_player[m_phase_player]);
	BMC_Player *target = &(m_player[m_target_player]);
	BMC_MoveAttack	move;

	_movelist.Clear();

	move.m_action = BME_ACTION_ATTACK;
	move.m_game = this;
	move.m_attacker_player = m_phase_player;
	move.m_target_player = m_target_player;

	// for each die, for each attack, for each target
	INT		a;
	BMC_Die *att_die,*tgt_die;
	for (move.m_attacker=0; move.m_attacker<attacker->GetAvailableDice(); move.m_attacker++)
	{
		att_die = attacker->GetDie(move.m_attacker);
		BM_ASSERT(att_die->IsAvailable());

		for (a=BME_ATTACK_FIRST; a<BME_ATTACK_MAX; a++)
		{
			move.m_attack = (BME_ATTACK)a;
			if (!att_die->CanDoAttack(move))
				continue;

			move.m_turbo_option = -1;

			switch (c_attack_type[move.m_attack])
			{
			case BME_ATTACK_TYPE_1_1:
				{
					// always walk targets smallest to largest - easier to cut walk
					for (move.m_target=target->GetAvailableDice()-1; move.m_target>=0; move.m_target--)
					{
						tgt_die = target->GetDie(move.m_target);

						// if past size restriction, break loop (note TRIP has no restriction)
						// SHADOW: give up once walked past our #sides
						if (move.m_attack==BME_ATTACK_SHADOW && tgt_die->GetValueTotal()>att_die->GetSidesMax())
							break;
						// POWER: give up once walked past our value
						else if (move.m_attack==BME_ATTACK_POWER && tgt_die->GetValueTotal()>att_die->GetValueTotal())
							break;

						if (!tgt_die->CanBeAttacked(move))
							continue;

						// is the move valid?
						if (ValidAttack(move))
							_movelist.Add(move);
					}
					break;
				}

			case BME_ATTACK_TYPE_N_1:
				{
					// TODO: KONSTANT: consider all combinations, don't break early due to total
					// OPTIMIZATION: KONSTANT optimization - add one negative version of each constant die to the front
					//  of the stack.

					BMC_DieIndexStack	die_stack(attacker);
					bool finished = false;
					bool has_stinger = attacker->HasDieWithProperty(BME_PROPERTY_STINGER);
					bool has_konstant = attacker->HasDieWithProperty(BME_PROPERTY_KONSTANT);

					// add the first die (this one)
					die_stack.Push(move.m_attacker);

					while (!finished)
					{
						//die_stack.Debug(BME_DEBUG_ALWAYS);

						// STINGER: if there are any stinger dice in the stack it gives us flexibility.
						// The range is [non_stinger_total+stinger_dice, total]
						if (has_stinger && die_stack.GetStackSize()>1)
						{
							INT i;
							INT minimum_value = 0;
							for (i=0; i<die_stack.GetStackSize(); i++)
							{
								BMC_Die *die = die_stack.GetDie(i);
								if (die->HasProperty(BME_PROPERTY_STINGER))
									minimum_value ++;
								else
									minimum_value += die->GetValueTotal();
							}

							// check all target dice
							for (move.m_target=0; move.m_target<target->GetAvailableDice(); move.m_target++)
							{
								tgt_die = target->GetDie(move.m_target);

								// if below our range, give up
								if (tgt_die->GetValueTotal() < minimum_value)
									break;

								// if within our range, check move
								if (tgt_die->GetValueTotal() <= die_stack.GetValueTotal())
								{
									// build m_attackers to check move validity
									die_stack.SetBits(move.m_attackers);
									if (ValidAttack(move))
										_movelist.Add(move);
								}
							}
						}

						// NON-STINGER: must match exactly
						else
						{
							// check all target dice
							for (move.m_target=0; move.m_target<target->GetAvailableDice(); move.m_target++)
							{
								tgt_die = target->GetDie(move.m_target);

								// if past our value, give up
								if (tgt_die->GetValueTotal() < die_stack.GetValueTotal())
									break;

								// if match our value, check move
								if (tgt_die->GetValueTotal() == die_stack.GetValueTotal())
								{
									// build m_attackers to check move validity
									die_stack.SetBits(move.m_attackers);
									if (ValidAttack(move))
										_movelist.Add(move);
								}
							}
						}

						// if full (using all target dice) and att value is <= tgt total value, give up since won't be able to do any other matches
						// drp100224 - this check was wrong. We can abort if GetValueTotal() <= target->GetMinValue(), since that's the highest we can combine to,
						//  but otherwise we should keep cycling since other combniations will have lower totals.
						if (die_stack.ContainsAllDice() && die_stack.GetValueTotal()<=target->GetMinValue())
							break;

						// if att_total matches or exceeds tgt_total, don't add a die
						if (die_stack.GetValueTotal() >= target->GetMaxValue())
							finished = die_stack.Cycle(false);
						else // otherwise standard cycle
							finished = die_stack.Cycle();

						// stop when cycled first die
						if (die_stack.GetStackSize()==1)
							break;

					} // end while(!finished)

					break;
				}


			case BME_ATTACK_TYPE_1_N:
				{
					BMC_DieIndexStack	die_stack(target);
					INT att_total = att_die->GetValueTotal();
					bool finished = false;

					// add the first die
					die_stack.Push(0);

					while (!finished)
					{
						// check move if at target value
						if (att_total == die_stack.GetValueTotal())
						{
							// build m_targets to check move validity
							die_stack.SetBits(move.m_targets);
							if (ValidAttack(move))
								_movelist.Add(move);
						}

						// step

						// if full (using all target dice) and tgt tot value is <= att value, give up since won't be able to do any other matches
						if (die_stack.ContainsAllDice() && att_total >= die_stack.GetValueTotal())
							break;

						// if tgt_total matches or exceeds att_total, don't add a die (no sense continuing on this line)
						// Otherwise do a standard cycle
						if (att_total <= die_stack.GetValueTotal())
							finished = die_stack.Cycle(false);
						else
							finished = die_stack.Cycle();

					} // end while(!finished)

					break;
				}

			default:
				BM_ASSERT(0);
				break;
			}
		}
	}

	// if have a TURBO die, then double all moves where attacking with it.
	// drp071305 - fix memory trasher. We were getting a ptr to a move in the list and
	//  using that as a workspace for adding new moves.  But it's a vector and memory can
	//  move. The workspace for new moves should be a local on the stack.
	INT turbo_die = attacker->HasDieWithProperty(BME_PROPERTY_TURBO);
	if (turbo_die>0)
	{
		turbo_die--;	// HasDieWithProperty() returns index+1
		INT moves = _movelist.Size();
		INT m;
		BMC_MoveAttack *attack;
		for (m=0; m<moves; m++)
		{
			attack = _movelist.Get(m);

			// does move involve a turbo die?
			switch (c_attack_type[attack->m_attack])
			{
				case BME_ATTACK_TYPE_1_1:
				case BME_ATTACK_TYPE_1_N:
				{
					if (attack->m_attacker!=turbo_die)
						continue;

					break;
				}
				case BME_ATTACK_TYPE_N_1:
				{
					if (!attack->m_attackers.IsSet(turbo_die))
						continue;
					break;
				}
			}

			// duplicate the attack, TODO: reduce number of possibilities for TURBO SWING
			// WARNING: attack is not safe to read once we've called "_movelist.Add()"
			BMC_Die *die = attacker->GetDie(turbo_die);
			if (die->HasProperty(BME_PROPERTY_OPTION))
			{
				// set turbo_option '0' for move on the list
				attack->m_turbo_option = 0;
				// and add a move for '1'
				BMC_MoveAttack new_move = *attack;
				new_move.m_turbo_option = 1;
				_movelist.Add(new_move);
			}
			else // SWING
			{
				BMC_MoveAttack new_move = *attack;
				INT swing = die->GetSwingType(0);
				BM_ASSERT(swing!=BME_SWING_NOT);
				INT sides;
				INT min = c_swing_sides_range[swing][0];
				INT max = c_swing_sides_range[swing][1];

				// the move on the list already should be "no change"
				attack->m_turbo_option = die->GetSides(0);

				// always do ends - min
				sides = min;
				if (die->GetSides(0) != sides)
				{
					new_move.m_turbo_option = sides;
					_movelist.Add(new_move);
				}

				// max
				sides = max;
				if (die->GetSides(0) != sides)
				{
					new_move.m_turbo_option = sides;
					_movelist.Add(new_move);
				}

				// now use g_turbo_accuracy
				// step_size = 1 / g_turbo_accuracy
				float step_size = (s_turbo_accuracy<=0) ? 1000 : 1 / s_turbo_accuracy;
				float sides_f;
				for (sides_f = (float)(min + 1); sides_f<max; sides_f += step_size)
				{
					sides = (INT)sides_f;
					BM_ASSERT(sides>min && sides<max);

					// skip the no change action
					if (sides==die->GetSides(0))
						continue;

					new_move.m_turbo_option = sides;
					_movelist.Add(new_move);
				}
			} // end if SWING
		}
	}

	// if no attacks found, generate a pass move
	if (_movelist.Empty())
	{
		move.m_action = BME_ACTION_PASS;
		_movelist.Add(move);
	}
}

// DESC: rerolls the given die, then recomputes initiative
// PRE: m_phase_player is the acting player (the player who does _NOT_ have initiative)
// POST:
// 1) m_phase_player is back to normal (the player with initiative)
// 2) m_phase == BME_PHASE_INITIATIVE_CHANCE if succeeded, otherwise BME_PHASE_INITIATIVE_FOCUS
// NOTE: this is a NATURE step
void BMC_Game::ApplyUseChance(BMC_Move &_move)
{
	BM_ASSERT(m_phase == BME_PHASE_INITIATIVE_CHANCE);

	if (_move.m_action == BME_ACTION_PASS)
	{
		FinishInitiativeChance(true);
		return;
	}

	BM_ASSERT(_move.m_action == BME_ACTION_USE_CHANCE);

	BMC_Player *player = &(m_player[m_phase_player]);
	BMC_Die *die;

	// reroll all chance dice
	INT i;
	for (i=0; i<player->GetAvailableDice(); i++)
	{
		if (!_move.m_chance_reroll.IsSet(i))
			continue;
		die = player->GetDie(i);

		// ASSUME: just Roll() - no special effects, so not calling OnBeforeRollInGame()
		die->SetState(BME_STATE_NOTSET);
		die->Roll();
	}

	// reoptimize dice
	player->OnChanceDieRolled();

	// recompute initiative
	INT initiative = CheckInitiative();

	// in case of a tie or initiative not gained - chance failed
	if (initiative!=0)
	{
		BMF_Log(BME_DEBUG_ROUND, "CHANCE %d fail\n", m_phase_player);

		FinishInitiativeChance(true);
		return;
	}

	BMF_Log(BME_DEBUG_ROUND, "CHANCE %d success\n", m_phase_player);

	// initiative gained
	// POST: after ApplyUseChance() always reset so phasing player is the player with initiative.
	// Meaning don't swap since it was swapped as a PRE
}

// PRE: m_phase_player is the acting player (the player who does _NOT_ have initiative)
// POST:
// 1) m_phase_player is back to normal (the player with initiative)
// 2) m_phase == BME_PHASE_INITIATIVE_FOCUS if succeeded, otherwise BME_PHASE_FIGHT
void BMC_Game::ApplyUseFocus(BMC_Move &_move)
{
	BM_ASSERT(m_phase == BME_PHASE_INITIATIVE_FOCUS);

	if (_move.m_action == BME_ACTION_PASS)
	{
		FinishInitiativeFocus(true);
		return;
	}

	// recover dizzy dice of the other player
	RecoverDizzyDice(m_target_player);

	BM_ASSERT(_move.m_action == BME_ACTION_USE_FOCUS);

	BMC_Player *player = &(m_player[m_phase_player]);
	BMC_Die *die;

	// fix all focus dice
	INT i;
	for (i=0; i<player->GetAvailableDice(); i++)
	{
		if (_move.m_focus_value[i]==0)
			continue;
		die = player->GetDie(i);

		die->SetFocus(_move.m_focus_value[i]);
	}

	// reoptimize dice
	player->OnFocusDieUsed();

	// initiative should have been gained
	BM_ASSERT(CheckInitiative()==m_phase_player);

	BMF_Log(BME_DEBUG_ROUND, "FOCUS %d success\n", m_phase_player);

	// POST: after ApplyUseFocus() always reset so phasing player is the player with initiative.
	// Meaning don't swap since it was swapped as a PRE
}

// PRE: phase RESERVE
// POST: phase PREROUND
void BMC_Game::ApplyUseReserve(BMC_Move &_move)
{
	BM_ASSERT(m_phase == BME_PHASE_RESERVE);

	// fix phase first
	m_phase = BME_PHASE_PREROUND;

	if (_move.m_action == BME_ACTION_PASS)
	{
		return;
	}

	BM_ASSERT(_move.m_action == BME_ACTION_USE_RESERVE);

	BMC_Player *player = &(m_player[m_phase_player]);
	BMC_Die *die = player->GetDie(_move.m_use_reserve);

	die->OnUseReserve();
	player->OnReserveDieUsed(die);
}

// PARAM: _lock true means set to "SWING_SET_LOCKED", otherwise "SWING_SET_READY."  If used inappropriately, the latter
// can result with the AI repeatedly stuck in PlayPreround().
void BMC_Game::ApplySetSwing(BMC_Move &_move, bool _lock)
{
	BMC_Player::SWING_SET	swing_status = _lock ? BMC_Player::SWING_SET_LOCKED : BMC_Player::SWING_SET_READY;

	if (_move.m_action == BME_ACTION_PASS)
	{
		m_player[m_phase_player].SetSwingDiceStatus(swing_status);
		return;
	}

	BM_ASSERT(_move.m_action == BME_ACTION_SET_SWING_AND_OPTION);

	INT i;

	// check swing dice
	for (i=0; i<BME_SWING_MAX; i++)
	{
		if (m_player[m_phase_player].GetTotalSwingDice(i)>0 && c_swing_sides_range[i][0]>0)
			m_player[m_phase_player].SetSwingDice(i, _move.m_swing_value[i]);
	}

	// check option dice
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!m_player[m_phase_player].GetDie(i)->IsUsed())
			continue;

		if (m_player[m_phase_player].GetDie(i)->HasProperty(BME_PROPERTY_OPTION))
			m_player[m_phase_player].SetOptionDie(i, _move.m_option_die[i]);
	}

	m_player[m_phase_player].SetSwingDiceStatus(swing_status);
}

// DESC: apply move and apply player-attack-stage effects (i.e. deterministic effects such
// as mighty, weak, and turbo)
// POST: all attacking dice are marked as NOT_READY.  Any deterministic post-attack actions
// have been applied (e.g. berserk attack side change)
void BMC_Game::ApplyAttackPlayer(BMC_Move &_move)
{
	//BM_ASSERT(_move.m_action == BME_ACTION_ATTACK);

	INT	i;
	BMC_Die *att_die;
	BMC_Player *attacker = &(m_player[m_phase_player]);

	// update game pointer in move to ensure it is correct
	_move.m_game = this;

	if (_move.m_action == BME_ACTION_PASS)
		_move.m_attack = BME_ATTACK_INVALID;

	// capture - attacker effects
	switch (c_attack_type[_move.m_attack])
	{
	case BME_ATTACK_TYPE_1_1:
	case BME_ATTACK_TYPE_1_N:
		{
			att_die = attacker->GetDie(_move.m_attacker);
			att_die->OnApplyAttackPlayer(_move,attacker);
			break;
		}
	case BME_ATTACK_TYPE_N_1:
		{
			for (i=0; i<attacker->GetAvailableDice(); i++)
			{
				if (!_move.m_attackers.IsSet(i))
					continue;
				att_die = attacker->GetDie(i);
				att_die->OnApplyAttackPlayer(_move,attacker);
			}
			break;
		}
	}

	// for TRIP, mark target as needing a reroll
	if (_move.m_attack == BME_ATTACK_TRIP)
	{
		BM_ASSERT(c_attack_type[_move.m_attack]==BME_ATTACK_TYPE_1_1);

		BMC_Die *tgt_die;
		BMC_Player *target = &(m_player[m_target_player]);

		tgt_die = target->GetDie(_move.m_target);
		tgt_die->SetState(BME_STATE_NOTSET);
		tgt_die->OnBeforeRollInGame(target);
	}

	// ORNERY: all ornery dice on attacker must reroll (whether or not attacked)
    // unless the player passed (there must be SOME attack involved)
    if (_move.m_attack != BME_ATTACK_INVALID)
    {
        for (i=0; i<attacker->GetAvailableDice(); i++)
        {
            att_die = attacker->GetDie(i);
            if (att_die->HasProperty(BME_PROPERTY_ORNERY))
                att_die->OnApplyAttackPlayer(_move,attacker,false);	// false means not _actually_attacking
        }
    }
}

// DESC: simulate all random steps - reroll attackers, targets, MOOD
// PRE: all dice that need to be rerolled have been marked BME_STATE_NOTSET
void BMC_Game::ApplyAttackNatureRoll(BMC_Move &_move)
{
	bool		capture = true;
	BMC_Player *attacker = &(m_player[m_phase_player]);
	BMC_Player *target = &(m_player[m_target_player]);
	BMC_Die		*att_die, *tgt_die;
	bool		null_attacker = false;
	INT i;

	// reroll attackers
	switch (c_attack_type[_move.m_attack])
	{
	case BME_ATTACK_TYPE_1_1:
	case BME_ATTACK_TYPE_1_N:
		{
			att_die = attacker->GetDie(_move.m_attacker);
			att_die->OnApplyAttackNatureRollAttacker(_move,attacker);
			break;
		}
	case BME_ATTACK_TYPE_N_1:
		{
			for (i=0; i<attacker->GetAvailableDice(); i++)
			{
				if (!_move.m_attackers.IsSet(i))
					continue;
				att_die = attacker->GetDie(i);
				att_die->OnApplyAttackNatureRollAttacker(_move,attacker);
			}
			break;
		}
	}

	// TRIP attack
	if (_move.m_attack == BME_ATTACK_TRIP)
	{
		// reroll target
		BM_ASSERT(c_attack_type[_move.m_attack]==BME_ATTACK_TYPE_1_1);

		tgt_die = target->GetDie(_move.m_target);
		tgt_die->OnApplyAttackNatureRollTripped();
	}
}

// DESC: handle non-deterministic and capture-specific effects.  This includes
// TIME_AND_SPACE, TRIP, NULL, VALUE
// PRE: all dice that needed to be rerolled have been rerolled
void BMC_Game::ApplyAttackNaturePost(BMC_Move &_move, bool &_extra_turn)
{
	// update game pointer in move to ensure it is correct
	_move.m_game = this;

	// for TIME_AND_SPACE
	_extra_turn = false;

	bool		capture = true;
	BMC_Player *attacker = &(m_player[m_phase_player]);
	BMC_Player *target = &(m_player[m_target_player]);
	BMC_Die		*att_die, *tgt_die;
	bool		null_attacker = false;
	bool		value_attacker = false;
	INT i;

	// capture - attacker effects
	switch (c_attack_type[_move.m_attack])
	{
	case BME_ATTACK_TYPE_1_1:
	case BME_ATTACK_TYPE_1_N:
		{
			att_die = attacker->GetDie(_move.m_attacker);
			null_attacker = att_die->HasProperty(BME_PROPERTY_NULL);
			value_attacker = att_die->HasProperty(BME_PROPERTY_VALUE);

			// TIME AND SPACE
			if (att_die->HasProperty(BME_PROPERTY_TIME_AND_SPACE) && att_die->GetValueTotal()%2==1)
				_extra_turn = true;
			break;
		}
	case BME_ATTACK_TYPE_N_1:
		{
			for (i=0; i<attacker->GetAvailableDice(); i++)
			{
				if (!_move.m_attackers.IsSet(i))
					continue;
				att_die = attacker->GetDie(i);
				null_attacker = null_attacker || att_die->HasProperty(BME_PROPERTY_NULL);
				value_attacker = value_attacker || att_die->HasProperty(BME_PROPERTY_VALUE);

				// TIME AND SPACE
				if (att_die->HasProperty(BME_PROPERTY_TIME_AND_SPACE) && att_die->GetValueTotal()%2==1)
					_extra_turn = true;
			}
			break;
		}
	}

	// TRIP attack
	if (_move.m_attack == BME_ATTACK_TRIP)
	{
		BM_ASSERT(c_attack_type[_move.m_attack]==BME_ATTACK_TYPE_1_1);

		tgt_die = target->GetDie(_move.m_target);

		// proceed with capture?
		if (att_die->GetValueTotal() < tgt_die->GetValueTotal())
		{
			capture = false;
			target->OnDieTripped();	// target needs to reoptimize
		}
	}

	// capture - target effects
	if (capture)
	{
		switch (c_attack_type[_move.m_attack])
		{
		case BME_ATTACK_TYPE_N_1:
		case BME_ATTACK_TYPE_1_1:
			{
				tgt_die = target->OnDieLost(_move.m_target);
				if (null_attacker)
					tgt_die->AddProperty(BME_PROPERTY_NULL);
				if (value_attacker)
					tgt_die->AddProperty(BME_PROPERTY_VALUE);
				tgt_die->SetState(BME_STATE_CAPTURED);
				attacker->OnDieCaptured(tgt_die);

				break;
			}
		case BME_ATTACK_TYPE_1_N:
			{
				// this is a little complex since removing dice changes their indices, so we track number removed
				INT removed = 0;
				INT i2;	// true index
				for (i=0; i<target->GetAvailableDice() + removed; i++)
				{
					if (!_move.m_targets.IsSet(i))
						continue;
					i2 = i - removed++;	// determine true index
					tgt_die = target->OnDieLost(i2);
					if (null_attacker)
						tgt_die->AddProperty(BME_PROPERTY_NULL);
					if (value_attacker)
						tgt_die->AddProperty(BME_PROPERTY_VALUE);
					tgt_die->SetState(BME_STATE_CAPTURED);
					attacker->OnDieCaptured(tgt_die);

				}
				break;
			}
		}
	}

	// since attacker dice rerolled, re-optimize
	attacker->OnAttackFinished();
}

// DESC: apply this move, apply the nature rules, then return the winning probability of the phase_player
// PRE: acting player move has been applied to the game
// POST: game has been updated to the state of the next move, but that move has not been applied
// RETURNS: the winning probability of the _pov_player
float BMC_Game::PlayRound_EvaluateMove(INT _pov_player)
{
	BMC_Move move;
	int			original_id		= m_phase_player;	// the player who is evaluating a move
	BMC_Player *player;								// the player who is making the next move
	int			id;
	BMC_AI *	ai;

	if (m_phase==BME_PHASE_PREROUND)
	{
		// PlayPreround
		BM_ASSERT(GetPhasePlayer()->GetSwingDiceSet()!=BMC_Player::SWING_SET_NOT);
		m_target_player = !m_phase_player;

		// if swing not set for opponent, then use that
		player = GetTargetPlayer();
		if (!player->NeedsSetSwing())
			player->SetSwingDiceStatus(BMC_Player::SWING_SET_LOCKED);	// might as well use LOCKED instead of READY

		if (player->GetSwingDiceSet() == BMC_Player::SWING_SET_NOT)
		{
			id = m_target_player;
			ai = m_ai[id];

			// to accurately model simultaneous swing actions, temporarily mark SWING_SET_READY as SWING_SET_NOT
			bool original_id_was_ready = false;
			if (m_player[original_id].GetSwingDiceSet()==BMC_Player::SWING_SET_READY)
			{
				m_player[original_id].SetSwingDiceStatus(BMC_Player::SWING_SET_NOT);
				original_id_was_ready = true;
			}

			// setup GetSetSwing
			m_phase_player = m_target_player;
			m_target_player = !m_phase_player;
			ai->GetSetSwingAction(this, move);

			if (original_id_was_ready)
				m_player[original_id].SetSwingDiceStatus(BMC_Player::SWING_SET_READY);

			goto prem_score;
		}

		// FinishPreround - does initiative
		FinishPreround();
	}

	// PlayInitiative
	if (m_phase==BME_PHASE_INITIATIVE)
	{
		m_phase = BME_PHASE_INITIATIVE_CHANCE;
	}

	if (m_phase == BME_PHASE_INITIATIVE_CHANCE)
	{
		// PlayInitiativeChance - only if non-initiative player has CHANCE dice
		id = m_target_player;
		player = &(m_player[id]);
		ai = m_ai[id];
		if (player->HasDieWithProperty(BME_PROPERTY_CHANCE))
		{
			// swap 'm_phase_player' to be the acting player.
			m_phase_player = !m_phase_player;
			m_target_player = !m_target_player;

			ai->GetUseChanceAction(this, move);

			// fix 'm_phase_player'
			m_phase_player = !m_phase_player;
			m_target_player = !m_target_player;

			goto prem_score;
		}

		// FinishInitiativeChance
		FinishInitiativeChance(false);
	}

	if (m_phase == BME_PHASE_INITIATIVE_FOCUS)
	{
		// PlayInitiativeFocus - only if non-initiative player has FOCUS dice
		id = m_target_player;
		player = &(m_player[id]);
		ai = m_ai[id];
		if (player->HasDieWithProperty(BME_PROPERTY_FOCUS))
		{
			// swap 'm_phase_player' to be the acting player.
			m_phase_player = !m_phase_player;
			m_target_player = !m_target_player;

			ai->GetUseFocusAction(this,move);

			// fix 'm_phase_player'
			m_phase_player = !m_phase_player;
			m_target_player = !m_target_player;

			goto prem_score;
		}


		// FinishInitiaitveFocus
		FinishInitiativeFocus(false);
	}

	FinishInitiative();

	// got this far, so next move is in "PlayFight"
	id = m_phase_player;
	ai = m_ai[id];
	ai->GetAttackAction(this, move);
	goto prem_score;

prem_score:
	BM_ASSERT(ai->IsBMAI3());
	BMC_BMAI3 *bmai3 = (BMC_BMAI3 *)ai;

	float next_player_prob_win = bmai3->GetLastProbabilityWin();
	BM_ASSERT(next_player_prob_win<=1);

	if (id != _pov_player)
		return 1 - next_player_prob_win;
	else
		return next_player_prob_win;
}

// DESC: apply this move, apply the nature rules, then return the winning probability of the phase_player
// PARAM: _action is the move for "phase_player"
// RETURNS: the winning probability of the _pov_player
float BMC_Game::PlayFight_EvaluateMove(INT _pov_player, BMC_Move &_move)
{
	BM_ASSERT(m_phase == BME_PHASE_FIGHT);
	BM_ASSERT(!FightOver());
	int original_phase_player = m_phase_player;
	int original_target_player = m_target_player;
	bool extra_turn = false;

	if (_move.m_action == BME_ACTION_SURRENDER)
		return 0;

	if (_move.m_action == BME_ACTION_PASS && m_last_action == BME_ACTION_PASS)
	{
		// game over
		return ConvertWLTToWinProbability();
	}
	else
	{
		//BM_ASSERT(_move.m_action == BME_ACTION_ATTACK);

		ApplyAttackPlayer(_move);
		ApplyAttackNatureRoll(_move);
		ApplyAttackNaturePost(_move, extra_turn);
	}

	m_last_action = _move.m_action;

	// FOCUS: undizzy the dice
	RecoverDizzyDice(m_phase_player);

	// did that end the fight?
	if (FightOver() || m_phase != BME_PHASE_FIGHT)
		return ConvertWLTToWinProbability();

	// if not, then finally, get action and return the probability win
	FinishTurn(extra_turn);

	BMC_Move move2;
	BM_ASSERT(m_ai[m_phase_player]->IsBMAI3());
	BMC_BMAI3 *bmai3 = (BMC_BMAI3 *)(m_ai[m_phase_player]);
	bmai3->GetAttackAction(this, move2);

	float new_phase_player_prob_win = bmai3->GetLastProbabilityWin();
	BM_ASSERT(new_phase_player_prob_win<=1);

	if (m_phase_player != _pov_player)
		return 1 - new_phase_player_prob_win;
	else
		return new_phase_player_prob_win;
}

// POST: m_phase is PREROUND or GAMEOVER appropriately
void BMC_Game::PlayFight(BMC_Move *_start_action)
{
	BMC_Move move;
	m_last_action = BME_ACTION_MAX;

	while (m_phase != BME_PHASE_PREROUND)
	{
		bool extra_turn = false;

		if (FightOver())
			return;

		// get action from phase player
		if (_start_action)
		{
			move = *_start_action;
			_start_action = NULL;
		}
		else
			m_ai[m_phase_player]->GetAttackAction(this, move);

		g_logger.Log(BME_DEBUG_ROUND, "action p%d ", m_phase_player );
		move.Debug(BME_DEBUG_ROUND);

		// is it a pass or surrender?
		if (move.m_action == BME_ACTION_SURRENDER)
		{
			m_player[m_phase_player].OnSurrendered();
			return;
		}
		else if (move.m_action == BME_ACTION_PASS && m_last_action==BME_ACTION_PASS)
		{
			// both passed - end game
			//BMF_Log(BME_DEBUG_ROUND, "both players passed - ending fight\n");
			return;
		}
		else // if (move.m_action == BME_ACTION_ATTACK)
		{
			ApplyAttackPlayer(move);
			ApplyAttackNatureRoll(move);
			ApplyAttackNaturePost(move, extra_turn);
		}

		m_last_action = move.m_action;

		// FOCUS: undizzy the dice
		RecoverDizzyDice(m_phase_player);

		FinishTurn(extra_turn);
	}
}

void BMC_Game::RecoverDizzyDice(INT _player)
{
	INT i;
	BMC_Player *p = GetPlayer(_player);

	for (i=0; i<p->GetAvailableDice(); i++)
	{
		BMC_Die *d = p->GetDie(i);
		if (d->GetState() == BME_STATE_DIZZY)
			d->OnDizzyRecovered();
	}
}

void BMC_Game::SimulateAttack(BMC_Move &_move, bool &_extra_turn)
{
	ApplyAttackPlayer(_move);

	ApplyAttackNatureRoll(_move);

	ApplyAttackNaturePost(_move, _extra_turn);
}