#!/usr/bin/perl --
##################################################################
# bmai.pl
#
# Copyright (c) 2001 Denis Papp. All rights reserved.
# denis@accessdenied.net
# http://www.accessdenied.net/bmai
#
# DESCRIPTION: this is the front-end to BMAI.  It serves as the interface between the AI and
# the BM web page by Dana Huyler.  It also is intended to do rating and evaluation of BMs,
# players and games.
#
# INSTRUCTIONS: as of 040601 this script was only tested with the following profile settings:
# - get no game emails (recommended)
# - show _all_ menu columns (there are 8 listed right now)
# - skip your turn when cannot attack is checked
# - don't hide the score
# - don't use expert mode
#
# REVISION HISTORY
# drp040601 - added $g_results so can see a summary of the session
# drp040701 - parse game list code better, purge completed games and log to $BMD_HISTORY
# drp041401 - changed to use 2-PLY and 20 SIMS (was 1/500), 7 games were left (up to 5344)
# drp051201 - added handling for reserve dice
#			-	sims to 50
# drp051301 -	sims to 100
# drp052501 - record and use reserve dice option values (numbers are not what BMAI thinks
#			  if reserve dice have already been used)
# drp052901 - added $g_actions to summarize game inputs and actions taken
#			- added SMTP update email for each (unaborted) run
# drp060501 - added maxbranch
# drp070701 - support for CHANCE
# drp072201 - add random "seed" call
# drp090601 - support for 'pass' on a CHANCE
##################################################################

# TODO
# drp060601 - because BMAI does not distinguish games by what section they are in, it tries to
#	      play games in the Preround stage, since it says "View Game".  There is a weird bug
#	      where if a game was in state "Preround" when bmai started, but the opponent has
#	      played their turn by the time bmai gets around to it, then it does not parse the
#	      page correctly, reads 0 dice, and then gets an error.
# WORKAROUND: do nothing if see "To attack"


# MAIN FEATURES
# - Log decisions for evaluation of AI

# EXTRA FEATURES
# - Create Game Action
# - Create Game Evaluation
# - Do Nothing Evaluation
# - Join Game Evaluation
# - Retrieve and Parse Stats
# - Store Records on BM and Players
# - Rate Players and BM
# - Tournaments: eval, join, select BM

# MINOR FEATURES
# - smarter game list parsing (i.e. look at field headers)

# INCLUDES
require 'dp_lib.pl';
use Net::SMTP;

# MAIN SETTINGS


# SETTINGS
$BMW_URL_BASE = "http://www.buttonmen.dhs.org/";
$BMD_BMAI = "c:\\dev\\bmai\\release\\bmai";
$BMD_BMAI_INPUT = "bmai_in.txt";
$BMD_BMAI_OUTPUT = "bmai_out.txt";
$BMD_BMAI_PATH = "c:\\dev\\bmai\\";
$BMD_HISTORY = $BMD_BMAI_PATH . "history.txt";
$DATE = "c:\\bin\\date";
$TEMP_FILE = "out";
$ADMIN_USER = "lunatic";
$ADMIN_EMAIL = "denis\@accessdenied.net";
$SMTP_SERVER = "mail.houston.rr.com";
$VERBOSE_USERS = "(lunatic|zaph)";

$USERNAME = "bmai";
$PASSWORD = "****";

# WEBSITE URLs
$BMW_MAIN = "menu.fcgi";
$BMW_MENU = $BMW_URL_BASE . $BMW_MAIN;
#$BMW_NEWUSER = $BMW_URL_BASE . "$BMW_MAIN?cmd=newuser";
$BMW_LOGIN = $BMW_URL_BASE . "$BMW_MAIN?cmd=login";
$BMW_JOIN = $BMW_URL_BASE . "$BMW_MAIN?cmd=list_open";
$BMW_CREATE = $BMW_URL_BASE . "$BMW_MAIN?cmd=create";
$BMW_PROFILE = $BMW_URL_BASE . "$BMW_MAIN?cmd=profile&view=";   # username
$BMW_PLAY = $BMW_URL_BASE . "bm.cgi\?game="; # id
$BMW_PURGE = $BMW_URL_BASE . "$BMW_MAIN?cmd=purge&game=";       # id

# SCORING
$SCORE_ADMIN_BONUS = 100;
$SCORE_CHALLENGE_BONUS = 10;
$SCORE_JOIN_THRESHOLD = 50;

# AI SETTINGS
$PLY = 2;
$SIMS = 150;
$MAXBRANCH = 1500;

# TWEAKS

# ENUMS
$ID_ME = 0;
$ID_OPP = 1;

# DEFINITIONS
%g_id_name = ( 'Your', $ID_ME, 'Opponent', $ID_OPP );

# GLOBALS

# misc
my $g_error = undef;

my $g_availgames = 0;
my $g_results = "";
my $g_actions = "";
my $g_date;
#@{availgame . $g}

#mainmenu
my $g_games = 0;
my $g_stats_finished;
my $g_stats_won;
my $g_stats_lost;
#@{game . $g}
#@{g_dice . $id}

#playturns-preround
my $g_setdice_actions = 0;

# GLOBALS FOR BMAI

##$g_bmai_pid = 0;

# GLOBALS TEMP
my $t_state;
my $t_result;

#######################################################################################3
# INTERFACE TO HTTP LIBRARY
#######################################################################################3

my $g_ref_var;

# PARAM: url, vars
# POST: page is in $TEMP_FILE
sub bm_get_url
{
	&get_url(undef, GET, $TEMP_FILE, @_);
}

# PARAM: url, vars
# POST: page is in $TEMP_FILE
sub bm_post_url
{
	&get_url(undef, POST, $TEMP_FILE, @_);
}

