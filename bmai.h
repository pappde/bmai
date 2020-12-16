///////////////////////////////////////////////////////////////////////////////////////////
// bmai.h
// Copyright (c) 2001 Denis Papp. All rights reserved.
// denis@accessdenied.net
// http://www.accessdenied.net/bmai
// 
// DESC: main header for BMAI
//
// REVISION HISTORY:
///////////////////////////////////////////////////////////////////////////////////////////


#ifndef __BMAI_H__
#define __BMAI_H__

// dependent includes (and high-level)
#include <windows.h>
#include <assert.h>
#include <vector>

// forward declarations
class BMC_Player;
class BMC_Game;
class BMC_AI;

// typedefs
typedef unsigned long	U32;
typedef unsigned char	U8;
typedef signed char		S8;
typedef unsigned short	U16;
typedef int				INT;
typedef unsigned int	UINT;
typedef float			F32;

// assert
#define BM_ASSERT	assert
#define BM_ERROR(check)	{ if (!(check)) { BMF_Error(#check); } }

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

	// accessors
	U8	IsSet(INT _bit)	{ INT byte = _bit/8; return bits[byte] & (1<<(_bit-byte*8)); }

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

// utilities

class BMC_RNG
{
public:
	UINT	GetRand(UINT _i)	{ return rand() % _i; }
	F32		GetFRand()			{ return (float)rand() / (float)0x8000; }
	void	SRand(UINT _seed);
};

// settings
#define BMD_MAX_TWINS	2
#define BMD_MAX_DICE	10
#define BMD_MAX_PLAYERS	2
#define BMD_DEFAULT_WINS	3
#define	BMD_VALUE_OWN_DICE	0.5f
#define BMD_SURRENDERED_SCORE	-1000
#define BMD_MAX_STRING	256
#define BMD_DEFAULT_WINS	3

// debug categories
enum BME_DEBUG
{
	BME_DEBUG_ALWAYS,
	BME_DEBUG_WARNING,
	BME_DEBUG_PARSER,
	BME_DEBUG_SIMULATION,
	BME_DEBUG_ROUND,
	BME_DEBUG_GAME,
	BME_DEBUG_QAI,
	BME_DEBUG_MAX
};

// all the different "actions" that can be submitted 
enum BME_ACTION
{
	xBME_ACTION_USE_AUXILIARY,
	BME_ACTION_USE_CHANCE,
	BME_ACTION_USE_FOCUS,
	BME_ACTION_SET_SWING_AND_OPTION,
	BME_ACTION_USE_RESERVE,
	BME_ACTION_ATTACK,
	BME_ACTION_PASS,
	BME_ACTION_SURRENDER,
	BME_ACTION_MAX
};

// attack-related dice are specially indicated
// D = attack defining stage
// P = apply player attack stage
// N = apply nature attack stage
enum BME_PROPERTY
{
	BME_PROPERTY_TIME_AND_SPACE	=	0x0001, // (N) post-attack effect
	BME_PROPERTY_AUXILIARY		=	0x0002, // pre-game action
	BME_PROPERTY_QUEER			=	0x0004, // attacks
	BME_PROPERTY_TRIP			=	0x0008, // (N) attacks, special attack
	BME_PROPERTY_SPEED			=	0x0010, // attacks
	BME_PROPERTY_SHADOW			=	0x0020, // attacks
	BME_PROPERTY_BERSERK		=	0x0040, // attacks
	BME_PROPERTY_STEALTH		=	0x0080, // attacks
	BME_PROPERTY_POISON			=	0x0100, // score modifier
	BME_PROPERTY_NULL			=	0x0200,	// (N) capture modifier
	BME_PROPERTY_MOOD			=	0x0400,	// (N) post-attack effect
	BME_PROPERTY_TURBO			=	0x0800,	// (DP) post-attack action
	BME_PROPERTY_OPTION			=	0x1000,	// swing, uses two dice
	BME_PROPERTY_TWIN			=	0x2000,	// uses two dice
	BME_PROPERTY_FOCUS			=	0x4000,	// initiative action
	BME_PROPERTY_VALID			=	0x8000,	// without this, it isn't a valid die!
	BME_PROPERTY_MIGHTY			=  0x10000,	// (P) pre-roll effect
	BME_PROPERTY_WEAK			=  0x20000,	// (P) pre-roll effect
	BME_PROPERTY_RESERVE		=  0x40000,	// preround (after a loss)
	BME_PROPERTY_ORNERY			=  0x80000,	// (N) post-attack effect on all dice
	BME_PROPERTY_DOPPLEGANGER	= 0x100000,	// (P) post-attack effect
	BME_PROPERTY_CHANCE			= 0x200000, // initiative action
	BME_PROPERTY_MORPHING		= 0x400000, // (P) post-attack effect
	BME_PROPERTY_RADIOACTIVE	= 0x800000, // (P) post-attack effect
	BME_PROPERTY_MAX
};

