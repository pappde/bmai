///////////////////////////////////////////////////////////////////////////////////////////
// bmai.cpp
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
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
// dbl052623	- main method shim allows us to split a library from an executable
//				  so we can link to the testing libraries
// dbl053123	- moved TestRNG and SetupTestGame to a `./test/` dir and testing framework
// dbl053123	- added ability to disallow surrenders
// dbl100824	- migrating most logic out into their own classes allows this to be the main method shim
//				  and the other classes can be linked for testing
//				- created many BMC_ .cpp/.h files
//				- adjusted many globals which helped narrow down the #include dependency tree
///////////////////////////////////////////////////////////////////////////////////////////

// TODO:
// - reenable have_turbo check
// - 092103 BUG: the game had a small chance of ending in a tie but
//   BMAI reported 100% win

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
#include "bmai_lib.h"

#include <cstdio>
#include "BMC_Logger.h"
#include "BMC_Parser.h"
#include "BMC_Stats.h"

///////////////////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
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
	printf("BMAI: the Button Men AI\nCopyright (c) 2001-2024, Denis Papp.\nFor information, contact Denis Papp, denis@accessdenied.net\nVersion: %s\n", GIT_DESCRIBE);

	BMC_Parser	parser;

	if (argc>1)
	{
		FILE *fp = fopen(argv[1],"r");
		if (!fp)
		{
			printf("could not open %s", argv[1]);
			return 1;
		}
		printf("Reading from %s\n", argv[1]); 
		parser.Parse(fp);
		fclose(fp);
	}
	else
	{
		parser.Parse();
	}

	//g_stats.DisplayStats();

	return 0;
}
