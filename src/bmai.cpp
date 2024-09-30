// TODO:
// - reenable have_turbo check
// - 092103 BUG: the game had a small chance of ending in a tie but
//   BMAI reported 100% win

///////////////////////////////////////////////////////////////////////////////////////////
// bmai.cpp
// Copyright (c) 2001-2023 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: BMAI main code and main game rules
//
// REVISION HISTORY:
// drp040101 - handle TRIP
// drp040101 - handle OPTION (basic decision only)
// drp040301 - added BERSERK, fixed SPEED, fixed parsing MOOD/TURBO SWING dice with a defined value
// drp040701 -	handle TIME_AND_SPACE 
//				ability to set target wins and to set number of simulations, 
//				store who won initiative
//				'playfair' code
//				BMC_Logger	
// drp040801 - added MIGHTY and WEAK
// drp040901 -	fixed so correctly allows changing # wins
//				fixed PlayFair stats	
//				fixed memory leak in BMC_MoveList::Clear
//				added mode 3 to PlayFair (2-ply mode 1)
// drp041001 -	correctly fixed memory leak (~BMC_MoveList)
//				don't consider TRIP when doing initiative
//				play games with a copy of g_game (otherwise things like berserk maintain state change)
//				sizeof die to 20, sizeof game to 432
//				use g_ai_mode1 as the QAI for mode3
//				changed movelist to start at 16 entries in the vector
//				changed movelist to use vector of moves instead of move pointers
// drp041201 -	added 'ply #' command
// drp041501 -	fixed incorrect calculation of m_sides_max on some parsed option dice
// drp051001 -  implemented GetSetSwingAction() - uses simulations
// drp051201 -	implemented support for RESERVE (AI and simulation)
// drp051401 -	implemented support for ORNERY
//			 -	recognize that m_swing_set for opponent if only has (defined) option dice
// drp060201 -  implemented support for TURBO OPTION (not TURBO SWING, and only one die)
// drp060301 -  implemented support for TURBO SWING (slow!)
// drp060501 -	implemented 'maxbranch' param for BMAI, to cap sims in "moves * sims"
//			 -	implemented g_turbo_accuracy
// drp070701 -	implemented support for CHANCE
//			    fixed initiative rules, which gave advantage to player with fewer dice (incorrect)
// drp072101	- added "debug" command, 
//				- added "seed" command
//				- changed mode 3 for "playfair" to be standard QAI
//				- in PlayGame() don't call GetReserveAction() unless the loser has reserve dice
//				- more ROUND debugging
// drp080501	- implemented support for MORPHING dice, adds set The Metamorphers
// drp081201	- fixed NULL (count as 0 in addition to capture effect - i.e. they are 0 when captured or held)
// drp102801	- release 1.0
// drp111001	- support for FOCUS
// drp111401	- support for WARRIOR (Sailor Moon)
// drp032502	- support for SLOW (Brom - Giant)
// drp033002	- support for UNIQUE (2000 Rare Promo)
// drp033102	- moved to BMAI2 (with culling, ply 3)
//				- TRIP illegal if can't capture (i.e. one die tripping two)
// drp040102    - website now live (www.buttonmen.com)
//				- more BMAI2 improvements
// drp062702	- BMAI2: improvements with TRIP and bad situations
// drp062902	- better RNG
//				- fixed dealing with TWIN that are MIGHTY/WEAK 
//				- BMAI2: surrenders if no move seems to result in a win
//				- BMAI2: fixed movelist leak in CullMoves()
//				- BMAI2: culling for preround (swing/option)
// drp063002    - Added performance stats
//              - BMAI: fixed major bug where ply3+ BMAI/BMAI2 was not working 
//              - BMAI3: replaces BMAI2, major change (for attack actions) to use next ply for more accurate move scoring.
//                Instead of playing out the sim and assigning a score of -1/0/+1 it uses the estimated P(win) of the next ply action.
// drp070202	- m_min_sims/m_max_sims.  Previously the min was 20 now the default is 5. 
//				- apply decay to min/max sims [currently hard-coded]
//				- BMAI: apply max_branch to UseChance() and Reserve()
// drp070502	- GenerateValidChance()
//				- BMAI3: apply move culling to GetUseChance(), GetUseFocus()
//				- BMAI3: use accurate move scoring on preround moves
// drp070602	- Stats: BMAI3 average moves/sims per ply 
// drp072102	- support for UNSKILLED (The Flying Squirrel)
// drp082802	- support for STINGER (set Diceland)
// drp092302	- ignore setswing action if have no swing dice (useless ply)
//				- AI::GetSetSwingAction() did not work with more than one swing type (but only used with ply 1 AIs)
// drp022203	- MoveList: reserve space for 32 instead of 16 moves. Also use store Move objects instead of pointers
//				  so that we cut out the extra allocs (if using pool, we would want to use pointers)
//				- BMC_Move: cut from 32b to 24b by replacing m_option_die with a BitArray<>
//				- BMAI3 SetSwing: catch case where the number of moves is too huge [Gordo] by randomly cutting out moves
// drp071005	- changed unskilled from "k" to "~" (for Konstant)
//				- added basic support for Konstant
// drp071305	- konstant: don't reroll (not done yet)
//			    - fixed memory trasher with turbo swing
// drp071505	- fixed UNIQUE check (mattered), fixed OPTION check (didn't matter)
// drp091905	- BMAI3 Focus/Chance: "pass" moves were being mis-evaluated (scoring from POV of opponent) so
//				  the AI would almost always pick "pass". Now pass "pov_player" to EvaluateMove() functions.
//				  This was a big problem and explains the 42.4% with "Legend of the Five Rings" or
//				  46.7% with Yoyodyne, 
// dbl051823	- added P-Swing, Q-Swing support
// dbl053123	- moved TestRNG and SetupTestGame to a `./test/` dir and testing framework
// dbl053123	- added ability to disallow surrenders
///////////////////////////////////////////////////////////////////////////////////////////

// TOP TODO
// - allow 1 die skill attacks (important for FIRE)
// - decay param
// - Performance: cap ply on preround moves, est time after 1 ply 1 sim and readjust ply, tweak plies/sims/decay
// - Performance: VTune
// - Hope vs Hope
// - BMAI3: move culling and use scored moves on reserve
// - move ordering

// TODO:
// - test ^hHc
// - Washu: swing must be submitted with reserve
// - QAI fuzziness should be based on range of scores found
// - comprehensive search
// - optimize game state
// - pre-allocate moves, use move pool
// - in N->1 and 1->N, can optimize by cutting out based on !CanDoAttack, and !CanBeAttacked
//   so it doesn't pursue lines that include that die
// - stats (i.e. nodes evaluated, etc)
// - ScoreAttack() based on capture, trip, mood/bers/mighty/weak
// - optimize: if player maintains which properties are present in dice, GetValidAttacks() can optimize check for TURBO
// - TURBO support for multiple dice
// - TURBO at end of game think of value for next game (Alice)
// - does not properly play consecutive games (e.g. not carrying over RESERVE action or clearing SwingDiceSet())
// - TRIP: looks like rerolling effects are only being applied for attacking dice and not tripped dice

// AI TODO:
// * apply same culling logic to other moves. Modularize code?
// * transposition table/stratification - seeing a lot of dup cases in later plies
// * stratify
// - stratify: attack rerolls, chance rolls, initiative roll, GetSetSwingAction(), TURBO SWING 
// - don't recursively do BMAI (ply>1) if top-level action is reserve, or standalone swing
// - optimize QAI by not completely applying attack in eval?
// - optimize simulations by breaking up ApplyAttack into ApplyAttackPre, SimAttackRoll, ApplyAttackPost
// - QAI: bonus for rerolling a vulnerable die
// - sim: recognize endgame conditions
// - sim: recognize moves that we have already seen, slowly build up chance of winning as previously seen that move (?)
// - FOCUS: if only takes the minimal focus step, only goes 2 ply so 3rd "focus action" will be a PASS 


//--------------------------------------------------------------------------------------------------------

// DICE HANDLED: (,Xpzsdqt?n/B^Hhrocmf`wug)
// - twin, swing, poison, speed, shadow, stealth, queer, trip, mood, null, option, berserk,
//   time and space, mighty, weak, reserve, ornery, chance, morphing, focus,
//   warrior, slow, unique, stinger
//
// PROPRIETARY DICE HANDLED: (~)
// - unskilled

// DICE REMAINING: (most worthwhile +FvGk) (+DC%GF@vk)
// k KONSTANT: (partial support)
//	  done: cannot power attack, cannot perform 1 die skill attack, does not reroll
//	  todo: can add or subtract in a skill attack
// + AUX: if both players have one and agree, they use them.  If one player doesn't and both
//   players agree, then you get the same die as the opponent.
// D DOPPLEGANGER: if doing a power attack, att_die becomes exact copy of tgt_die (still rerolls)
// C WILDCARD:
// % RADIOACTIVE: if attacks, decays into two dice (not radioactive, half sides - ROUND?)
//				  if attacked, all that attack it decay (lose RADIOACTIVE, TURBO, MOOD)
// G RAGE: when captured, owner of the RAGE adds equal die minus RAGE ability.  Not counted for initiative
// F FIRE: cannot power, can increase value of other dice in skill or
//      power attack by decreasing value of powre die.
// @ CHAOTIC: ? (fanatics)
// v VALUE: scored as if sides was number it was showing.  Store this value when captured
// SPECIAL: cannot SKILL against Japanese Beetle