enum BME_SWING
{
	BME_SWING_NOT,
	BME_SWING_R,
	BME_SWING_S,
	BME_SWING_T,
	BME_SWING_U,
	BME_SWING_V,
	BME_SWING_W,
	BME_SWING_X,
	BME_SWING_Y,
	BME_SWING_Z,
	BME_SWING_MAX
};

#define	BMD_FIRST_SWING_CHAR	'R'
#define BMD_LAST_SWING_CHAR		'Z'

enum BME_STATE
{
	BME_STATE_READY,
	BME_STATE_NOTSET,
	BME_STATE_CAPTURED,
	BME_STATE_PARALYZED,	// focus dice - one turn paralysis
	BME_STATE_NULLIFIED,
	BME_STATE_NOTUSED,		// this die (slot) isn't being used at all this game
	BME_STATE_RESERVE,		
	BME_STATE_MAX
};

enum BME_WLT
{
	BME_WLT_WIN,
	BME_WLT_LOSS,
	BME_WLT_TIE,
	BME_WLT_MAX
};

enum BME_PHASE
{
	BME_PHASE_PREROUND,
	BME_PHASE_RESERVE,				// part of PREROUND
	BME_PHASE_INITIATIVE,			
	BME_PHASE_INITIATIVE_CHANCE,	// part of INITIATIVE
	BME_PHASE_INITIATIVE_FOCUS,		// part of INITIATIVE
	BME_PHASE_FIGHT,
	BME_PHASE_GAMEOVER,
	BME_PHASE_MAX
};

enum BME_ATTACK
{
	BME_ATTACK_FIRST = 0,
	BME_ATTACK_POWER = BME_ATTACK_FIRST,	// 1 -> 1
	BME_ATTACK_SKILL,						// N -> 1
	BME_ATTACK_BERSERK,						// N -> 1
	BME_ATTACK_SPEED,						// 1 -> N
	BME_ATTACK_TRIP,						// 1 -> 1
	BME_ATTACK_SHADOW,						// 1 -> 1
	BME_ATTACK_MAX
};

enum BME_ATTACK_TYPE
{
	BME_ATTACK_TYPE_1_1,
	BME_ATTACK_TYPE_N_1,
	BME_ATTACK_TYPE_1_N,
	BME_ATTACK_TYPE_MAX
};

// global definitions

extern BME_ATTACK_TYPE	g_attack_type[BME_ATTACK_MAX];
extern INT g_swing_sides_range[BME_SWING_MAX][2];

// globals

extern class BMC_Logger	g_logger;
extern class BMC_BMAI	g_ai;
extern class BMC_QAI	g_qai;

///////////////////////////////////////////////////////////////////////////////////////////
// classes
///////////////////////////////////////////////////////////////////////////////////////////

