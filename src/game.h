///////////////////////////////////////////////////////////////////////////////////////////
// game.h
// Copyright (c) 2001-2023 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: main game logic class
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
///////////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include "player.h"

class BMC_Game		// 420b
{
	friend class BMC_Parser;

public:
	BMC_Game(bool _simulation);
	BMC_Game(const BMC_Game & _game);
	const BMC_Game & operator=(const BMC_Game &_game);

	// game simulation - level 0
	void		PlayGame(BMC_Man *_man1 = NULL, BMC_Man *_man2 = NULL);

	// game simulation - level 1 (for simulations)
	BME_WLT		PlayRound(BMC_Move *_start_action = NULL);

	// game methods
	bool		ValidAttack(BMC_MoveAttack &_move);
	void		GenerateValidAttacks(BMC_MoveList &_movelist);

	bool		ValidSetSwing(BMC_Move &_move);
	void		GenerateValidSetSwing(BMC_MoveList & _movelist);
	void		ApplySetSwing(BMC_Move &_move, bool _lock = true);

	bool		ValidUseFocus(BMC_Move &_move);
	void		GenerateValidFocus(BMC_MoveList & _movelist);
	void		ApplyUseFocus(BMC_Move &_move);

	void		ApplyUseReserve(BMC_Move &_move);

	bool		ValidUseChance(BMC_Move &_move);
	void		GenerateValidChance(BMC_MoveList & _movelist);
	void		ApplyUseChance(BMC_Move &_move);

	// initiative
	INT			CheckInitiative();

	// managing fights
	bool		FightOver();
	void		ApplyAttackPlayer(BMC_Move &_move);
	void		ApplyAttackNatureRoll(BMC_Move &_move);
	void		ApplyAttackNaturePost(BMC_Move &_move, bool &_extra_turn);
	void		SimulateAttack(BMC_MoveAttack &_move, bool & _extra_turn);
	void		RecoverDizzyDice(INT _player);

	// accessors
	BMC_Player *GetPlayer(INT _i) { return &m_player[_i]; }
	BMC_Player *GetPhasePlayer() { return GetPlayer(m_phase_player); }
	BMC_Player *GetTargetPlayer() { return GetPlayer(m_target_player); }
	INT			GetPhasePlayerID() { return m_phase_player; }
	bool		IsPreround() { return m_phase == BME_PHASE_PREROUND || m_phase == BME_PHASE_RESERVE; }
	BME_PHASE	GetPhase() { return m_phase; }
	INT			GetStanding(INT _wlt) { return m_standing[_wlt]; }
	INT			GetInitiativeWinner() { return m_initiative_winner; }
	bool		IsSimulation() { return m_simulation; }
	BMC_AI *	GetAI(INT _p) { return m_ai[_p]; }

	// mutators
	void		SetAI(INT _p, BMC_AI *_ai) { m_ai[_p] = _ai; }

	// methods wrt. "percent chance to win"
	float		ConvertWLTToWinProbability();
	float		PlayFight_EvaluateMove(INT _pov_player, BMC_Move &_move);
	float		PlayRound_EvaluateMove(INT _pov_player);

protected:
	// game simulation - level 1
	void		Setup(BMC_Man *_man1 = NULL, BMC_Man *_man2 = NULL);

	// game simulation - level 2
	void		PlayPreround();
	void		PlayInitiative();
	void		PlayInitiativeChance();
	void		PlayInitiativeFocus();
	void		PlayFight(BMC_Move *_start_action = NULL);
	void		FinishPreround();
	void		FinishInitiative();
	void		FinishInitiativeChance(bool _swap_phase_player);
	void		FinishInitiativeFocus(bool _swap_phase_player);
	BME_WLT		FinishRound(BME_WLT _wlt_0);

	// game simulation - level 3
	void		FinishTurn(bool extra_turn = false);

private:
	BMC_Player	m_player[BMD_MAX_PLAYERS];
	U8			m_standing[BME_WLT_MAX];
	U8			m_target_wins;
	BME_PHASE	m_phase;
	U8			m_phase_player;
	U8			m_target_player;
	BME_ACTION	m_last_action;

	// AI players
	BMC_AI *	m_ai[BMD_MAX_PLAYERS];

	// stats accumulated during the round
	U8			m_initiative_winner;	// the player who originally won initiative (ignoring CHANCE and FOCUS)

	// is this a simulation run by the AI?
	bool		m_simulation;
};