//--------------------------------------------------------------------------------------------------------

// SETS CAN PLAY:
// - 1999 Rare-Promo, 2000 Rare-Promo, *
// - 2002 Rare-Promo, 2003 Rare-Promo, *
// - Brawl, Brom, Bruno!, *
// - Chicago Crew, Club Foglio, Diceland, Dork Victory, Fairies, *
// - Fantasy, Four Horsemen, Freaks, *
// - Geekz, *
// - Howling Wolf, Iron Chef, *
// - Legend of the Five Rings, Lunch Money, Majesty, *
// - Presidential Button Men, Renaissance, Sailor Moon, Sailor Moon 2, Samurai, Sanctum, Save the Ogres,
//   Sluggy Freelance, Soldiers, *
// - Tenchi Muyo, The Metamorphers, Vampyres, *
// - Wonderland, Yoyodyne

// CHECK:

// SETS CANT PLAY: (* = TL, ? = haven't verified dice)
// - * 2001 Rare-Promo (G)
// - 7 Deadly Sins (D)
// - Amber (D)
// - Avatars (...)
// - * Button Brains (k)
// - * Button Lords (+)
// - Fanatics (...)
// - Free Radicals (D)
// - Hodge Podge (v@DGF, 0-sided)
// - It Came From the 80's (G+%)
// - * Japanese Beetle (special rules - Beetle)
// - Las Vegas (C)
// - Moody Mutants (%J)
// - * Polycon (F)
// - Radioactive (%)
// - Space Girlz (plasma)
// - Victorian Horror (vk)

// RIP SETS WAS ABLE TO PLAY:
//  - BladeMasters, Dr. Oche

// Game Stages and Valid Actions: [outdated]
//	BME_PHASE_PREROUND: p0/p1 must both do (depending on state)
//		BME_ACTION_SET_SWING_AND_OPTION,
//	BME_PHASE_INITIATIVE: first CHANCE then FOCUS
//  BME_PHASE_INITIATIVE_CHANCE: alternately give the non-initiative player the opporutnity to reroll one CHANCE die
//      BME_ACTION_USE_CHANCE
//		BME_ACTION_PASS
//  BME_PHASE_INITIATIVE_FOCUS: alternates give the non-initiative player the opportunity to use FOCUS dice
//		BME_ACTION_USE_FOCUS,
//		BME_ACTION_PASS
//	BME_PHASE_FIGHT: alternates p0/p1 until game over
//		BME_ACTION_ATTACK,
//		BME_ACTION_PASS
//		BME_ACTION_SURRENDER 

//--------------------------------------------------------------------------------------------------------
// LOW PRIORITY

// QUESTIONS (can test?)
// - reset SWING/OPTION in a TIE?  RESERVE in a TIE? [normally no - site allows changing swing after 3 ties]
// - allowing skill only with 2 dice, but speed/berserk with 1. [skill is allowed with 1]
// - for TIME AND SPACE, works if *either* dice in a twin is odd?  
// - rules for MOOD SWING on 'U'?  or is MOOD SWING on X/V also uniform dist?
// - INITIATIVE: handle ties? [currently gives to player 0]
// - ORNERY causes MIGHTY(y)/WEAK(y)/MOOD(y)/TURBO(y)?
// - CHANCE causes ?Hh!? (assuming no)
// - MORPHING SPEED/BERSERK or TURBO works? (not allowing)
// - MORPHING MIGHTY/WEAK order? (applying morphing after)
// - WARRIOR: worth 0 when has skill, still worth 0 once loses? (assuming no)
// - KONSTANT: reroll on trip?

// LOW PRIORITY TODO:
// - clean up logging system
// - don't know "last_action".  Needs to be communicated from bmai.pl to know if a pass ends game
// - BMAI3: try to increase sims after culling. I.e., right now it determines sims immediately
//   for 'x' moves based on 'branch'.  But we are culling moves so can reevaluate sims to
//   target original 'branch'.

// SPECIAL RULES (official)
// - INITIATIVE: if run out of dice, initiative goes to player with extra dice

//--------------------------------------------------------------------------------------------------------

// includes
#include "bmai.h"
#include "bmai_ai.h"
#include "dieindexstack.h"
#include "game.h"
#include "player.h"
#include "parser.h"
#include "stats.h"
#include "logger.h"
#include <ctime>	// for time()
#include <cmath>	// for fabs()
#include <cstdarg>  // for va_start() va_end()

// some compilers want to be very explicit
#include <cstring>  // string functions on std namespace

// definitions
BME_ATTACK_TYPE	g_attack_type[BME_ATTACK_MAX] =
{
	BME_ATTACK_TYPE_1_1,  // FIRST & POWER 1:1
	BME_ATTACK_TYPE_N_1,  // SKILL N:1
	BME_ATTACK_TYPE_1_N,  // BERSERK
	BME_ATTACK_TYPE_1_N,  // SPEED
	BME_ATTACK_TYPE_1_1,  // TRIP
	BME_ATTACK_TYPE_1_1,  // SHADOW
	BME_ATTACK_TYPE_0_0,  // INVALID
};

// MOOD dice - from BM page:
//X?: Roll a d6. 1: d4; 2: d6; 3: d8; 4: d10; 5: d12; 6: d20. 
//V?: Roll a d4. 1: d6; 2: d8; 3: d10; 4: d12. 
INT g_mood_sides_X[BMD_MOOD_SIDES_RANGE_X] = { 4, 6, 8,  10, 12, 20 };
INT g_mood_sides_V[BMD_MOOD_SIDES_RANGE_V] = { 6, 8, 10, 12 };

// MIGHTY dice - index by old number of sides
INT g_mighty_sides[20] = { 1, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 16, 16, 16, 16, 20, 20, 20, 20 };

// WEAK dice - index by old number of sides
INT g_weak_sides[20] = { 1, 1, 1, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 12, 12, 16, 16, 16 };

// SWING dice - from spindisc's page
INT g_swing_sides_range[BME_SWING_MAX][2] = 
{
	{ 0, 0 },
	{ 1, 30 },	// P
	{ 2, 20 },	// Q
	{ 2, 16 },
	{ 6, 20 },
	{ 2, 12 },
	{ 8, 30 },	// U
	{ 6, 12 },
	{ 4, 12 },
	{ 4, 20 },	// X
	{ 1, 20 },
	{ 4, 30 },	// Z
};

const char * g_swing_name[BME_SWING_MAX] =
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

// PHASE names
const char *g_phase_name[BME_PHASE_MAX] =
{
	"preround",
	"reserve",
	"initiative",
	"chance",
	"focus",
	"fight",
	"gameover"
};

// ATTACK names
const char *g_attack_name[BME_ATTACK_MAX] =
{
	"power",
	"skill",
	"berserk",
	"speed",
	"trip",
	"shadow",
	"invalid",
};

// ACTION names
const char *g_action_name[BME_ACTION_MAX] =
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

// BME_DEBUG setting names
const char *g_debug_name[BME_DEBUG_MAX] =
{
	"ALWAYS",
	"WARNING",
	"PARSER",
	"SIMULATION",
	"ROUND",
	"GAME",
	"QAI",
	"BMAI",
};

// globals
BMC_RNG		g_rng;
BMC_BMAI3	g_ai;	// the main AI used
BMC_Game	g_game(false);
BMC_Parser	g_parser;
BMC_Logger	g_logger;
float		g_turbo_accuracy = 1;	// 0 is worst, 1 is best
BMC_Stats	g_stats;

// AI types - for setting up compare games
#define BMD_AI_TYPES	3
BMC_QAI		g_qai2;
BMC_BMAI	g_bmai;
BMC_BMAI3	g_bmai3;
BMC_AI *	g_ai_type[BMD_AI_TYPES] = { &g_bmai, &g_qai2, &g_bmai3 };

///////////////////////////////////////////////////////////////////////////////////////////
// logging methods
///////////////////////////////////////////////////////////////////////////////////////////

void BMF_Error( const char *_fmt, ... )
{
	va_list	ap;
	va_start(ap, _fmt), 
	vprintf( _fmt, ap );
	va_end(ap);	

	exit(1);
}

void BMF_Log(BME_DEBUG _cat, const char *_fmt, ... )
{
	if (!g_logger.IsLogging(_cat))
		return;

	va_list	ap;
	va_start(ap, _fmt), 
	vprintf( _fmt, ap );
	va_end(ap);	
}

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Logger
///////////////////////////////////////////////////////////////////////////////////////////