class BMC_Move
{
public:
	//BMC_MoveAttack() : BMC_Move(BME_ACTION_ATTACK) {}
	BMC_Move() : m_pool_index(-1)	{}

	// methods
	void	Debug(BME_DEBUG _cat = BME_DEBUG_ALWAYS, const char *_postfix = "\n");
	BOOL	operator<  (const BMC_Move &_m) const  { return this < &_m; }
	BOOL	operator==	(const BMC_Move &_m) const { return this == &_m; }

	// accessors
	BOOL	MultipleAttackers() { return g_attack_type[m_attack]==BME_ATTACK_TYPE_N_1; }
	BOOL	MultipleTargets()	{ return g_attack_type[m_attack]==BME_ATTACK_TYPE_1_N; }
	BMC_Player *	GetAttacker(); 
	BMC_Player *	GetTarget(); 

	// data
	BME_ACTION					m_action;
	INT							m_pool_index;
	BMC_Game					*m_game;
	union
	{
		// BME_ACTION_ATTACK
		struct {
			BME_ATTACK					m_attack;
			BMC_BitArray<BMD_MAX_DICE>	m_attackers;
			BMC_BitArray<BMD_MAX_DICE>	m_targets;
			S8							m_attacker;
			S8							m_target;
			U8							m_attacker_player;
			U8							m_target_player;
			S8							m_turbo_option;	// only supports one die, 0/1 for which sides to go with. -1 means none
		};

		// BME_ACTION_SET_SWING_AND_OPTION,
		struct {
			U8			m_swing_value[BME_SWING_MAX];	
			U8			m_option_die[BMD_MAX_DICE];	// use die 0 or 1
		};

		// BME_ACTION_USE_CHANCE
		struct {
			BMC_BitArray<BMD_MAX_DICE>	m_chance_reroll;
		};

		// BME_ACTION_USE_FOCUS,
		struct {
			U8			m_focus_value[BMD_MAX_DICE];
		};

		// BME_ACTION_USE_RESERVE (use ACTION_PASS to use none)
		struct {
			U8			m_use_reserve;
		};
	};
};

#define	BMC_MoveAttack	BMC_Move

class BMC_MoveSetSwingAndOption
{
public:
};

typedef std::vector<BMC_Move *>	BMC_MoveVector;

class BMC_MoveList
{
public:
			BMC_MoveList();
			~BMC_MoveList() { Clear(); }
	void	Clear();
	void	Add(BMC_Move  & _move );
	INT		Size() { return list.size(); }
	BMC_Move *	Get(INT _i) { return list[_i]; }
	BOOL	Empty() { return Size()<1; }

protected:
private:
	BMC_MoveVector	list;
};

class TGC_MovePower
{
public:
protected:
private:
	U8	m_attacker;
	U8	m_target;
};

class TGC_MoveSkill
{
public:
protected:
private:
	BMC_BitArray<BMD_MAX_DICE>	m_attackers;
	U8	m_target;
};

class TGC_MoveSpeed
{
public:
protected:
private:
	U8	m_attacker;
	BMC_BitArray<BMD_MAX_DICE>	m_targets;
};

// NOTES:
// - second die only used for Option and Twin
class BMC_DieData
{
	friend class BMC_Parser;

public:
				BMC_DieData();
	virtual void Reset();

	// methods
	void		Debug(BME_DEBUG _cat = BME_DEBUG_ALWAYS);

	// accessors
	BOOL		HasProperty(INT _p)  { return m_properties & _p; }
	BME_SWING	GetSwingType(INT _d) { return (BME_SWING)m_swing_type[_d]; }
	BOOL		Valid()				 { return (m_properties & BME_PROPERTY_VALID); }
	BOOL		Dice()				 { return (m_properties & BME_PROPERTY_TWIN) ? 2 : 1; }
	INT			GetSides(INT _t)	{ return m_sides[_t]; }

protected:
	U32			m_properties;
	U8			m_sides[BMD_MAX_TWINS];			// number of sides if not a swing die, or current number of sides

private:
	U8			m_swing_type[BMD_MAX_TWINS];	// definition number of sides, should be BME_SWING
};