sub parse_print_var
{
	$$g_ref_var .= $_;
	1;
}

sub bm_dump
{
	$g_ref_var = shift;
	if (defined $g_ref_var) {
		&parse('parse_print_var', $TEMP_FILE);
	} else {
		&parse('parse_print', $TEMP_FILE);
	}
}

# PARAM: parse function
sub bm_parse
{
	my $func = shift;
	&parse($func, $TEMP_FILE);
}


#######################################################################################
# utilities
#######################################################################################

sub bm_error
{
    $g_error = join(' ', @_);
    $g_results .= "ERROR: $g_error\n";
	$g_error_details = "ERROR: $g_error";
	$g_error_details .= "\n----------------------------\n";
	&bm_dump(\$g_error_details);
	$g_error_details .= "\n----------------------------\n";

	print $g_error_details;

	&error(@_);
}

sub bm_onexit
{
	print "SUMMARY:\n$g_actions\n\n";
	print "FINISHED:\n$g_results\n";

	open(REPORT,">>report.txt") && do
	{
		print REPORT "$g_actions\n\n$g_results\n-----------\n";
		close REPORT;
	};


	# if played no games, and no error - send no email
    return if ($games_played==0 && !defined($g_error));

    $smtp = new Net::SMTP($SMTP_SERVER); ##, Debug=>1);
    $smtp->mail($ADMIN_EMAIL);
    $smtp->to($ADMIN_EMAIL);
    $smtp->data();
    $smtp->datasend("Subject: BMAI run $g_date\n");
    $smtp->datasend("To: $ADMIN_EMAIL\n");
    $smtp->datasend("\n");
    $smtp->datasend("SUMMARY:\n$g_actions\n\nFINISHED:\n$g_results\n");
	$smtp->datasend("DETAILS:\n$g_error_details\n") if defined $g_error_details;
    $smtp->dataend();
    $smtp->quit;
}

#######################################################################################3
# interface to BMAI
#######################################################################################3

$SIG{CHLD} = sub { wait };

sub bmai_init
{
	open (BMAI_IN, ">$BMD_BMAI_INPUT") || &error("error writing $BMD_BMAI_INPUT");
	$g_bmai_state = 'init';
    # every time this is called, pushes another dp_onexit.  But
    # function is unnecessary since not using open2 so removed
    #&dp_onexit('bmai_uninit');
}

sub bmai_process
{
	&error("bad bmai state") unless $g_bmai_state eq 'init';

	&bmai_uninit;

	system "$BMD_BMAI < $BMD_BMAI_INPUT > $BMD_BMAI_OUTPUT";
	open ( BMAI_OUT, "<$BMD_BMAI_OUTPUT") || &error("error reading $BMD_BMAI_OUTPUT");

	# drain input until past 'action' (to avoid comments during move processing)
	while (<BMAI_OUT>)
	{
		last if /^action/;
		if (/best move/)
		{
			$g_bmai_best_move = $_;
			chop $g_bmai_best_move;
			&debug(BMAI, $g_bmai_best_move . "\n");
		}
	}

	&bm_error("no 'action' found in bmai output") unless defined $_;

	$g_bmai_state = 'processed';
}


sub bmai_uninit
{
	if ($g_bmai_state eq 'init')
	{
		close BMAI_IN;
	}
	elsif ($g_bmai_state eq 'processed')
	{
		close BMAI_OUT;
	}

	$g_bmai_state = undef;
}

sub bmai_read
{
	$_ = <BMAI_OUT>;
	return $_;
}

sub bmai_write
{
	while ($_ = shift)
	{
		print BMAI_IN;
	}
}

#######################################################################################
# login
#######################################################################################

