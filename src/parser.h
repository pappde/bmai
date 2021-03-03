#pragma once

class BMC_Parser
{
public:
	BMC_Parser();
	void	SetupTestGame();
	void	ParseGame();
	void	Parse();
	void	Parse(FILE *_fp) { file = _fp; Parse(); }

protected:
	void			GetAction();
	void			PlayGame(INT _games);
	void			CompareAI(INT _games);
	void			PlayFairGames(INT _games, INT _mode, F32 _p);
	void			ParseDie(INT _p, INT _dice);
	void			ParsePlayer(INT _p, INT _dice);
	bool			Read(bool _fatal = true);

	// output
	void			Send(char *_fmt, ...);
	void			SendStats();
	void			SendSetSwing(BMC_Move &_move);
	void			SendUseReserve(BMC_Move &_move);
	void			SendAttack(BMC_Move &_move);
	void			SendUseChance(BMC_Move &_move);
	void			SendUseFocus(BMC_Move &_move);

	// parsing dice
	bool			DieIsSwing(char _c) { return _c >= BMD_FIRST_SWING_CHAR && _c <= BMD_LAST_SWING_CHAR; }
	bool			DieIsNumeric(char _c) { return (_c >= '0' && _c <= '9'); }
	bool			DieIsValue(char _c) { return DieIsSwing(_c) || DieIsNumeric(_c); }
	bool			DieIsTwin(char _c) { return _c == '('; }
	bool			DieIsOption(char _c) { return _c == '/'; }

private:
	// parsing dice methods (uses 'line')
	INT				ParseDieNumber(INT & _pos);
	void			ParseDieSides(INT & _pos, INT _die);
	INT				ParseDieDefinedSides(INT _pos);

	// state for dice parsing
	BMC_Player *p;
	BMC_Die *d;
	char			line[BMD_MAX_STRING];
	FILE *file;
};