class BMC_Die : public BMC_DieData
{
	friend		class BMC_Parser;

public:
	// setup
	virtual void Reset();
	void		SetDie(BMC_DieData *_data);
	void		Roll();
	void		GameRoll(BMC_Player *_owner);

	// methods
	void		Debug(BME_DEBUG _cat = BME_DEBUG_ALWAYS);
	void		SetOption(INT _d);

	// accessors
	BOOL		CanDoAttack(BMC_MoveAttack &_move)	{ return m_attacks.IsSet(_move.m_attack); }
	BOOL		CanBeAttacked(BMC_MoveAttack &_move)	{ return m_vulnerabilities.IsSet(_move.m_attack); }
	INT			GetValueTotal()					{ return m_value_total; }
	INT			GetSidesMax()					{ return m_sides_max; }
	BOOL		IsAvailable()					{ return m_state == BME_STATE_READY; }
	BOOL		IsInReserve()						{ return m_state == BME_STATE_RESERVE; }
	BOOL		IsUsed()						{ return m_state != BME_STATE_NOTUSED && m_state!=BME_STATE_RESERVE; }
	float		GetScore(BOOL _own);
	INT			GetOriginalIndex()				{ return m_original_index; }

	// mutators
	void		SetState(BME_STATE _state)		{ m_state = _state; }

	// events
	void		OnDieChanged();
	void		OnSwingSet(INT _swing, U8 _value);
	void		OnApplyAttackPlayer(BMC_Move &_move, BMC_Player *_owner, BOOL _actually_attacking=TRUE);
	void		OnApplyAttackNatureRollAttacker(BMC_Move &_move, BMC_Player *_owner);
	void		OnApplyAttackNatureRollTripped();
	void		OnBeforeRollInGame(BMC_Player *_owner);
	void		OnUseReserve();

protected:

	// call this once state changes
	void		RecomputeAttacks();

private:
	U8			m_state;				// should be BME_STATE
	U8			m_value_total;			// current total value of all dice, redundant
	U8			m_sides_max;			// max value of dice (based on m_sides), redundant
	U8			m_original_index;		// save original index for interface
	BMC_BitArray<BME_ATTACK_MAX>	m_attacks;
	BMC_BitArray<BME_ATTACK_MAX>	m_vulnerabilities;
	//U8			m_value[BMD_MAX_TWINS];	// current value of dice, not really necessary (and often not known!)
};

class BMC_Man
{
public:
	// accessors
	BMC_DieData *GetDieData(INT _d) { return &m_die[_d]; }

protected:
private:
	BMC_DieData	m_die[BMD_MAX_DICE];
};

class BMC_Player	// 204b
{
	friend class BMC_Parser;

public:
				BMC_Player();
	void		SetID(INT _id) { m_id = _id; }			
	void		Reset();				
	void		OnDiceParsed();

	// playing with normal rules
	void		SetButtonMan(BMC_Man *_man);
	void		SetSwingDice(INT _swing, U8 _value, BOOL _from_turbo=FALSE);
	void		SetOptionDie(INT _i, INT _d);
	void		RollDice();

	// methods
	void		Debug(BME_DEBUG _cat = BME_DEBUG_ALWAYS);
	void		DebugAllDice(BME_DEBUG _cat = BME_DEBUG_ALWAYS);

	// events
	void		OnDieSidesChanging(BMC_Die *_die);
	void		OnDieSidesChanged(BMC_Die *_die);
	BMC_Die *	OnDieLost(INT _d);
	void		OnDieCaptured(BMC_Die *_die);
	void		OnRoundLost();
	void		OnSurrendered();
	void		OnSwingDiceSet() { m_swing_set = TRUE; }
	void		OnAttackFinished() { OptimizeDice(); }
	void		OnDieTripped() { OptimizeDice(); }
	void		OnChanceDieRolled() { OptimizeDice(); }