BMC_Logger::BMC_Logger()
{
	INT i;
	for (i=0; i<BME_DEBUG_MAX; i++)
		m_logging[i] = true;
}

void BMC_Logger::Log(BME_DEBUG _cat, const char *_fmt, ... )
{
	if (!m_logging[_cat])
		return;

	va_list	ap;
	va_start(ap, _fmt), 
	vprintf( _fmt, ap );
	va_end(ap);	
}

bool BMC_Logger::SetLogging(const char *_catname, bool _log)
{
	// case-insensitive compares are not easy to come by in a cross-platform way
	// so lets uppercase the input and do a normal std::strcmp below
	std::string _cn(_catname);
	std::transform(_cn.begin(), _cn.end(), _cn.begin(), ::toupper);

	INT i;
	for (i=0; i<BME_DEBUG_MAX; i++)
	{
		if (!std::strcmp(_catname, g_debug_name[i]))
		{
			SetLogging((BME_DEBUG)i, _log);
			Log(BME_DEBUG_ALWAYS, "Debug %s set to %d\n", _catname, _log);
			return true;
		}
	}

	BMF_Error("Could not find debug category: %s\n", _catname);
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_RNG 
//
// DESC: a good RNG, was better and faster than the old standard library random(), but
// have not evaluated it against MSVC 6's standard library random.  This is taken
// from my thesis Poker AI, which in turn took it from somewhere else.
//
// TEST: with 1M samples of FRand and 10 distribution sets, the stddev was 3.7, with
// a maximum "error" of 0.29%.  For the runtime library "rand()" the stddev was 4 with a maximum
// "error" of 0.52%.
///////////////////////////////////////////////////////////////////////////////////////////

BMC_RNG::BMC_RNG() :
	m_seed(78904497)	
{
}

// PARAM: 0 means completely random
void BMC_RNG::SRand(UINT _seed)
{
	if (_seed==0)
		_seed = (UINT)time(NULL);

	// bad arbitrary way of tweaking bad seed values
	if (!_seed) 
		m_seed=-1;
	else if((_seed>>16)==0) 
		m_seed = _seed | (_seed<<16);
	else 
		m_seed = _seed;
}

UINT BMC_RNG::GetRand()
{
	// drp060323 - removed "register" keyword, which is no longer supported in C++ versions after 14. Optimizer should be making it irrelevant.
	long LO, HI;

    LO = ( (m_seed) & 0177777 ) * 16807;
    HI = ( (m_seed) >> 16 ) * 16807 + ( LO >> 16 );
    LO = ( LO & 0177777 ) + ( HI >> 15 );
    HI = ( HI & 0077777 ) + ( LO >> 16 );
    LO = ( LO & 0177777 ) + ( HI >> 15 );
    HI = (( HI & 077777 ) << 16 ) + LO;
    return( (m_seed) = HI );
}

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_DieIndexStack methods
///////////////////////////////////////////////////////////////////////////////////////////

BMC_DieIndexStack::BMC_DieIndexStack(BMC_Player *_owner)
{
	owner = _owner;
	die_stack_size = 0;
	value_total = 0;
}

void BMC_DieIndexStack::Debug(BME_DEBUG _cat)
{
	printf("STACK: ");
	INT i;
	for (i=0; i<die_stack_size; i++)
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

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_DieData methods
///////////////////////////////////////////////////////////////////////////////////////////

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
	BMF_Log(_cat,"(%x)", m_properties & (~BME_PROPERTY_VALID));
}

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Die methods
///////////////////////////////////////////////////////////////////////////////////////////

void BMC_Die::Reset()
{
	BMC_DieData::Reset();

	m_state = BME_STATE_NOTUSED;
	m_value_total = 0;
	m_sides_max = 0;
}

void BMC_Die::RecomputeAttacks()
{
	BM_ASSERT(m_state!=BME_STATE_NOTSET);

	// by default vulnerable to all
	m_vulnerabilities.SetAll();

	m_attacks.Clear();

	// POWER and SKILL - by default for all dice
	m_attacks.Set(BME_ATTACK_POWER);
	m_attacks.Set(BME_ATTACK_SKILL);

	// Handle specific dice types

	// UNSKILLED [The Flying Squirrel]
	// TODO: does WARRIOR override this? (currently)
	if (HasProperty(BME_PROPERTY_UNSKILLED))
		m_attacks.Clear(BME_ATTACK_SKILL);

	// SPEED
	if (HasProperty(BME_PROPERTY_SPEED))
		m_attacks.Set(BME_ATTACK_SPEED);

	// TRIP
	if (HasProperty(BME_PROPERTY_TRIP))
		m_attacks.Set(BME_ATTACK_TRIP);

	// SHADOW
	if (HasProperty(BME_PROPERTY_SHADOW))
	{
		m_attacks.Set(BME_ATTACK_SHADOW);
		m_attacks.Clear(BME_ATTACK_POWER);
	}

	// KONSTANT
	if (HasProperty(BME_PROPERTY_KONSTANT))
		m_attacks.Clear(BME_ATTACK_POWER);

	// BERSERK
	if (HasProperty(BME_PROPERTY_BERSERK))
	{
		m_attacks.Set(BME_ATTACK_BERSERK);
		m_attacks.Clear(BME_ATTACK_SKILL);
	}

	// stealth dice
	if (HasProperty(BME_PROPERTY_STEALTH))
	{
		m_attacks.Clear(BME_ATTACK_POWER);
		m_vulnerabilities.Clear();
		m_vulnerabilities.Set(BME_ATTACK_SKILL);
	}

	// WARRIOR 
	// - cannot be attacked
	// - can only skill attack (with non-warrior)
	if (HasProperty(BME_PROPERTY_WARRIOR))
	{
		m_vulnerabilities.Clear();
		m_attacks.Clear();
		m_attacks.Set(BME_ATTACK_SKILL);
	}

	// queer dice
	if (HasProperty(BME_PROPERTY_QUEER))
	{
		if (m_value_total % 2 == 1)	// odd means acts as a shadow die
		{
			m_attacks.Set(BME_ATTACK_SHADOW);
			m_attacks.Clear(BME_ATTACK_POWER);
		}
	}

	// FOCUS: if dizzy no attacks
	if (m_state == BME_STATE_DIZZY)
		m_attacks.Clear();

	// verification

	// MORPHING shouldn't be allowed to SPEED
	BM_ERROR( !HasProperty(BME_PROPERTY_MORPHING) || !m_attacks.IsSet(BME_ATTACK_SPEED) );
	
	// MORPHING shouldn't be paired with TURBO
	BM_ERROR( (m_properties & (BME_PROPERTY_MORPHING|BME_PROPERTY_TURBO)) != (BME_PROPERTY_MORPHING|BME_PROPERTY_TURBO) );

	// MORPHING shouldn't be paired with TWIN
	BM_ERROR( (m_properties & (BME_PROPERTY_MORPHING|BME_PROPERTY_TWIN)) != (BME_PROPERTY_MORPHING|BME_PROPERTY_TWIN) );
}

// POST: all data related to current value have not been set up
void BMC_Die::SetDie(BMC_DieData *_data)
{
	// dead?
	if (!_data || !_data->Valid())
	{
		m_state = BME_STATE_NOTUSED;
		return;
	}

	// first do default copy constructor to set up base class 
	*(BMC_DieData*)this = *_data;

	// now set up state
	m_state = BME_STATE_NOTSET;

	// set up actual die
	INT i;
	m_value_total = 0;
	m_sides_max = 0;
	for (i=0; i<Dice(); i++)
	{
		if (GetSwingType(i) != BME_SWING_NOT)
			m_sides[i] = 0;
		else
			m_sides_max += m_sides[i];
		//m_value[i] = 0;
	}

	// set up attacks/vulnerabilities
	//RecomputeAttacks();
}

void BMC_Die::OnUseReserve()
{
	BM_ASSERT(HasProperty(BME_PROPERTY_RESERVE));
	BM_ASSERT(m_state==BME_STATE_RESERVE);

	m_properties &= ~BME_PROPERTY_RESERVE;
	m_state = BME_STATE_NOTSET;
}

void BMC_Die::OnDizzyRecovered()
{
	m_state = BME_STATE_READY;

	RecomputeAttacks();
}

void BMC_Die::OnSwingSet(INT _swing, U8 _value)
{
	if (!IsUsed())
		return;

	BM_ASSERT(m_state=BME_STATE_NOTSET);

	INT i;
	m_sides_max = 0;
	for (i=0; i<Dice(); i++)
	{
		if (GetSwingType(i) == _swing)
			m_sides[i] = _value;
		m_sides_max += m_sides[i];
	}
}

// POST: m_sides[0] is swapped with the desired die
void BMC_Die::SetOption(INT _d)
{
	if (_d != 0)
	{
		INT temp = m_sides[0];
		m_sides[0] = m_sides[_d];
		m_sides[_d] = temp;
	}
	m_sides_max = m_sides[0];
}

void BMC_Die::SetFocus(INT _v)
{
	BM_ASSERT(Dice()==1);
	BM_ASSERT(_v<=m_sides[0]);

	m_value_total = _v;

	m_state = BME_STATE_DIZZY;

	RecomputeAttacks();
}

void BMC_Die::Roll()
{
	if (!IsUsed())
		return;

	BM_ASSERT(m_state=BME_STATE_NOTSET);

	INT i;
	m_value_total = 0;
	for (i=0; i<Dice(); i++)
	{
		BM_ASSERT(i>0 || m_sides[i]>0);	// swing die hasn't been set
		if (m_sides[i]==0)
			break;
		// WARRIOR, MAXIMUM: always rolls highest number
		if (HasProperty(BME_PROPERTY_WARRIOR) || HasProperty(BME_PROPERTY_MAXIMUM))
			m_value_total += m_sides[i];
		else
			m_value_total += g_rng.GetRand(m_sides[i])+1;
	}

	m_state = BME_STATE_READY;

	RecomputeAttacks();
}

float BMC_Die::GetScore(bool _own)
{
	// WARRIOR and NULL: worth 0
	if (HasProperty(BME_PROPERTY_NULL|BME_PROPERTY_WARRIOR))
	{
		return 0;
	}

	else if (HasProperty(BME_PROPERTY_POISON))
	{
		if (_own)
			return -m_sides_max * 1.0f;
		else
			return -m_sides_max * 0.5f;	// this is not an optional rule
	}

	else
	{
		if (_own)
			return m_sides_max * BMD_VALUE_OWN_DICE;	// scoring owned dice is optional
		else
			return m_sides_max;
	}
}

// DESC: deal with all deterministic effects of attacking, including TURBO
// PARAM: _actually_attacking is normally true, but may be false for forced attack rerolls like ORNERY
void BMC_Die::OnApplyAttackPlayer(BMC_Move &_move, BMC_Player *_owner, bool _actually_attacking)
{
    // TODO determine how this may conflict with the design of `bool _actually_attacking`
    BM_ASSERT(g_attack_type[_move.m_attack]!=BME_ATTACK_TYPE_0_0);

	// clear value
	// KONSTANT: don't reroll
	if (!HasProperty(BME_PROPERTY_KONSTANT))
		SetState(BME_STATE_NOTSET);

	// BERSERK attack - half size and round fractions up
	if (_actually_attacking && _move.m_attack == BME_ATTACK_BERSERK)
	{
		BM_ASSERT(g_attack_type[_move.m_attack]==BME_ATTACK_TYPE_1_N);
		BM_ASSERT(Dice()==1);

		_owner->OnDieSidesChanging(this);
		m_sides[0] = (m_sides[0]+1) / 2;
		m_sides_max = m_sides[0];
		m_properties &= ~BME_PROPERTY_BERSERK;
		_owner->OnDieSidesChanged(this);
	}

	if (GetState()==BME_STATE_NOTSET)
		OnBeforeRollInGame(_owner);

	// MORPHING - assume same size as target
	// drp022521 - this rule only applies if actually going to capture a die
	if (HasProperty(BME_PROPERTY_MORPHING) && _move.m_attack != BME_ATTACK_INVALID)
	{
		BM_ASSERT(g_attack_type[_move.m_attack]==BME_ATTACK_TYPE_1_1 || g_attack_type[_move.m_attack]==BME_ATTACK_TYPE_N_1);
		BMC_Player *target = _move.m_game->GetPlayer(_move.m_target_player);

		BM_ASSERT(Dice()==1);

		_owner->OnDieSidesChanging(this);
		m_sides_max = m_sides[0] = target->GetDie(_move.m_target)->GetSidesMax();
		_owner->OnDieSidesChanged(this);
	}

	// TURBO dice 
	if (HasProperty(BME_PROPERTY_TURBO))
	{
		// TURBO OPTION - no change if sticking with '0'
		if (HasProperty(BME_PROPERTY_OPTION) &&  _move.m_turbo_option==1)
		{
			_owner->OnDieSidesChanging(this);
			SetOption(_move.m_turbo_option);
			_owner->OnDieSidesChanged(this);
		}
		// TURBO SWING
		else if (_move.m_turbo_option>0)
		{
			BM_ASSERT(GetSwingType(0)!=BME_SWING_NOT);
			_owner->OnDieSidesChanging(this);
			_owner->SetSwingDice(GetSwingType(0), _move.m_turbo_option, true);
			_owner->OnDieSidesChanged(this);
		}
	}


	// WARRIOR: loses property 
	// TODO: KONSTANT will be forced to reroll but shouldn't (OnDieSidesChanging misinterpreting situation)
	if (HasProperty(BME_PROPERTY_WARRIOR))
	{
		_owner->OnDieSidesChanging(this);
		m_properties &= ~BME_PROPERTY_WARRIOR;
		_owner->OnDieSidesChanged(this);
	}
}

// DESC: effects that happen whenever rerolling die (for whatever reason)
void BMC_Die::OnBeforeRollInGame(BMC_Player *_owner)
{
	// MIGHTY
	if (HasProperty(BME_PROPERTY_MIGHTY))
	{
		_owner->OnDieSidesChanging(this);
		m_sides_max = 0;
		for (int d=0; d<Dice(); d++)
		{
			if (m_sides[d]>=20)
				m_sides_max += (m_sides[d] = 30);
			else 
				m_sides_max += (m_sides[d] = g_mighty_sides[m_sides[d]]);
		}
		_owner->OnDieSidesChanged(this);
	}

	// WEAK
	if (HasProperty(BME_PROPERTY_WEAK))
	{
		_owner->OnDieSidesChanging(this);
		m_sides_max = 0;
		for (int d=0; d<Dice(); d++)
		{
			if (m_sides[d]>30)
				m_sides_max += (m_sides[d] = 30);
			else if (m_sides[d]>20)
				m_sides_max += (m_sides[d] = 20);
			else if (m_sides[d]==20)
				m_sides_max += (m_sides[d] = 16);
			else 
				m_sides_max += (m_sides[d] = g_weak_sides[m_sides[d]]);
		}
		_owner->OnDieSidesChanged(this);
	}
}

void BMC_Die::OnApplyAttackNatureRollAttacker(BMC_Move &_move, BMC_Player *_owner)
{
	INT i;

	// MOOD dice
	if (HasProperty(BME_PROPERTY_MOOD))
	{
		_owner->OnDieSidesChanging(this);
		m_sides_max = 0;
		INT delta;
		INT swing;
		for (i=0; i<Dice(); i++)
		{
			swing = GetSwingType(i);
			switch (swing)
			{
			case BME_SWING_X:
				m_sides[i] = g_mood_sides_X[g_rng.GetRand(BMD_MOOD_SIDES_RANGE_X)];
				break;
			case BME_SWING_V:
				m_sides[i] = g_mood_sides_V[g_rng.GetRand(BMD_MOOD_SIDES_RANGE_V)];
				break;
			default:
				// some BM use MOOD SWING on other than X and V
				delta = g_swing_sides_range[swing][1] - g_swing_sides_range[swing][0];
				m_sides[i] = g_rng.GetRand(delta+1)+ g_swing_sides_range[swing][0];
				break;
			}
			m_sides_max += m_sides[i];

		}
		_owner->OnDieSidesChanged(this);
	}

	// reroll
	Roll();
}

void BMC_Die::OnApplyAttackNatureRollTripped()
{
	// reroll
	Roll();
}

void BMC_Die::Debug(BME_DEBUG _cat)
{
	BMC_DieData::Debug(_cat);

	printf("%d:%d ", m_sides_max, m_value_total);
}

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_MovePool
///////////////////////////////////////////////////////////////////////////////////////////

/*
// TODO: assert on m_pool_index
// TODO: use bits on m_available
class BMC_MovePool
{
public:
	BMC_MovePool() 
	{
		m_pool.resize(128);
		m_available.resize(128, true);
		m_next_available = 0;
	}

	BMC_Move *			New()
	{
		CheckSize();
		while (!m_available[m_next_available])
		{
			m_next_available++;
			CheckSize();
		}

		m_available[m_next_available] = false;
		m_pool[m_next_available].m_pool_index = m_next_available;
		return &(m_pool[m_next_available++]);
	}

	void				Release(BMC_Move *_move)
	{
		if (_move->m_pool_index<0)
			return;
		m_available[_move->m_pool_index] = true;
		_move->m_pool_index = -1;
	}


private:

	void	CheckSize()
	{
		BM_ASSERT(m_available.size()==m_pool.size());
		if (m_next_available>=m_available.size())
		{
			m_next_available = 0;
			INT grow = m_available.size() * 2;
			m_available.resize( grow );
			m_pool.resize( grow );
		}
	}

	std::vector<BMC_Move>		m_pool;
	std::vector<bool>	m_available;
	INT					m_next_available;
};

BMC_MovePool	g_move_pool;
*/

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_MoveList methods
///////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Move methods
///////////////////////////////////////////////////////////////////////////////////////////

void BMC_Move::Debug(BME_DEBUG _cat, const char *_postfix)
{
	if (!g_logger.IsLogging(_cat))
		return;

	INT i;
	BMC_Player *phaser = m_game->GetPhasePlayer();

	printf("%s ", g_action_name[m_action]);

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
	 				printf("%s %d  ", g_swing_name[i], m_swing_value[i]); 
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

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Game methods
///////////////////////////////////////////////////////////////////////////////////////////

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
		if (m_player[m_phase_player].GetTotalSwingDice(i)>0 && g_swing_sides_range[i][0]>0)
		{
			// check range
			if (_move.m_swing_value[i]<g_swing_sides_range[i][0] || _move.m_swing_value[i]>g_swing_sides_range[i][1])
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
			BM_ASSERT(_move.m_swing_value[swing_type]>=g_swing_sides_range[swing_type][0] && _move.m_swing_value[swing_type]<=g_swing_sides_range[swing_type][1]);
			for (INT s=0; s<swing_type; s++)
			{
				if (	m_player[m_phase_player].GetTotalSwingDice(s)>0 
					&&	g_swing_sides_range[s][0]>0
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
		if (pl->GetTotalSwingDice(i)>0 && g_swing_sides_range[i][0]>0)
		{
			swing_action[actions].swing = true;
			swing_action[actions].index = i;
			swing_action[actions].value = g_swing_sides_range[i][0];	// initialize to min
			actions++;
			//combinations *= (g_swing_sides_range[i][1]-g_swing_sides_range[i][0])+1;
			move.m_swing_value[i] = g_swing_sides_range[i][0];
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
				max = g_swing_sides_range[swing_action[p].index][1];
			else
				max = 1;
			if (swing_action[p].value <= max)
				break;

			// we are past max - reset to min and cycle the next value
			if (swing_action[p].swing)
			{
				swing_action[p].value = g_swing_sides_range[swing_action[p].index][0];
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

			switch (g_attack_type[move.m_attack])
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
						if (die_stack.ContainsAllDice() && die_stack.GetValueTotal()<=target->GetMaxValue())
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
						if (die_stack.ContainsAllDice() && att_total>=die_stack.GetValueTotal())
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
			switch (g_attack_type[attack->m_attack])
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
				INT min = g_swing_sides_range[swing][0];
				INT max = g_swing_sides_range[swing][1];

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
				float step_size = (g_turbo_accuracy<=0) ? 1000 : 1 / g_turbo_accuracy;
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
		if (m_player[m_phase_player].GetTotalSwingDice(i)>0 && g_swing_sides_range[i][0]>0)
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
	switch (g_attack_type[_move.m_attack])
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
		BM_ASSERT(g_attack_type[_move.m_attack]==BME_ATTACK_TYPE_1_1);

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
	switch (g_attack_type[_move.m_attack])
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
		BM_ASSERT(g_attack_type[_move.m_attack]==BME_ATTACK_TYPE_1_1);

		tgt_die = target->GetDie(_move.m_target);
		tgt_die->OnApplyAttackNatureRollTripped();
	}
}

// DESC: handle non-deterministic and capture-specific effects.  This includes
// TIME_AND_SPACE, TRIP, NULL 
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
	INT i;

	// capture - attacker effects
	switch (g_attack_type[_move.m_attack])
	{
	case BME_ATTACK_TYPE_1_1:
	case BME_ATTACK_TYPE_1_N:
		{
			att_die = attacker->GetDie(_move.m_attacker);
			null_attacker = att_die->HasProperty(BME_PROPERTY_NULL);

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
		BM_ASSERT(g_attack_type[_move.m_attack]==BME_ATTACK_TYPE_1_1);

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
		switch (g_attack_type[_move.m_attack])
		{
		case BME_ATTACK_TYPE_N_1:
		case BME_ATTACK_TYPE_1_1:
			{
				tgt_die = target->OnDieLost(_move.m_target);
				if (null_attacker)
				{
					tgt_die->SetState(BME_STATE_NULLIFIED);
				}
				else
				{
					tgt_die->SetState(BME_STATE_CAPTURED);
					attacker->OnDieCaptured(tgt_die);
				}
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
					{
						tgt_die->SetState(BME_STATE_NULLIFIED);
					}
					else
					{
						tgt_die->SetState(BME_STATE_CAPTURED);
						attacker->OnDieCaptured(tgt_die);
					}
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

///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Parser methods
///////////////////////////////////////////////////////////////////////////////////////////

INT BMC_Parser::ParseDieNumber(INT & _pos)
{
	INT value = atoi(line + _pos);
	while (DieIsValue(line[++_pos]))	// skip past number
	{	
	}
	return value;
}

INT BMC_Parser::ParseDieDefinedSides(INT _pos)
{
	while (_pos < (INT)std::strlen(line) && line[_pos] != '-')
		_pos++;

	if (line[_pos]=='-')
		return ParseDieNumber(++_pos);

	return 0;
}

// TODO: p->m_swing_value[BME_SWING_MAX] if known at this stage
// TODO: p->m_swing_set
// PRE: if this is _die 1, then _die 0 has already been parsed
// POST: sets m_swing_type[], m_sides[], m_sides_max, and m_swing_set
void BMC_Parser::ParseDieSides(INT & _pos, INT _die)
{
	if (DieIsSwing(line[_pos]))
	{
		BME_SWING	swing_type = (BME_SWING)(BME_SWING_FIRST + (line[_pos] - BMD_FIRST_SWING_CHAR));
		d->m_swing_type[_die] =  swing_type;
		d->m_sides[_die] = 0;
		p->m_swing_set = BMC_Player::SWING_SET_NOT;
		_pos++;

		// check if swing sides have been defined - NOTE this does not move _pos
		INT sides = ParseDieDefinedSides(_pos);
		if (sides>0)
		{
			d->m_sides[_die] = sides;
			d->m_sides_max += sides;
			p->m_swing_value[swing_type] = sides;
			p->m_swing_set = BMC_Player::SWING_SET_LOCKED;
		}
	}
	else
	{
		d->m_swing_type[_die] = BME_SWING_NOT;
		d->m_sides[_die] = ParseDieNumber(_pos);
		d->m_sides_max += d->m_sides[_die];
	}
}

// PRE: line is already filled
// TODO: deal with option and twin dice
void BMC_Parser::ParseDie(INT _p, INT _die)
{
	BM_ERROR(_die<BMD_MAX_DICE);

	p = g_game.GetPlayer(_p);
	d = p->GetDie(_die);

	// save original index
	d->m_original_index = _die;

	// DIE DEFINITION:
	// - m_sides[2], m_properties, m_swing_type[2]

	// prefix properties
	INT pos = 0;
	d->m_properties = BME_PROPERTY_VALID;
	while (!DieIsValue(line[pos]) && !DieIsTwin(line[pos]))
	{
		U8 ch = line[pos++];
		#define	DEFINE_PROPERTY(_s, _v)	case _s: d->m_properties |= _v; break;

		// WARNING: don't use SWING dice here (PQRSTUVWXYZ)
		switch (ch)
		{
			DEFINE_PROPERTY('z', BME_PROPERTY_SPEED)
			DEFINE_PROPERTY('^', BME_PROPERTY_TIME_AND_SPACE)
			DEFINE_PROPERTY('+', BME_PROPERTY_AUXILIARY)
			DEFINE_PROPERTY('q', BME_PROPERTY_QUEER)
			DEFINE_PROPERTY('t', BME_PROPERTY_TRIP)
			DEFINE_PROPERTY('s', BME_PROPERTY_SHADOW)
			DEFINE_PROPERTY('d', BME_PROPERTY_STEALTH)
			DEFINE_PROPERTY('p', BME_PROPERTY_POISON)
			DEFINE_PROPERTY('n', BME_PROPERTY_NULL)
			DEFINE_PROPERTY('B', BME_PROPERTY_BERSERK)
			DEFINE_PROPERTY('f', BME_PROPERTY_FOCUS)
			DEFINE_PROPERTY('H', BME_PROPERTY_MIGHTY)
			DEFINE_PROPERTY('h', BME_PROPERTY_WEAK)
			DEFINE_PROPERTY('r', BME_PROPERTY_RESERVE)
			DEFINE_PROPERTY('o', BME_PROPERTY_ORNERY)
			DEFINE_PROPERTY('D', BME_PROPERTY_DOPPLEGANGER)
			DEFINE_PROPERTY('c', BME_PROPERTY_CHANCE)
			DEFINE_PROPERTY('m', BME_PROPERTY_MORPHING)
			DEFINE_PROPERTY('%', BME_PROPERTY_RADIOACTIVE)
			DEFINE_PROPERTY('`', BME_PROPERTY_WARRIOR)
			DEFINE_PROPERTY('w', BME_PROPERTY_SLOW)
			DEFINE_PROPERTY('u', BME_PROPERTY_UNIQUE)
			DEFINE_PROPERTY('~', BME_PROPERTY_UNSKILLED)
			DEFINE_PROPERTY('g', BME_PROPERTY_STINGER)
			DEFINE_PROPERTY('G', BME_PROPERTY_RAGE)
			DEFINE_PROPERTY('k', BME_PROPERTY_KONSTANT)
			DEFINE_PROPERTY('M', BME_PROPERTY_MAXIMUM)
		default:
			BMF_Error("error parsing die %s (pre-fix property at %c)", line, ch);
		}
	}

	// parse sides
	// TWIN dice
	d->m_sides_max = 0;
	if (DieIsTwin(line[pos]))
	{
		pos++;
		d->m_properties |= BME_PROPERTY_TWIN;
		ParseDieSides(pos, 0);
		if (line[pos++]!=',')
			BMF_Error("Error parsing twin die");
		ParseDieSides(pos, 1);
		if (line[pos++]!=')')
			BMF_Error("Error parsing twin die");
	}
	else
	{
		ParseDieSides(pos, 0);

		// OPTION dice
		if (DieIsOption(line[pos]))
		{
			pos++;
			d->m_properties |= BME_PROPERTY_OPTION;
			p->m_swing_set = BMC_Player::SWING_SET_NOT;
			ParseDieSides(pos, 1);
			// the previous call added to m_sides_max, so correct m_sides_max back to the # on the first die (by default)
			d->m_sides_max = d->m_sides[0];
			// check for defined sides - NOTE this does not move pos
			INT sides = ParseDieDefinedSides(pos);
			if (sides>0)
			{
				BM_ASSERT(sides==d->m_sides[0] || sides==d->m_sides[1]);
				if (sides==d->m_sides[1])
				{
					d->SetOption(1);
				}
				p->m_swing_set = BMC_Player::SWING_SET_LOCKED;
			} // end if sides are defined for OPTION
		} // end if OPTION die
	}

	// postfix properties
	while (pos < (INT)std::strlen(line) && line[pos] != ':')
	{
		U8 ch = line[pos++];
		#define	DEFINE_POST_PROPERTY(_s, _v)	case _s: d->m_properties |= _v; break;

		switch (ch)
		{
			DEFINE_POST_PROPERTY('!', BME_PROPERTY_TURBO)
			DEFINE_POST_PROPERTY('?', BME_PROPERTY_MOOD)
		// if we spot a '-', then we have already parsed the defined sides for a SWING or OPTION,
		// so skip the number
		case '-':
			ParseDieNumber(pos);
			break;
		default:
			BMF_Error("error parsing die %s (post-fix property at %c)", line, ch);
		}	
	}

	// state
	if (d->HasProperty(BME_PROPERTY_RESERVE))
		d->m_state = BME_STATE_RESERVE;
	else
		d->m_state = BME_STATE_NOTSET;

	// actual value
	if (line[pos]==':')
	{
		pos++;
		if (!DieIsNumeric(line[pos]))
			BMF_Error("error parsing die %s (expecting current value at %c)", line, line[pos]);

		BM_ASSERT(d->m_state == BME_STATE_NOTSET);	// die marked as RESERVE shouldn't be in play
		d->m_state = BME_STATE_READY;
		d->m_value_total = atoi(line + pos);
		while (DieIsValue(line[++pos]))	// skip past number
		{	
		}

		// FOCUS: dizzy?
		if (line[pos]=='d')
		{
			pos++;
			d->m_state = BME_STATE_DIZZY;
		}
	}

	// did we successfully determine what this die is?
	if (pos != std::strlen(line))
		BMF_Error( "Could not successfully parse die: %s (broken at '%c')", line, line[pos]);

	// set up attacks/vulnerabilities
	if (d->m_state==BME_STATE_READY)
		d->RecomputeAttacks();
}

void BMC_Parser::ParsePlayer(INT _p, INT _dice)
{
	BMC_Player *p = g_game.GetPlayer(_p);

	p->Reset();

	// TODO: float	m_score;

	INT i;
	for (i=0; i<_dice; i++)
	{
		Read();
		ParseDie(_p,i);
	}

	p->OnDiceParsed();
}

// USAGE: game [wins] 
// wins is optional, default is 3
void BMC_Parser::ParseGame()
{
	INT wlt,i;

	// parse wins required
	if (sscanf(line, "game %d", &i)==1)
	{
		BMF_Log(BME_DEBUG_ALWAYS, "target wins set to %d\n", i);
		g_game.m_target_wins = i;	
	}

	// TODO: parse standings
	for (wlt=0; wlt < BME_WLT_MAX; wlt++)
		g_game.m_standing[wlt] = 0;

	// parse phase
	Read();
	g_game.m_phase = BME_PHASE_MAX;
	for (i=0; i<BME_PHASE_MAX; i++)
	{
		if (!std::strcmp(g_phase_name[i], line))
			g_game.m_phase = (BME_PHASE)i;
	}
	if (g_game.m_phase == BME_PHASE_MAX)
		BMF_Error( "phase not found");

	// parse players
	for (i=0; i<BMD_MAX_PLAYERS; i++)
	{
		Read();
		INT p, dice;
		F32	score;
		if (sscanf(line, "player %d %d %f", &p, &dice, &score)<3 || p!=i)
			BMF_Error( "missing player: %s", line );
		ParsePlayer(p, dice);
		// don't clobber score in INITIATIVE phases
		if (g_game.GetPhase()!=BME_PHASE_INITIATIVE && g_game.GetPhase()!=BME_PHASE_INITIATIVE_CHANCE && g_game.GetPhase()!=BME_PHASE_INITIATIVE_FOCUS)
			g_game.GetPlayer(p)->m_score = score;
		// if this is still preround, then print all dice (i.e. including those not ready)
		if (g_game.IsPreround())
			g_game.GetPlayer(p)->DebugAllDice();
		else
			g_game.GetPlayer(p)->Debug();
	}

	// set up AI
	g_game.SetAI(0, &g_ai);
	g_game.SetAI(1, &g_ai);
}

BMC_Parser::BMC_Parser()
{
	file = stdin;
}

bool BMC_Parser::Read(bool _fatal)
{
	if (!fgets(line, BMD_MAX_STRING, file))
	{
		if (_fatal)
			BMF_Error( "missing input" );
		return false;
	}

	// remove EOL
	INT len = (INT)std::strlen(line);
	if (len>0 && line[len-1]=='\n')
		line[len-1] = 0;

	return true;
}

void BMC_Parser::Send( const char *_fmt, ... )
{
	va_list	ap;
	va_start(ap, _fmt), 
	vprintf( _fmt, ap );
	va_end(ap);	
}

void BMC_Parser::SendStats()
{
	extern float s_ply_decay;
	printf("stats %d/%d-%d/%d/%.2f ", g_ai.GetMaxPly(), g_ai.GetMinSims(), g_ai.GetMaxSims(), g_ai.GetMaxBranch(), s_ply_decay);
	g_stats.DisplayStats();
}

// NOTE: this parallels ApplySetSwing()
void BMC_Parser::SendSetSwing(BMC_Move &_move)
{
	if (_move.m_action == BME_ACTION_PASS)
	{
		Send("pass\n");
		return;
	}

	BM_ASSERT(_move.m_action == BME_ACTION_SET_SWING_AND_OPTION);

	INT i;

	// check swing dice
	BMC_Player *p = _move.m_game->GetPlayer(_move.m_game->m_phase_player);

	for (i=0; i<BME_SWING_MAX; i++)
	{
		if (p->GetTotalSwingDice(i)>0 && g_swing_sides_range[i][0]>0)
			Send("swing %c %d\n", BMD_FIRST_SWING_CHAR + i - BME_SWING_FIRST, _move.m_swing_value[i]);
	}

	// check option dice
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (!p->GetDie(i)->IsUsed())
			continue;

		if (p->GetDie(i)->HasProperty(BME_PROPERTY_OPTION))
			Send("option %d %d\n", p->GetDie(i)->GetOriginalIndex(), p->GetDie(i)->GetSides(_move.m_option_die[i]));
	}
}

void BMC_Parser::SendUseReserve(BMC_Move &_move)
{
	if (_move.m_action == BME_ACTION_PASS)
	{
		Send("reserve -1\n");
		return;
	}

	BM_ASSERT(_move.m_action==BME_ACTION_USE_RESERVE);

	BMC_Player *p = _move.m_game->GetPlayer(_move.m_game->m_phase_player);

	Send("reserve %d\n", p->GetDie(_move.m_use_reserve)->GetOriginalIndex());
}

void BMC_Parser::SendUseChance(BMC_Move &_move)
{
	if (_move.m_action == BME_ACTION_PASS)
	{
		Send("pass\n");
		return;
	}

	BM_ASSERT(_move.m_action==BME_ACTION_USE_CHANCE);

	BMC_Player *p = _move.m_game->GetPlayer(_move.m_game->m_phase_player);

	INT i;
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		if (_move.m_chance_reroll.IsSet(i))
			Send("chance %d\n", p->GetDie(i)->GetOriginalIndex());
	}
}

void BMC_Parser::SendUseFocus(BMC_Move &_move)
{
	if (_move.m_action == BME_ACTION_PASS)
	{
		Send("pass\n");
		return;
	}

	BM_ASSERT(_move.m_action==BME_ACTION_USE_FOCUS);

	BMC_Player *p = _move.m_game->GetPlayer(_move.m_game->m_phase_player);

	INT i;
	for (i=0; i<BMD_MAX_DICE; i++)
	{
		BMC_Die *d = p->GetDie(i);

		if (!d->IsUsed() || !d->HasProperty(BME_PROPERTY_FOCUS))
			continue;

		if (_move.m_focus_value[i]>0)
			Send("focus %d %d\n", d->GetOriginalIndex(), _move.m_focus_value[i]);
	}
}

// NOTE: this parallels ApplyAction()
void BMC_Parser::SendAttack(BMC_Move &_move)
{
	if (_move.m_action == BME_ACTION_PASS)
	{
		Send("pass\n");
		return;
	}

	if (_move.m_action == BME_ACTION_SURRENDER)
	{
		Send("surrender\n");
		return;
	}

	BM_ASSERT(_move.m_action == BME_ACTION_ATTACK);

	// send attack type
	Send("%s\n", g_attack_name[_move.m_attack]);

	/// ensure m_game is correct
	_move.m_game = &g_game;

	BMC_Player *attacker = _move.m_game->GetPhasePlayer();
	BMC_Player *target = _move.m_game->GetTargetPlayer();
	BMC_Die		*att_die, *tgt_die;
	INT i;

	// send attackers
	switch (g_attack_type[_move.m_attack])
	{
	case BME_ATTACK_TYPE_1_1:
	case BME_ATTACK_TYPE_1_N:
		{
			att_die = attacker->GetDie(_move.m_attacker);
			Send("%d\n", att_die->GetOriginalIndex());
			break;
		}
	case BME_ATTACK_TYPE_N_1:
		{
			int sent = 0;
			for (i=0; i<attacker->GetAvailableDice(); i++)
			{
				if (!_move.m_attackers.IsSet(i))
					continue;
				att_die = attacker->GetDie(i);
				if (sent++ > 0)
					Send(" ");
				Send("%d", att_die->GetOriginalIndex());
			}
			Send("\n");
			break;
		}
	}

	// send defenders
	switch (g_attack_type[_move.m_attack])
	{
	case BME_ATTACK_TYPE_N_1:
	case BME_ATTACK_TYPE_1_1:
		{
			tgt_die = target->GetDie(_move.m_target);
			Send("%d\n", tgt_die->GetOriginalIndex());
			break;
		}
	case BME_ATTACK_TYPE_1_N:
		{
			int sent = 0;
			for (i=0; i<target->GetAvailableDice(); i++)
			{
				if (!_move.m_targets.IsSet(i))
					continue;
				tgt_die = target->GetDie(i);
				if (sent++ > 0)
					Send(" ");
				Send("%d", tgt_die->GetOriginalIndex());
			}
			Send("\n");
			break;
		}
	}

	// send TURBO
	if (_move.m_turbo_option>=0)
	{
		INT i;
		for (i=0; i<attacker->GetAvailableDice(); i++)
		{
			att_die = attacker->GetDie(i);
			if (!att_die->HasProperty(BME_PROPERTY_TURBO))
				continue;

			if (att_die->HasProperty(BME_PROPERTY_OPTION))
			{	
				Send("option %d %d\n", att_die->GetOriginalIndex(), att_die->GetSides(_move.m_turbo_option));
				break;
			}
			else // TURBO SWING
			{
				Send("swing %c %d\n", BMD_FIRST_SWING_CHAR + att_die->GetSwingType(0) - BME_SWING_FIRST, _move.m_turbo_option);
				break;
			}
		}
	}
}

void BMC_Parser::GetAction()
{
	if (g_game.m_phase == BME_PHASE_PREROUND)
	{
		INT i = 0;
		BMC_Move move;
		g_game.m_phase_player = i;	// setup phase player for the AI
		g_game.m_ai[i]->GetSetSwingAction(&g_game, move);

		SendStats();

		Send("action\n");
		SendSetSwing(move);
	}
	else if (g_game.m_phase == BME_PHASE_RESERVE)
	{
		INT i = 0;
		BMC_Move move;
		g_game.m_phase_player = i;	// setup phase player for the AI
		g_game.m_ai[i]->GetReserveAction(&g_game, move);

		SendStats();

		Send("action\n");
		SendUseReserve(move);
	}
	else if (g_game.m_phase == BME_PHASE_FIGHT)
	{
		g_game.m_phase_player = 0;
		g_game.m_target_player = 1;
		BMC_Move move;
		g_game.m_ai[0]->GetAttackAction(&g_game, move);

		SendStats();

		Send("action\n");
		SendAttack(move);
	}
	else if (g_game.m_phase == BME_PHASE_INITIATIVE_CHANCE)
	{
		g_game.m_phase_player = 0;
		g_game.m_target_player = 1;
		BMC_Move move;
		g_game.m_ai[0]->GetUseChanceAction(&g_game, move);

		SendStats();

		Send("action\n");
		SendUseChance(move);
	}
	else if (g_game.m_phase == BME_PHASE_INITIATIVE_FOCUS)
	{
		g_game.m_phase_player = 0;
		g_game.m_target_player = 1;
		BMC_Move move;
		g_game.m_ai[0]->GetUseFocusAction(&g_game, move);

		SendStats();

		Send("action\n");
		SendUseFocus(move);
	}
	else
		BMF_Error("GetAction(): Unrecognized phase");

	//Send("endaction\n");
}

void BMC_Parser::PlayGame(INT _games)
{
	if (g_game.GetPhase()!=BME_PHASE_PREROUND)
		BMF_Error("Cannot PlayGame unless it is preround");

	INT wins[2] = {0,0};

	BMC_Game	g_sim(false);
	while (_games-->0)
	{
		g_sim = g_game;
		g_sim.PlayGame();
		if (g_sim.GetStanding(0) > g_sim.GetStanding(1))
			wins[0]++;
		else
			wins[1]++;
	}

	BMF_Log(BME_DEBUG_ALWAYS, "matches over %d - %d\n", wins[0], wins[1]);
}

void BMC_Parser::CompareAI(INT _games)
{
	if (g_game.GetPhase()!=BME_PHASE_PREROUND)
		BMF_Error("Cannot PlayGame unless it is preround");

	INT wins[2] = {0,0};

	BMC_Game	g_sim(false);
	while (_games-->0)
	{
		g_sim = g_game;
		g_sim.PlayGame();
		if (g_sim.GetStanding(0) > g_sim.GetStanding(1))
			wins[0]++;
		else
			wins[1]++;
	}

	BMF_Log(BME_DEBUG_ALWAYS, "matches over %d - %d\n", wins[0], wins[1]);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// evaluating fairness
///////////////////////////////////////////////////////////////////////////////////////////////

// AI globals - for fairness test

BMC_AI					g_ai_mode0;
BMC_AI_Maximize			g_ai_mode1;
BMC_AI_MaximizeOrRandom	g_ai_mode1b;
BMC_BMAI				g_ai_mode2;
BMC_BMAI				g_ai_mode3;


void BMC_AI_MaximizeOrRandom::GetAttackAction(BMC_Game *_game, BMC_Move &_move)
{
	if (g_rng.GetFRand()<p)
		g_ai_mode1.GetAttackAction(_game, _move);
	else
		g_ai_mode0.GetAttackAction(_game, _move);
}

// DESC: written for Zomulgustar fair testing fairness
// PARAMS:
//	_mode 0 = random action
//	_mode 1 = simple maximizer
//	_mode 2 = BMAI using Maximizer/Random as QAI
//	_mode 3 = BMAI using QAI
//	_p = probability (0..1) that Maximizer/Random uses "Maximize", it will use random with probability (1-_p)
void BMC_Parser::PlayFairGames(INT _games, INT _mode, F32 _p)
{
	if (g_game.GetPhase()!=BME_PHASE_PREROUND)
		BMF_Error("Cannot PlayGame unless it is preround");

	INT wins[2][2] = {0, };	// indexes: [got_initiative] [winner]
	INT p, i;				// p here is player ID

	// setup AIs
	g_ai_mode1b.SetP(_p);
	g_ai_mode2.SetQAI(&g_ai_mode1b);
	g_ai_mode3.SetQAI(&g_qai);

	// set ply of BMAI according to whatever the "ply" command was
	g_ai_mode2.SetMaxPly(g_ai.GetMaxPly());
	g_ai_mode3.SetMaxPly(g_ai.GetMaxPly());

	for (p=0; p<2; p++)
	{
		if (_mode==0)
			g_game.SetAI(p, &g_ai_mode0);
		else if (_mode==1)
			g_game.SetAI(p, &g_ai_mode1);
		else if (_mode==2)
			g_game.SetAI(p, &g_ai_mode2);
		else if (_mode==3)
			g_game.SetAI(p, &g_ai_mode3);
	}

	// play games
	INT g = 0;
	BMC_Game	g_sim(false);
	for (g=0; g<_games; g++)
	{
		g_sim = g_game;
		g_sim.PlayGame();
		if (g_sim.GetStanding(0) > g_sim.GetStanding(1))
			wins[g_sim.GetInitiativeWinner()][0]++;
		else
			wins[g_sim.GetInitiativeWinner()][1]++;
	}

	BMF_Log(BME_DEBUG_ALWAYS, "PlayFairGames: %d games, mode %d, p %f\n", _games, _mode, _p);

	for (p=0; p<2; p++)
	{
		for (i=p; i<2 && i>=0; i += (p>0) ? -1 : +1)
		{
			INT w = wins[i][p];
			INT l = wins[i][!p];
			INT g = w + l;
			BMF_Log(BME_DEBUG_ALWAYS, "P%d stats: initiative P%d games %d wins %d losses %d percent %.1f%%\n",
				p, i, g, w, l, (float)w*100/g );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
// BMC_Parser::Parse
// DESC: this is the main interface for the AI
///////////////////////////////////////////////////////////////////////////////////////////////

/*
PARSER COMMANDS

SETUP
game %1				begin parsing game setup, setting %1 as the target number of wins [%1 is an optional field, default 3]

SETTINGS
seed %1				seed the RNG, use 0 to randomize it  [default is deterministic each run]
max_sims %1			number of rollout simulations BMAI will use [default 500]
min_sims %1			when applying 'maxbranch' rules, min_sims BMAI will use [default 10]
turbo_accuracy %1	how many turbo options to consider, where 1 means consider all valid turbo options and 0 means consider only the extremes [range 0..1, default 1]
ply %1				deepest ply for BMAI to run at (uses simulations after that ply)
maxbranch %1		maximum number of total simulations to run at a ply (valid moves * simulations) [default 5000]
debug %1 %2			adjust logging settings (e.g. "debug SIMULATION 0")
debugply %1
ai %1 %2			set player %1 (0-1) to AI type %2 (0 = BMAI, 1 = QAI, 2 = BMAI v2)
surrender %1        set if AI is allowed to surrender. If off then AI will continue to play loosing positions. [default is on]

ACTIONS
playgame %1			play %1 games and output results 
compare %1			play %1 games and output results of current AI vs OLD AI
playfair %1 %2 %3	play %1 games, using 'mode' %2, and 'p' %3
getaction			ask BMAI for what action it would select in the given situation
quit				terminate (same as EOF)

GAME SETUP FORMAT	use these commands after a 'game' command
%1					the phase (preround, fight, reserve, ...)
player 0 %2 %3		begin player setup, 0 = player number (0 or 1), %2 = number of dice to follow, %3 = score
%1					first die (e.g. "20:11", "!X-4:3", "2/6-6", "rz18", ...)
%1					second die, and so on
player 1 %2 %3		repeat for the second player
%1					first die
%1					second die, and so on
*/

void BMC_Parser::Parse()
{
	INT	param, param2;
	F32	fparam;
	char sparam[BMD_MAX_STRING+1];
	while (Read(false))	// non-fatal Read()
	{
		// game [wins]
		if (!std::strncmp(line, "game", 4))
		{
			ParseGame();
		}
		else if (sscanf(line, "playgame %d", &param)==1)
		{
			PlayGame(param);			
		}
		else if (sscanf(line, "compare %d", &param)==1)
		{
			CompareAI(param);
		}
		// playfair [games] [mode] [p]
		else if (sscanf(line, "playfair %d %d %f", &param, &param2, &fparam)==3)
		{
			PlayFairGames(param, param2, fparam);
		}
		// ai [player] [type]
		else if (sscanf(line, "ai %d %d", &param, &param2)==2)
		{
			if (param2<0 || param2>=BMD_AI_TYPES)
				BMF_Error("invalid setting for ai type: %d", param2);
			if (param<0 || param>1)
				BMF_Error("invalid setting for ai player number: %d", param);
			g_game.SetAI(param, g_ai_type[param2]);
			printf("Setting AI for player %d to type %d\n", param, param2);
		}
		else if (sscanf(line, "max_sims %d %d", &param, &param2)==2)
		{
			BMC_AI * ai = g_game.GetAI(param);
			if (ai->IsBMAI())
			{
				((BMC_BMAI*)ai)->SetMaxSims(param2);
				printf("Setting max sims for player %d to %d\n", param, param2);
			}
		}
		else if (sscanf(line, "max_sims %d", &param)==1)
		{
			g_ai.SetMaxSims(param);
			printf("Setting max # simulations to %d\n", param);
		}
		else if (sscanf(line, "min_sims %d %d", &param, &param2)==2)
		{
			BMC_AI * ai = g_game.GetAI(param);
			if (ai->IsBMAI())
			{
				((BMC_BMAI*)ai)->SetMinSims(param2);
				printf("Setting min sims for player %d to %d\n", param, param2);
			}
		}
		else if (sscanf(line, "min_sims %d", &param)==1)
		{
			g_ai.SetMinSims(param);
			printf("Setting min # simulations to %d\n", param);
		}
		else if (sscanf(line, "turbo_accuracy %f", &fparam)==1)
		{
			g_turbo_accuracy = fparam;
			printf("Setting turbo accuracy to %f\n", g_turbo_accuracy);
		}
		else if (sscanf(line, "ply %d %d", &param, &param2)==2)
		{
			BMC_AI * ai = g_game.GetAI(param);
			if (ai->IsBMAI())
			{
				((BMC_BMAI*)ai)->SetMaxPly(param2);
				printf("Setting max ply for player %d to %d\n", param, param2);
			}
		}
		else if (sscanf(line, "ply %d", &param)==1)
		{
			g_ai.SetMaxPly(param);
			printf("Setting max ply to %d\n", param);
		}
		else if (sscanf(line, "debugply %d", &param)==1)
		{
			BMC_BMAI::SetDebugLevel(param);
			printf("Setting debug ply to %d\n", param);
		}
		else if (sscanf(line, "maxbranch %d %d", &param, &param2)==2)
		{
			BMC_AI * ai = g_game.GetAI(param);
			if (param>=0 && param<BMD_AI_TYPES && ai->IsBMAI())
			{
				((BMC_BMAI*)ai)->SetMaxBranch(param2);
				printf("Setting max branch for player %d to %d\n", param, param2);
			}
		}
		else if (sscanf(line, "maxbranch %d", &param)==1)
		{
			g_ai.SetMaxBranch(param);
			printf("Setting max branch to %d\n", param);
		}
		else if (!std::strcmp(line, "getaction"))
		{
			GetAction();
		}
		// PRE: magic # (32) must be < BMD_MAX_STRING and >= largest g_debug_name[] name
		else if (sscanf(line, "debug %32s %d" ,&sparam,&param)==2)
		{
			g_logger.SetLogging(sparam,param);
		}
		else if (sscanf(line, "seed %d", &param)==1)
		{
			g_rng.SRand(param);
			printf("Seeding with %d\n", param);
		}
        else if (sscanf(line, "surrender %32s", &sparam)==1)
        {
            g_game.SetSurrenderAllowed(std::string(sparam)=="on");
        }
		else if (!std::strcmp(line, "quit"))
		{
			return;
		}
		else if (line[0]==0)
		{
			;	// blank line
		}
		else
		{
			BMF_Error("unrecognized command: %s\n", line);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////////////////

int bmai_main(int argc, char *argv[])
{
	g_stats.OnAppStarted();

	// set up logging
	// - disable in release build (for ZOM)
	// drp051401 - reenabled in RELEASE since using for BMAI
#ifndef _DEBUG	
	//g_logger.SetLogging(BME_DEBUG_SIMULATION, false);
	g_logger.SetLogging(BME_DEBUG_BMAI, false);
#endif
	g_logger.SetLogging(BME_DEBUG_ROUND, false);
	g_logger.SetLogging(BME_DEBUG_QAI, false);

	//g_ai_mode1b.SetP(0.5);
	//g_ai.SetQAI(&g_ai_mode0);

	// banner
	printf("BMAI: the Button Men AI\nCopyright (c) 2001-2023, Denis Papp.\nFor information, contact Denis Papp, denis@accessdenied.net\nVersion: %s\n", GIT_DESCRIBE);

	if (argc>1)
	{
		FILE *fp = fopen(argv[1],"r");
		if (!fp)
		{
			printf("could not open %s", argv[1]);
			return 1;
		}
		printf("Reading from %s\n", argv[1]); 
		g_parser.Parse(fp);
		fclose(fp);
	}
	else
	{
		g_parser.Parse();
	}

	//g_stats.DisplayStats();

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////
// extinct code
////////////////////////////////////////////////////////////////////////////////////////////


