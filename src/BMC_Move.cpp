///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Move.cpp
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
//
// REVISION HISTORY:
// dbl100524 - broke this logic out into its own class file
///////////////////////////////////////////////////////////////////////////////////////////

#include "BMC_Move.h"

#include <cstdio>
#include "BMC_Game.h"
#include "BMC_Logger.h"


const char *m_swing_name[BME_SWING_MAX] =
{
	"None",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
};

// ACTION names
const char *m_action_name[BME_ACTION_MAX] =
{
	"aux",
	"chance",
	"focus",
	"swing/option",
	"reserve",
	"attack",
	"pass",
	"surrender",
};

void BMC_Move::Debug(BME_DEBUG _cat, const char *_postfix)
{
	if (!g_logger.IsLogging(_cat))
		return;

	INT i;
	BMC_Player *phaser = m_game->GetPhasePlayer();

	printf("%s ", m_action_name[m_action]);

	switch (m_action)
	{
	case BME_ACTION_SURRENDER:
	case BME_ACTION_PASS:
		break;

	case BME_ACTION_SET_SWING_AND_OPTION:
		{
			// check swing dice
			for (i=0; i<BME_SWING_MAX; i++)
			{
				if (phaser->GetTotalSwingDice(i)>0 && g_swing_sides_range[i][0]>0)
	 				printf("%s %d  ", m_swing_name[i], m_swing_value[i]);
			}

			// check option dice
			for (i=0; i<BMD_MAX_DICE; i++)
			{
				if (!phaser->GetDie(i)->IsUsed())
					continue;

				if (phaser->GetDie(i)->HasProperty(BME_PROPERTY_OPTION))
				{
					phaser->GetDie(i)->Debug(_cat);
					printf("%d  ", m_option_die[i]);
				}
			}

			break;
		}

	case BME_ACTION_USE_CHANCE:
		{
			for (i=0; i<phaser->GetAvailableDice(); i++)
			{
				if (!m_chance_reroll.IsSet(i))
					continue;
				printf(" %d = ", i);
				phaser->GetDie(i)->Debug(_cat);
			}
			break;
		}

	case BME_ACTION_USE_FOCUS:
		{
			for (i=0; i<phaser->GetAvailableDice(); i++)
			{
				if (!phaser->GetDie(i)->HasProperty(BME_PROPERTY_FOCUS) || m_focus_value[i]==0)
					continue;
				phaser->GetDie(i)->Debug(_cat);
				printf("-> %d ", m_focus_value[i]);
				//printf(" %d -> %d = ", i, m_focus_value[i]);
			}
			break;
		}

	case BME_ACTION_USE_RESERVE:
		{
			printf("%d = ", m_use_reserve);
			phaser->GetDie(m_use_reserve)->Debug(_cat);
			break;		}

	case BME_ACTION_ATTACK:
		{
			BMC_Player *attacker = GetAttacker();
			BMC_Player *target = GetTarget();
			INT printed;

			printf("%s - ", g_attack_name[m_attack]);

			if (MultipleAttackers())
			{
				printed = 0;
				for (i=0; i<attacker->GetAvailableDice(); i++)
				{
					if (!m_attackers.IsSet(i))
						continue;
					if (printed++>0)
						printf("+ ");
					attacker->GetDie(i)->Debug(_cat);
				}
			}
			else
			{
				attacker->GetDie(m_attacker)->Debug(_cat);
			}

			printf("-> ");

			if (MultipleTargets())
			{
				printed = 0;
				for (i=0; i<target->GetAvailableDice(); i++)
				{
					if (!m_targets.IsSet(i))
						continue;
					if (printed++>0)
						printf("+ ");
					target->GetDie(i)->Debug(_cat);
				}
			}
			else
			{
				target->GetDie(m_target)->Debug(_cat);
			}

			if (m_turbo_option>0)
			{
				printf(" TURBO %d", m_turbo_option);
			}

			break;
		}

	default:
		break;
	} // end switch(m_action)

	if (_postfix)
		printf(_postfix);
	return;
}

BMC_Player *BMC_Move::GetAttacker()
{
	return m_game->GetPlayer(m_attacker_player);
}

BMC_Player *BMC_Move::GetTarget()
{
	return m_game->GetPlayer(m_target_player);
}

BMC_MoveList::BMC_MoveList()
{
	list.reserve(32);
}

void BMC_MoveList::Add(BMC_Move & _move)
{
	/*
	BMC_Move * move;

	move = new BMC_Move;
	std::memcpy(move, &_move, sizeof(BMC_Move));
	*/

	list.push_back(_move);
}

void BMC_MoveList::Clear()
{
	/*
	INT i,s;
	s = Size();
	for (i=0; i<s; i++)
		delete list[i];
	*/
	list.clear();
}

void BMC_MoveList::Remove(int _index)
{
	/*
	delete list[_index];
	*/
	list[_index] = list[list.size()-1];
	list.pop_back();
}