	// accessors
	BMC_Die *	GetDie(INT _d) { return &m_die[_d]; }
	INT			GetAvailableDice() { return m_available_dice; }
	INT			GetMaxValue() { return m_max_value; }
	float		GetScore() { return m_score; }
	BOOL		SwingDiceSet() { return m_swing_set; }
	INT			HasDieWithProperty(INT _p, BOOL _check_all_dice=FALSE);
	INT			GetTotalSwingDice(INT _s) { return m_swing_dice[_s]; }


protected:
	// methods
	void		OptimizeDice();

private:
	BMC_Man	*	m_man;
	INT			m_id;
	BOOL		m_swing_set;
	BMC_Die		m_die[BMD_MAX_DICE];			// as long as Optimize was called, these are sorted largest to smallest that are READY
	U8			m_swing_value[BME_SWING_MAX];	
	U8			m_swing_dice[BME_SWING_MAX];	// number of dice of each swing type
	INT			m_available_dice;				// only valid after Optimize
	INT			m_max_value;					// only valid after Optimize, useful to know for skill attacks
	float		m_score;
};

class BMC_Game		// 420b
{
	friend class BMC_Parser;

public:
				BMC_Game(BOOL _simulation);
				BMC_Game(const BMC_Game & _game);
	const BMC_Game & operator=(const BMC_Game &_game);

	// game simulation - level 0
	void		PlayGame(BMC_Man *_man1 = NULL, BMC_Man *_man2 = NULL);

	// game simulation - level 1 (for simulations)
	BME_WLT		PlayRound(BMC_Move *_start_action=NULL);

	// game methods
	BOOL		ValidAttack(BMC_MoveAttack &_move);
	void		GenerateValidAttacks(BMC_MoveList &_movelist);

	BOOL		ValidSetSwing(BMC_Move &_move);
	void		GenerateValidPreround(BMC_MoveList & _movelist);
	void		ApplySetSwing(BMC_Move &_move);

	BOOL		ValidUseFocus(BMC_Move &_move);
	void		GenerateValidFocus(BMC_MoveList & _movelist);
	void		ApplyUseFocus(BMC_Move &_move);

	void		ApplyUseReserve(BMC_Move &_move);

	BOOL		ValidUseChance(BMC_Move &_move);
	void		ApplyUseChance(BMC_Move &_move);

	// initiative
	INT			CheckInitiative();

	// managing fights
	BOOL		FightOver();
	void		ApplyAttackPlayer(BMC_Move &_move);
	void		ApplyAttackNatureRoll(BMC_Move &_move);
	void		ApplyAttackNaturePost(BMC_Move &_move, BOOL &_extra_turn);
	void		SimulateAttack(BMC_MoveAttack &_move, BOOL & _extra_turn);

	// accessors
	BMC_Player *GetPlayer(INT _i) { return &m_player[_i]; }
	BMC_Player *GetPhasePlayer() { return GetPlayer(m_phase_player); }
	BMC_Player *GetTargetPlayer() { return GetPlayer(m_target_player); }
	INT			GetPhasePlayerID() { return m_phase_player; }
	BOOL		IsPreround() { return m_phase == BME_PHASE_PREROUND || m_phase==BME_PHASE_RESERVE; }
	BME_PHASE	GetPhase() { return m_phase; }
	INT			GetStanding(INT _wlt) { return m_standing[_wlt]; }
	INT			GetInitiativeWinner() { return m_initiative_winner; }
	BOOL		IsSimulation() { return m_simulation; }

	// mutators
	void		SetAI(INT _p, BMC_AI *_ai) { m_ai[_p] = _ai; }

protected:
	// game simulation - level 1
	void		Setup(BMC_Man *_man1 = NULL, BMC_Man *_man2 = NULL);