sub bm_login_parse
{
	if (m,<META HTTP-EQUIV=\"Refresh\" CONTENT=\"0; URL=(\S+)\">,)
	{
		$t_result = $1;
		return 0;
	}
	1;
}

sub bm_login
{
	&init();
	&dp_onexit(bm_logout);
	&bm_post_url($BMW_LOGIN, 'u', $USERNAME, 'p', $PASSWORD);
	# this now logs in but will give us a META tag to refresh with cmd=menu
	undef $t_result;
	&bm_parse('bm_login_parse');
	&bm_error("Error logging in") unless defined $t_result;
	&bm_get_url($t_result);
}

sub bm_logout
{
	#FIX: unlink $TEMP_FILE;
}

#######################################################################################
# newuser - incomplete
#######################################################################################

# PARAM: user, pass, name, email
sub bm_newuser
{
	&error("dont use me");
	my $user = shift;
	my $pass = shift;
	my $name = shift;
	my $email = shift;

	&bm_get_url($BMW_NEWUSER, 'handle', $name, 'password', $pass, 'name', $name, 'email', $email);
}

#######################################################################################
# listgames
#######################################################################################

###################
# game parameters
$LG_USER = 0;
$LG_USER_BM = 1;
$LG_USER_TL = 2;
$LG_OPP_BM = 3;
$LG_OPP_TL = 4;
$LG_WINS = 5;
$LG_FORFUN = 6;
$LG_COMMENT = 7;
$LG_ACTION = 8;
$LG_SCORE = 9;
$LG_CHALLENGE = 10;

# valid game values

# LG_USER_BM/LG_OPP_BM:
# Random BM

# LG_USER_TL/LG_OPP_TL
# Yes/Maybe

###################


sub parse_listgames
{
	if ($t_state eq readuser)
	{
		return 1 unless /profile\.cgi/;
		@t_params = ();
		push(@t_params, &get_token(\$_));
		$t_state = readgame;
	}
	elsif ($t_state eq readgame)
	{
		my $string = $_;
		my $token;
		while ($token = &get_token(\$_))
		{
			push(@t_params, $token);
		}
		# fill join link
		if ($string =~ /href=(joinnewgame.+)>Join/)
		{
			$t_params[$LG_ACTION] = $1;
		}
		elsif ($string =~ /href=(joinnewgame.+)>Accept/)
		{
			$t_params[$LG_ACTION] = $1;
			&debug(LIST, "Game $g_availgames is a challenge from $t_params[$LG_USER]\n");
			$t_params[$LG_CHALLENGE] = 1;
		}
		$t_params[$LG_SCORE] = 0;
		&error("no action for game $g_availgames") unless $t_params[$LG_ACTION] ne "";
		@{availgame . $g_availgames++} = @t_params;
		$t_state = readuser;
	}
	1;
}

# POST:
# - fills  @{availgame . 0...} up to $g_availgames
# - scores games
sub bm_listgames
{
	my @game;
	&bm_get_url($BMW_JOIN);
	#&bm_dump;
	$t_state = readuser;
	$g_availgames = 0;
	&bm_parse('parse_listgames');
	for ($g=0; $g<$g_availgames; $g++)
	{
		@game = @{availgame . $g};
		&debug(LIST, "Game $g: $game[$LG_USER] playing $game[$LG_USER_BM] ($game[$LG_USER_TL]) vs $game[$LG_OPP_BM] ($game[$LG_OPP_BM]) for $game[$LG_WINS] fun $game[$LG_FORFUN] comment $game[$LG_COMMENT] action $game[$LG_ACTION]\n");
	}
}

sub bm_game_param
{
	my $game = shift;
	my $param = shift;
	&error("bad game") unless $game<$g_availgames;
	return ${availgame . $game}[$param];
}

sub bm_game_user {	return &bm_game_param(shift,$LG_USER);}
sub bm_game_action {	return &bm_game_param(shift,$LG_ACTION);}
sub bm_game_score {	return &bm_game_param(shift,$LG_SCORE);}
sub bm_game_challenge { return &bm_game_param(shift,$LG_CHALLENGE);}

#######################################################################################
# scoregame
#######################################################################################

sub bm_scoreuser
{
	#FIXME
}

sub bm_scoreBM
{
	#FIXME
}

# FIX: account for stats
sub bm_scoregame
{
	my $game = shift;
	my $score = 0;
	my $user = &bm_game_user($game);

	&debug(SCORE, "scoring $game user $user\n");
	$score += $SCORE_ADMIN_BONUS if ($user eq $ADMIN_USER);
	$score += $SCORE_CHALLENGE_BONUS if (&bm_game_challenge($game));

	return $score;
}

sub bm_scoregames
{
	for ($g=0;$g<$g_availgames;$g++)
	{
		$score = ${availgame . $g}[$LG_SCORE] = &bm_scoregame($g);
		&debug(SCORE, "game $g score $score\n");
	}
}

#######################################################################################
# joingame
#######################################################################################

sub parse_joingame
{
	if (/Starting Game \#(\d+)/)
	{
		&debug(JOIN, "joined game $1 successfully\n");
		$t_success = 1;
		return 0;
	}
	1;
}

# PARAM: game number
sub bm_joingame
{
	my $game = shift;
	$action = &bm_game_action($game);
	&error("game $game action not defined") unless defined $action;
	my $user = &bm_game_user($game);
	&debug(JOIN, "joining game $game user $user action $action\n");
	&bm_get_url($BMW_URL_BASE . $action);
	$t_success = 0;
	&bm_parse('parse_joingame');
	&error("Error joining game") unless $t_success;
}

sub bm_joingames
{
	&debug(JOIN, "bm_joingames\n");
	for ($g=0; $g<$g_availgames; $g++)
	{
		my $score = &bm_game_score($g);
		&debug(JOIN, "considering joining game $g score $score\n");
		if (&bm_game_score($g)>=$SCORE_JOIN_THRESHOLD)
		{
			&bm_joingame($g);
		}
	}
}

#######################################################################################
# mainmenu (my games)
# TODO: use hash instead (i.e. refer to vars by name, such as "Opponent")
#######################################################################################

# each game
# game, opponent, status, button men, enter game action

# parameters for mainmenu games
$i=0;
$MM_ID = $i++;
$MM_TOURN = $i++;
$MM_FUN = $i++;
$MM_OPP = $i++;
$MM_STATUS = $i++;
# $MM_BLANK = $i++;
$MM_MY_BM = $i++;
$MM_VS = $i++;
$MM_OPP_BM = $i++;
# $MM_BLANKPOST = $i++;
$MM_SCORE = $i++;
$MM_IDLETIME = $i++;
$MM_ACTION_NAME = $i++;
# $MM_ACTION = $i++;
$MM_MAX = $i++;

# values


#MM_STATUS
# Your Turn, [opponent]'s Turn, Preround

sub bm_mainmenu_start_reading_game
{
	@{game . $g_games} = ();
	$t_state++;
}

sub bm_mainmenu_end_reading_game
{
	# SPECIAL: completed games have an extra field
	# For normal games look at $MM_MAX + 1 since $t_state starts at 1 when parsing a game
	if ($t_state == $MM_MAX + 2 || $t_state == $MM_MAX + 1)
	{
		${game . $g_games . action} = $BMW_PLAY . ${game . $g_games}[$MM_ID];
		@game = @{game . $g_games};
		&debug(MAIN, "in game $g_games id $game[$MM_ID] status $game[$MM_STATUS] vs $game[$MM_OPP] playing $game[$MM_MY_BM] vs $game[$MM_OPP_BM]\n");
		$t_state = 0;
		$g_games++;
		($w, $l, $t) = split(m|/|, $game[$MM_SCORE]);
		$g_temp_wlt[($w>$l) ? 0 : (($l>$w) ? 1 : 2) ]++;
	}
	else
	{
		&debug(MAIN, "unexpected <\/tr> in mainmenu ($t_state/$MM_MAX)\n");
		$t_state = 0;
	}
}

# TODO: if the last item is "View Game" it adds an unnecessary entry
sub parse_mainmenu
{
	#print "LINE: $_";
	if (/You have tournaments/)	# this puts a <tr> which messes up game parsing
	{
		##&bm_error("There are tournaments awaiting BM selection [unhandled]\n");
	}
	elsif (m,tourn=(\d+).+View Tournament.+</tr>,)
	{
		&debug(MAIN, "Ignoring tournament $1\n");
	}
	elsif (/Player Mail/)
	{
		&bm_error("There is player mail to address [unhandled]\n");
	}
	elsif (/<tr>/)
	{
		if ($t_state==$MM_MAX)
		{
			&bm_mainmenu_end_reading_game;
		}
		elsif ($t_state>0)
		{
			&debug(MAIN, "ignoring unexpected <tr> in mainmenu ($t_state)\n");
			$t_state = 0;
		}
		&bm_mainmenu_start_reading_game;
	}
	# also check for <tr>, since on 031801 was missing </tr>
	elsif (m,</tr>, || ($t_state>0 && m,<tr>,))
	{
		&bm_mainmenu_end_reading_game;
		# if was ended by a <tr>, duplicate code in the first block
		if (m,<tr>,)
		{
			&bm_mainmenu_start_reading_game;
		}
	}
	elsif ($t_state>0)
	{
		#&debug(MAIN, "watch $t_state: $_");
		$string = $_;
		while ($token = &get_token(\$string))
		{
			# ignore blank tokens
			next if ($token =~ /^\s+$/);
			push(@{game . $g_games},$token);
			#&debug(MAIN, "found index $t_state lastentry $#{game . $g_games} $token\n");
			$t_state++;
		}
	}
	elsif (m,You have finished <B>(\d+),i)
	{
		$g_stats_finished = $1;
	}
	elsif (m,winning <B>(\d+)</b>\, and losing <b>(\d+),i)
	{
		($g_stats_won,$g_stats_lost) = ($1,$2);
	}
	1;
}


# POST: fills @{game...} and sets $g_games
sub bm_mainmenu
{
	&bm_get_url($BMW_MENU);
	$g_games = 0;
	$t_state = 0;
	&bm_parse('parse_mainmenu');
	#&bm_dump;

	&bm_error("Could not parse games stats") unless defined $g_stats_finished;

	$g_results .= "Games Won $g_stats_won Lost $g_stats_lost Finished $g_stats_finished\n";
	$g_results .= "$g_games games found. Winning $g_temp_wlt[0] Losing $g_temp_wlt[1] Tied $g_temp_wlt[2]\n";

	&debug(MAIN, $g_results);
}

sub bm_ingame_param
{
	my $game = shift;
	my $param = shift;
	&error("bad ingame number $game") if !defined $game || $game>=$g_games;
	return ${game . $game}[$param];
}

sub bm_ingame_id { return &bm_ingame_param(shift, $MM_ID); }
sub bm_ingame_status { return &bm_ingame_param(shift,$MM_STATUS); }
sub bm_ingame_score { return &bm_ingame_param(shift,$MM_SCORE); }
sub bm_ingame_action_name { return &bm_ingame_param(shift, $MM_ACTION_NAME); }
sub bm_ingame_action { my $game = shift; return ${game . $game . action}; }
sub bm_ingame_opp { return &bm_ingame_param(shift,$MM_OPP); }

#######################################################################################
# general BM functions
#######################################################################################

# Swing dice:
# X = 4..20
# T = ?
# V = 6..12
# S = ?

# TODO: should strip out pieces as they are recognized to catch new dice
sub bm_text_to_die
{
	my $text = shift;
	my $die = "";
	my $sides_defined = 0;
	my $option = "";

	# properties (zero or more)
	# TODO: when mixing multiple properties, uncertain what the appropriate
	# order is.  (Bluff has 'sp', Werner has 'zsp', also saw 'tzn')
	$die .= "^" if ($text =~ /Time and Space/);     $text =~ s/Time and Space//;
	$die .= "+" if ($text =~ /Auxiliary/);          $text =~ s/Auxiliary//;
	$die .= "q" if ($text =~ /Queer/);                      $text =~ s/Queer//;
	$die .= "t" if ($text =~ /Trip/);                       $text =~ s/Trip//;
	$die .= "z" if ($text =~ /Speed/);                      $text =~ s/Speed//;
	$die .= "s" if ($text =~ s/Shadow//);
	$die .= "d" if ($text =~ s/Stealth//);
	$die .= "p" if ($text =~ s/Poison//);
	$die .= "n" if ($text =~ s/Null//);
    $die .= "B" if ($text =~ s/Berserk//);
    $die .= "H" if ($text =~ s/Mighty//);
    $die .= "h" if ($text =~ s/Weak//);
	$die .= "r" if ($text =~ s/Reserve//);
	$die .= "o" if ($text =~ s/Ornery//);
	$die .= "D" if ($text =~ s/Doppleganger//);
	$die .= "f" if ($text =~ s/Focus//);
	$die .= "c" if ($text =~ s/Chance//);
	$die .= "m" if ($text =~ s/Morphing//);
	$die .= "%" if ($text =~ s/Radioactive//);

	# option postfix (but before "defined sides")
	$option .= "!" if ($text =~ /Turbo/);           $text =~ s/Turbo//;
	$option .= "?" if ($text =~ /Mood/);            $text =~ s/Mood//;

	# sides (one)
	if ($text =~ m,Option (\d+)/(\d+),)
	{
		$die .= "$1/$2";
		$sides_defined = 1;
		$text =~ s,Option (\d+)/(\d+),,;
	}
	elsif ($text =~ s/Twin +(\S) +Swing( Die)?//)
	{
		$die .= "($1,$1)";
		$sides_defined = 1;
	}
	elsif ($text =~ /(\S) +Swing/)
	{
		$die .= $1;
		$sides_defined = 1;
		$text =~ s/(\S) +Swing//;
	}
	elsif ($text =~ /Twin Die +\(both with (\d+) sides\)/)
	{
		$die .= "($1,$1)";
		$text =~ s/Twin Die +\(both with (\d+) sides\)//;
	}
	elsif ($text =~ /Twin Die +\(with (\d+) and (\d+) sides\)/)
	{
		$die .= "($1,$2)";
		$text =~ s/Twin Die +\(with (\d+) and (\d+) sides\)//;
	}

	# add options
	$die .= $option;

	# finally, defined number of sides
	if ($text =~ /(\d+)-sided die/)
	{
		$die .= "-" if $sides_defined;  # deal with case where this is a fixed option/swing die
		$die .= $1;
		$text =~ s/(\d+)-sided die//;
	}
	elsif ($text =~ s/\(with (\d+) sides\)//)
	{
		$die .= "-" if $sides_defined;  # deal with case where this is a fixed option/swing die
		$die .= $1;
	}
	elsif ($text =~ s/\(both with (\d+) sides\)//)
	{
		$die .= "-" if $sides_defined;  # deal with case where this is a fixed option/swing die
		$die .= $1;
	}
	elsif ($text =~ s/\(with (\d+) and (\d+) sides\)//)
	{
		# PRE: only with "Twin [?] Swing"
		if ($1 != $2)
		{
			&bm_error("error parsing twin swing: $text");
		}
		$die .= "-" if $sides_defined;  # deal with case where this is a fixed option/swing die
		$die .= $1;
	}

	# when doing Focus/Chance the number of dice will be revealed in the die description
	if ($text =~ s/showing (\d+)//)
	{
		$die .= ":$1";
	}

    &bm_error("Unsuccessfully parsed die: $text -> $die\n") if $text !~ /^\s*$/;

	return $die;
}

#######################################################################################
# playturns
# TEMP VARS:
# $t_state
# $t_id
#######################################################################################

#http://dale.coe.missouri.edu/~dana/buttonmen/bm.cgi?attack_die=1,0&attack_type=skill&defender_die=0&game=721&userid=lunatic
#http://dale.coe.missouri.edu/~dana/buttonmen/bm.cgi?attack_type=Surrender+Round&attack_die=-1&attack_die=&game=755
#http://dale.coe.missouri.edu/~dana/buttonmen/bm.cgi?attack_type=pass&attack_die=-1&attack_die=&game=595

# phases of a game
$PT_PREROUND = 0;
$PT_TURN = 1;

sub parse_preround
{
	if (/You have set your Swing dice./)
	{
		$t_state = 'no_turn_needed';
		return 0;
	}
	elsif (m,needs to select (his|her|his/her) reserve die,)
	{
		$t_state = 'no_turn_needed';
		return 0;
	}
	elsif (m,The game cannot start,)
	{
		$t_state = 'no_turn_needed';
		return 0;
	}
	elsif (m,Viewing Game,)
	{
		$t_state = 'no_turn_needed';
		return 0;
	}
	elsif (m|value=\"Surrender\"|)
	{
		&debug(PLAYPREROUND, "parsing preround, but apparently is now my turn - skipping\n");
		$t_state = 'no_turn_needed';
		return 0;
	}
	elsif ($t_state eq 'readdice')
	{
		if (/Opponent's Dice.+Your Dice/)
		{
			&error("Preround dice are listed in reverse order.");
		}
		elsif (/<li>(.+)/)
		{
			push(@{g_dice . $t_id} , &bm_text_to_die($1));
			#my $l_size = $#{g_dice . $t_id};
			#&debug(PLAYPREROUND, "DICE$t_id: #$l_size = ${g_dice . $t_id}[$l_size]\n");
		}
		elsif (m,</ul>,)
		{
			$t_id++;
			$t_state = 'readaction' if ($t_id>$ID_OPP);
		}
		# for RESERVE must get dice in a group
		elsif (m,(Your|Opponent's) Dice: (.+)<Br>,)
		{
			$t_id = ($1 eq Your) ? 0 : 1;
			$dice = $2;
			@DICE = split(/, ?/, $dice);
			foreach $die (@DICE)
			{
				push(@{g_dice . $t_id}, &bm_text_to_die($die));
				#my $l_size = $#{g_dice . $t_id};
				#&debug(PLAYPREROUND, "DICE$t_id: #$l_size = ${g_dice . $t_id}[$l_size]\n");
			}
		}
		elsif (m,you may select a reserve die,)
		{
			$t_state = 'reserveaction';
			&debug(PLAYPREROUND, "looking for reserve action\n");
		}
	}
	elsif (m,You have Chance Dice,)
	{
		$t_state = 'chanceaction';
		&debug(PLAYPREROUND, "looking for chance action\n");

		# don't bother reading actions
		return 0;
	}
	# reserve action (will ignore the "-1" no reserve option
	elsif ($t_state eq 'reserveaction' && m,<option value=(\d+)>(.+),)
	{
		$value = $1;
		push(@{g_dice . 0}, &bm_text_to_die($2));
		my $l_size = $#{g_dice . 0};
		&debug(PLAYPREROUND, "RESERVE: $value = ${g_dice . 0}[$l_size]\n");
		${g_reservevalue . 0}[$l_size] = $value;
		## &bm_error("value $value mismatches # dice $l_size") unless $l_size == $value;
	}
	# reserveaction - end of actions
	elsif ($t_state eq 'reserveaction' && m,input type=submit,)
	{
		return 0;
	}
	# readaction - end of actions
	elsif ($t_state eq 'readaction' && m,input type=submit,)
	{
		return 0;
	}
	# readaction
	elsif ($t_state eq 'readaction' && m,^<td>(.+)</td>,)
	{
		my $die = $1;
		return 1 if $die =~ /^</;
		&debug(PLAYPREROUND, "readdie $die\n");
		$g_setdice_die[$g_setdice_actions] = &bm_text_to_die($die);
		$t_state = 'readmin';
	}
	elsif ($t_state eq 'readmin' && m,^<td>(\d+)</td>,)
	{
		&debug(PLAYPREROUND, "readmin $1\n");
		$g_setdice_min[$g_setdice_actions] = $1;
		$t_state = 'readmax';
	}
	elsif ($t_state eq 'readmax' && m,^<td>(\d+)</td>,)
	{
		&debug(PLAYPREROUND, "readmax $1\n");
		$g_setdice_max[$g_setdice_actions] = $1;
		$t_state = 'readvar';
	}
	elsif ($t_state eq 'readvar' && m,^<td><input type.+ name=\"(\S+)\",)
	{
		$g_setdice_var[$g_setdice_actions] = $1;
		$t_state = 'readaction';
		$g_setdice_actions++;
	}
	elsif ($t_state ne 'readaction' && $t_state ne 'reserveaction')
	{
		&error("Error in parse_preround: $_");
	}

	1;
}

sub parse_preround_result
{
	if (/The game cannot start.+Reason: (.+)/)
	{
		$t_result = $1;
		return 0;
	}
	elsif (/(Rolling Dice)/)
	{
		$t_result = $1;
		return 0;
	}
	elsif (/.+(gained the initiative) with.+/) # CHANCE success
	{
		$t_result = $1;
		return 0;
	}
	elsif (/(failed to get initiative)/) # CHANCE failure
	{
		$t_result = $1;
		return 0;
	}
	# sometimes after a preround action, it gives the "View" page
	elsif (/(Viewing Game #)/)
	{
		$t_result = $1;
		return 0;
	}
	# sometimes after doing a reserve action, it asks for swing dice
	elsif ($t_state eq "reserveaction" && /Pick a value for each of your Swing dice/)
	{
		$t_result = "WARNING: must setswing!!";
		return 0;
	}

	1;
}


# PRE: state retrieved already
# FIX: deal with multiple swing dice
# FIX: learn to parse dice by names?
sub bm_playturn_preround
{
	my $g = shift;
	my $id = &bm_ingame_id($g);

	# parse
	for ($i=0; $i<2; $i++)
	{
		@{g_dice . $i} = ();
	}
	$t_state = readdice;
	$t_id = $ID_ME;
	$g_setdice_actions = 0;
	&bm_parse('parse_preround');

	if ($t_state eq 'no_turn_needed')
	{
		&debug(PLAYPREROUND, "swing dice already set\n");
		$g_actions .= "no action\n";
		return 0;
	}

	if ($#{g_dice . 0}<1)
	{
		&bm_error("no dice read in preround");
	}

	# log dice
	for ($p=0; $p<2; $p++)
	{
	my $log = "p$p: ";
		for ($n=0; $n<=$#{g_dice . $p}; $n++)
		{
	    $log .= "${g_dice . $p}[$n] ";
		}
	&debug(PLAYPREROUND, "$log\n");
		$g_actions .= "$log\n";
	}

	# this has filled @g_setdice... with the desired actions
	my $a;
	for ($a=0; $a<$g_setdice_actions; $a++)
	{
		&debug(PLAY, "action $a die $g_setdice_die[$a] min $g_setdice_min[$a] max $g_setdice_max[$a] var $g_setdice_var[$a]\n");
	}

	# send state to bmai
	&bmai_init();
	# drp072201 - randomize the BMAI RNG
	&bmai_write("seed 0\n");
	&bmai_write("game\n");
	if ($t_state eq "reserveaction") {
		&bmai_write("reserve\n");
	} elsif ($t_state eq "chanceaction") {
		&bmai_write("chance\n");
	} else {
		&bmai_write("preround\n");
	}
	for ($p=0; $p<2; $p++)
	{
		my $num_dice = $#{g_dice . $p} + 1;
		&bmai_write("player $p $num_dice 0\n"); # score can be computed correctly in preround, so is passed as 0
		for ($n=0; $n<$num_dice; $n++)
		{
			&bmai_write("${g_dice . $p}[$n]\n");
		}
	}
    &bmai_write("ply $PLY\nsims $SIMS\nmaxbranch $MAXBRANCH\n");
	&bmai_write("getaction\n");
	&bmai_process();

	# send a 'taunt' of the best move score if opponent is admin
	my @taunt = ();
	if (&bm_ingame_opp($g) =~ /^$VERBOSE_USERS$/i)
	{
		@taunt = ('taunt', $g_bmai_best_move);
	}

	# get action
	my @vars = ();
	my $pushed_chance_dice = 0;
	while ($_ = &bmai_read)
	{
		## &bm_error("passing on preround\n") if /^pass/;

		$g_actions .= $_;

		if (/^swing (\S) (\d+)/)
		{
			push @vars, "TurboSwing$1";
			push @vars, $2;
		}
		elsif (/^option (\d+) (\d+)/)
		{
			push @vars, "TurboSwing$1";     # 0-based
			push @vars, $2;
		}
		elsif (/^reserve (\-?\d+)/)
		{
			# reserve:
			# BMAI gives 0-based
			# BMWEB wants 1-based
			# BMWEB also uses original BM definition ordering (stored in ${g_reserve})
			#my $value = $1 + 1;	# 1-based
			my $value = ${g_reservevalue . 0}[$1];
			push @vars, "reserve";
			push @vars, $value;
		}
		elsif (/^chance (\d+)/)
		{
			push @vars, "setme", "1" if (!$pushed_chance_dice);
			push @vars, "chance$1";
			push @vars, "1";
			$pushed_chance_dice++;
		}
		elsif (/^pass/) # pass on chance
		{
			push @vars, "bail", "1";
			push @vars, "attack_type", "";
		}
	}
	&bmai_uninit();

	push(@vars, @taunt);

	# submit request
	&bm_post_url("$BMW_PLAY$id", 'attack_die', "", 'attack_type', "", @vars);
	$t_result = undef;
	&bm_parse('parse_preround_result');

	if (defined $t_result)
	{
		&debug(PLAY, $t_result . "\n");
		$g_actions .= "$t_result\n";
	}
	else
	{
		&bm_error("Did not recognize preround action result");
	}

	return 1;
}

sub parse_fight
{
	#if (/(\S+) Button Man/)
	if (0) #WARNING: does not print out word "Opponent"
	{
		$t_id = $g_id_name{$1};
	}
	elsif (/^$USERNAME passed this turn/)
	{
		$t_state = 'no_turn_needed';
		return 0;
	}
	elsif (/Score: <b>([\-\d]+)/i)
	{
		$g_score[$t_id] = $1;
	}
	elsif ($t_id == $ID_ME && m,Rounds Won/Lost/Tied: <B>(\d+) */ *(\d+) */ *(\d+).+of (\d+) wins,)
	{
		$g_standing[0] = $1;
		$g_standing[1] = $2;
		$g_standing[2] = $3;
		$g_wins = $4;
	}
	elsif (m,(\S+) Captured Dice: <B>(.+)</,)
	{
		$t_id = $g_id_name{$1};
		# TODO: parse captured dice
	}
	elsif (m,(Your|Opponent) Dice,)
	{
		$t_id = $g_id_name{$1};
		$t_state = 'readdice';
		$t_die = 0;
	}
	elsif (m,(Your|Opponent) Value,)
	{
		$t_id = $g_id_name{$1};
		$t_state = 'readvalue';
		$t_die = 0;
	}
	elsif ( ($t_state eq 'readdice' || $t_state eq 'readvalue') && m,</tr>,)
	{
		$t_state = undef;
	}
	elsif ($t_state eq 'readdice')
	{
		# ignore deleted dice - or stop reading dice when reading a blank table entry <td>&nbsp;</td>
		if (m,^<td>\&nbsp\;</td>,)
		{
			$t_state = undef;
			return 1;
		}
		&error("error reading die $_") unless m,(pick_attack|font size=-1)>(.+?)</,;
		my $die = $2;
		$die =~ s/.+(pick_attack|font_size=-1)>//;
		$die =~ s/<a href.+>//;
		$die =~ s/<br>/ /g;
		push(@{g_dice . $t_id}, &bm_text_to_die($die));
	}
	elsif ($t_state eq 'readvalue')
	{
		&error("error reading value $_") unless m,>(\d+)<,;
		push(@{g_value . $t_id}, $1);
	}

	1;
}

sub parse_fight_result
{
	if (/Performing a (\S+) attack/i)
	{
		$t_state = success;
	}
	elsif (/Select Turbo/)
	{
		$t_state = turbo;
		return 0;
	}
	1;
}

# PRE: state retrieved already
sub bm_playturn_main
{
	my $g = shift;
	my $id = &bm_ingame_id($g);

	# parse
	for ($i=0; $i<2; $i++)
	{
		@{g_dice . $i} = ();
		@{g_value . $i} = ();
	}
	$t_id = $ID_ME;
	$t_state = undef;
	&bm_parse('parse_fight');

	if ($t_state eq 'no_turn_needed')
	{
		&debug(PLAYFIGHT, "no turn needed\n");
		return;
	}

	# VARS THAT WERE SET UP
	# $g_score[id]
	# $g_standing[]
	# $g_wins

	# send state to bmai
	&bmai_init();
	&bmai_write("game\n");
	&bmai_write("fight\n");
	for ($p=0; $p<2; $p++)
	{
		my $num_dice = $#{g_dice . $p} + 1;
	my $log = "p$p/$g_score[$p]: ";
		&bmai_write("player $p $num_dice $g_score[$p]\n");
		for ($n=0; $n<$num_dice; $n++)
		{
	    $log .= "${g_dice . $p}[$n]:${g_value . $p}[$n] ";
			&bmai_write("${g_dice . $p}[$n]:${g_value . $p}[$n]\n");
		}
	&debug(FIGHT, "$log\n");
		$g_actions .= "$log\n";
	}
    &bmai_write("ply $PLY\nsims $SIMS\nmaxbranch $MAXBRANCH\n");
	&bmai_write("getaction\n");
	&bmai_process();

	# get action
	my $att_type;
	$att_type = &bmai_read;
	chop $att_type;

	# read involved dice - may be undef if this is a pass or surrender
	my $att = &bmai_read;
	my $def = &bmai_read;
	chop $att; chop $def;

	# read any possible turbo actions (probably undef)
	my $turbo = &bmai_read;
	chop $turbo;

	$att =~ y/ /,/;
	$def =~ y/ /,/;

	&bmai_uninit();

	$g_actions .= "$g_bmai_best_move\n";
	$g_actions .= defined $att ? "$att $att_type $def\n" : "pass\n";

	# submit request
	# send a 'taunt' of the best move score if opponent is admin
	my @taunt = ();
	if (&bm_ingame_opp($g) =~ /^$VERBOSE_USERS$/i)
	{
		@taunt = ('taunt', $g_bmai_best_move);
	}
	my @vars = ();
	push @vars, 'userid', $USERNAME;
	push @vars, 'attack_type', $att_type;
	push @vars, 'attack_die', $att;
	push @vars, 'defender_die', $def;
	push @vars, 'action', 'Beat People UP!';

	if ($turbo =~ /^option (\d+) (\d+)/)
	{
		push @vars, "TurboSwing$1";
		push @vars, $2;
	}
	elsif ($turbo =~ /^swing (\S) (\d+)/)
	{
		push @vars, "TurboSwing$1";
		push @vars, $2;
	}
	elsif (defined $turbo)
	{
		&bm_error("Did not recognize turbo: $turbo\n");
	}

	push @vars, @taunt;

	&bm_post_url("$BMW_PLAY$id", @vars);
	$t_state = undef;

	&bm_parse('parse_fight_result');

	&bm_error("attack failed game $id\n") unless defined $t_state;

	&bm_error("did not recognize turbo ($turbo)\n") unless $t_state ne 'turbo';

	&debug(PLAY, "Attack Result: $t_state\n");

	return 1;
}

sub bm_playturn
{
	my $g = shift;
	my $phase = shift;
	my $id = &bm_ingame_id($g);
	my $date = `$DATE`;
	chop $date;

	my $desc = "$date Phase $phase Game $id BM ${game . $g}[$MM_MY_BM] VS ${game . $g}[$MM_OPP_BM] (${game . $g}[$MM_OPP]) Score ${game . $g}[$MM_SCORE]\n";
	&debug(PLAY, $desc);
	$g_actions .= "\n$desc";

	my $action = &bm_ingame_action($g);
	&error("undefined playturn action") unless defined $action;
	&bm_get_url($action);

	if ($phase == $PT_PREROUND)
	{
		return &bm_playturn_preround($g);
	}
	else
	{
		return &bm_playturn_main($g);
	}
}

sub bm_purgegame
{
	my $g = shift;
	my $id = &bm_ingame_id($g);

	my $date = localtime(time);
	$date =~ s/(\S+ +\S+ +\d+).+/\1/;
	my $opp = &bm_ingame_opp($g);
	my $my_bm = &bm_ingame_param($g, $MM_MY_BM);
	my $opp_bm = &bm_ingame_param($g, $MM_OPP_BM);
	my $score = &bm_ingame_param($g, $MM_SCORE);
	local($w, $l, $t) = split(m:/:, $score);
	my $result = ($w > $l) ? "win" : "loss";

	my $log = "$date\t$id\t$opp\t$my_bm\t$opp_bm\t$w\t$l\t$t\t$result\n";

	open(HISTORY,">>$BMD_HISTORY") || &bm_error("could not open $BMD_HISTORY: $!");
	print HISTORY $log;
	close HISTORY;

	&debug(PURGE, "purging game $id: $log");
	$g_actions .= "\nCompleted: Game $id BM $my_bm VS $opp_bm ($opp) Score $score\n";

	&bm_post_url("$BMW_PURGE$id");
}

sub bm_playturns
{
	$games_played = 0;
	$games_finished = 0;
	for ($g=0; $g<$g_games; $g++)
	{
	&debug(PLAY, "Playing game $g score " . &bm_ingame_score($g) . " status " . &bm_ingame_status($g) . " action " . &bm_ingame_action_name($g) . "\n");

		if (&bm_ingame_status($g) eq 'Your Turn')
		{
			$games_played++;
			&bm_playturn($g, $PT_TURN);
		}
		elsif ( &bm_ingame_status($g) eq 'Focus/Chance' && &bm_ingame_action_name($g) eq 'Enter Game' )
		{
			if (&bm_playturn($g, $PT_PREROUND))
			{	$games_played++;	}
		}
		elsif ( (&bm_ingame_status($g) eq 'Preround' || &bm_ingame_status($g) eq 'New Game')
			 && (&bm_ingame_action_name($g) eq 'Enter Game' || &bm_ingame_action_name($g) eq 'View Game') )
	    {
			if (&bm_playturn($g, $PT_PREROUND))
			{	$games_played++;	}
		}
		elsif ( (&bm_ingame_status($g) eq 'Game Over') )
		{
			$games_finished++;
			&bm_purgegame($g);
		}
	}

	$g_results .= "$games_finished games completed\n" if ($games_finished>0);
	$g_results .= "$games_played games played\n";
}

#######################################################################################
# main
#######################################################################################

# DETERMINE DATE
$g_date = `$DATE`;
chop $g_date;

# SET DEBUGGING LEVELS
&set_debug(undef,1);

# LOG IN
&dp_onexit('bm_onexit');
&bm_login();
# TODO: LOAD DATA or UPDATE DATA (stats on players and BM)
# &bm_listgames();
# &bm_scoregames();
# &bm_joingames();
&bm_mainmenu();
&bm_error("No games\n") unless $g_games>0;

&bm_playturns();

&dp_exit(0);

__END__