	// game simulation - level 2
	void		PlayPreround();
	void		PlayInitiative();
	void		PlayInitiativeChance();
	void		PlayInitiativeFocus();
	void		PlayFight(BMC_Move *_start_action=NULL);
	void		FinishPreround();
	void		FinishInitiative();
	void		FinishInitiativeChance(BOOL _swap_phase_player);
	BME_WLT		FinishRound(BME_WLT _wlt_0);

	// game simulation - level 3
	void		FinishTurn(BOOL extra_turn=FALSE);

private:
	BMC_Player	m_player[BMD_MAX_PLAYERS];
	U8			m_standing[BME_WLT_MAX];
	U8			m_target_wins;
	BME_PHASE	m_phase;
	U8			m_phase_player;
	U8			m_target_player;

	// AI players
	BMC_AI *	m_ai[BMD_MAX_PLAYERS];

	// stats accumulated during the round
	U8			m_initiative_winner;	// the player who originally won initiative (ignoring CHANCE and FOCUS)

	// is this a simulation run by the AI?
	BOOL		m_simulation;
};

// basic AI class
class BMC_AI
{
public:

	virtual void	GetSetSwingAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetAttackAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetReserveAction(BMC_Game *_game, BMC_Move &_move);		
	virtual void	GetUseChanceAction(BMC_Game *_game, BMC_Move &_move);
	void		GetUseFocusAction(BMC_Game *_game, BMC_Move &_move);

	void		GetUseAuxiliaryAction(BMC_Game *_game, BMC_Move &_move);
	void		GetSetTurboAction(BMC_Game *_game, BMC_Move &_move);

	F32			ScoreAttack(BMC_Game *_game, BMC_Move &_move);

protected:
private:
	BMC_AI *	m_qai;
};

// The real BMAI
class BMC_BMAI : public BMC_AI
{
public:
				BMC_BMAI();

	virtual void	GetAttackAction(BMC_Game *_game, BMC_Move &_move);
	virtual	void	GetSetSwingAction(BMC_Game *_game, BMC_Move &_move);
	virtual void	GetReserveAction(BMC_Game *_game, BMC_Move &_move);		
	virtual void	GetUseChanceAction(BMC_Game *_game, BMC_Move &_move);

	//////////////////////////////////////////
	// mutators
	//////////////////////////////////////////

	void		SetQAI(BMC_AI *_ai) { m_qai = _ai; }		// use this to change the QAI used in simulations
	void		SetMaxPly(INT _m)	{ m_max_ply = _m; }
	void		SetMaxBranch(INT _m) { m_max_branch = _m; }

	//////////////////////////////////////////
	// accessors
	//////////////////////////////////////////

	INT			GetMaxPly() { return m_max_ply; }

protected:
private:

	// determining number of sims to use
	INT			ComputeNumberSims(INT _moves);

	// for dealing with 'level'
	void		OnStartEvaluation(INT &_enter_level);
	void		OnPreSimulation(BMC_Game &_sim);
	void		OnPostSimulation(BMC_Game *_game, INT _enter_level);
	void		OnEndEvaluation(BMC_Game *_game, INT _enter_level);

	BMC_AI *	m_qai;
	INT			m_max_ply;
	INT			m_max_branch;
};

// Heuristic Quick AI that BMAI uses by default
class BMC_QAI : public BMC_AI
{
public:
	virtual void		GetAttackAction(BMC_Game *_game, BMC_Move &_move);


protected:
private:
};

// Selects the maximizing move
class BMC_AI_Maximize : public BMC_AI
{
public:
	virtual void		GetAttackAction(BMC_Game *_game, BMC_Move &_move);
};

// Randomly selects between a basic AI and a maximizing AI
// P(p) use Maximize (mode 1), else Basic (mode 0)
class BMC_AI_MaximizeOrRandom : public BMC_AI
{
public:
	virtual void		GetAttackAction(BMC_Game *_game, BMC_Move &_move);

	void	SetP(F32 _p) { p = _p; }

private:
	F32	p;
};


class BMC_AI_Mode2 : public BMC_AI
{
};


// INVARIANT: only contains dice in increasing order (e.g. 0,2,3 but not 2.0,3)				
class BMC_DieIndexStack
{
public:
	BMC_DieIndexStack(BMC_Player *_owner);

	// mutators
	void			Pop()	{ die_stack_size--; }
	void			Push(INT _index);
	BOOL			Cycle(BOOL _add_die=TRUE);

	// methods 
	void			SetBits(BMC_BitArray<BMD_MAX_DICE> & _bits);
	void			Debug(BME_DEBUG _cat = BME_DEBUG_ALWAYS);

	// accessors
	INT				GetValueTotal() { return value_total; }
	BOOL			ContainsAllDice() { return die_stack_size == owner->GetAvailableDice(); }
	BOOL			ContainsLastDie() { return GetTopDieIndex() == owner->GetAvailableDice()-1; }
	INT				GetTopDieIndex() { BM_ASSERT(die_stack_size>0); return die_stack[die_stack_size-1]; }
	BMC_Die *		GetTopDie()		{ return owner->GetDie(GetTopDieIndex()); }
	BOOL			Empty()			{ return die_stack_size == 0; }
	INT				GetStackSize()	{ return die_stack_size; }

protected:
private:
	BMC_Player *	owner;
	INT				die_stack[BMD_MAX_DICE];
	INT				die_stack_size;
	INT				value_total;
};

class BMC_Parser
{
public:
	void	SetupTestGame();
	void	ParseGame();
	void	Parse();

protected:
	void			GetAction();
	void			PlayGame(INT _games);
	void			PlayFairGames(INT _games, INT _mode, F32 _p);
	void			ParseDie(INT _p, INT _dice);
	void			ParsePlayer(INT _p, INT _dice);
	BOOL			Read(BOOL _fatal=TRUE);

	// output
	void			Send(char *_fmt, ...);
	void			SendSetSwing(BMC_Move &_move);
	void			SendUseReserve(BMC_Move &_move);
	void			SendAttack(BMC_Move &_move);
	void			SendUseChance(BMC_Move &_move);

	// parsing dice
	BOOL			DieIsSwing(char _c) { return _c>=BMD_FIRST_SWING_CHAR && _c<=BMD_LAST_SWING_CHAR; }
	BOOL			DieIsNumeric(char _c) { return (_c>='0' && _c<='9'); }
	BOOL			DieIsValue(char _c) { return DieIsSwing(_c) || DieIsNumeric(_c); }
	BOOL			DieIsTwin(char _c) { return _c=='('; }
	BOOL			DieIsOption(char _c) { return _c=='/'; }

private:
	// parsing dice methods (uses 'line')
	INT				ParseDieNumber(INT & _pos);
	void			ParseDieSides(INT & _pos, INT _die);
	INT				ParseDieDefinedSides(INT _pos);

	// state for dice parsing
	BMC_Player *p;
	BMC_Die *d;
	char			line[BMD_MAX_STRING];
};

class BMC_Logger
{
public:
					BMC_Logger();
	
	// methods
	void			Log(BME_DEBUG _cat, char *_fmt, ... );

	// mutators
	void			SetLogging(BME_DEBUG _cat, BOOL _log) { m_logging[_cat] = _log; }
	BOOL			SetLogging(const char *_catname, BOOL _log);

	// accessors
	BOOL			IsLogging(BME_DEBUG _cat) { return m_logging[_cat]; }
	
private:
	BOOL			m_logging[BME_DEBUG_MAX];
};

// Utility functions

void BMF_Error( char *_fmt, ... );
void BMF_Log(BME_DEBUG _cat, char *_fmt, ... );

#endif //__BMAI_H__
