#!/usr/bin/perl --
# TODO: new "game over" notification
###################################################################################
# bmai.pl
#
# Copyright (c) 2001-2020 Denis Papp. All rights reserved.
# denis@accessdenied.net
# https://github.com/pappde/bmai
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
# drp102801	- release 1.0
# drp110301 - fixed problem recognizing CHANCE action wanted
# drp111001 - support for FOCUS
# drp031302 - added ability to create games (3 games of random set vs the same set)
# drp031502 - don't fail if player mail/news/challenges are there, but notify in $g_actions
# drp031702 - added statistics reporting in taunt when game starts
# drp062702	- fixed stats taunting - didn't display 'vs_bm' correctly
#			- added '2000 Rare/Promo' to sets
# drp063002	- dump 'stats' line, such as "Time Elapsed" in $g_actions
#			- handle 'surrender'
# drp071302	- deal with case where default "bmai_out.txt" file cannot be written to
# drp072102 - deal with 'The Flying Squirrel' special rule (die modifier 'k')
# drp090202 - parsing: added 'Stinger' and fixed problem parsing special option dice
#	    - add 'Diceland' and '2002 Rare-Promo' sets
# drp090702 - added game # to bets move taunt
# drp101302 - deal with 'Guillermo' special rule (die modifier 'u' - used to be part of the definition)
#	    - added 'Renaissance' set
# drp102202 - handle new "Notification Center" for acknowledging/purging completed games
# drp021403 - fixed mainmenu gamelist parsing (<tr class="ready"> from <tr>)
# drp021503 - fixed complete_game notification parsing
# drp030103 - complete_game notification link changed again, support both forms
# drp071305 - added lamer support (people who join games but don't play their turns if they start losing)
# drp080405 - fixed reserve preround parsing
###################################################################################

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

# BEHAVIOR
$::PURGE_STALE_GAMES = 0;
#$::DEBUG_GAME = 446689;

# SETTINGS
$BMW_URL_BASE = "http://www.buttonmen.dhs.org";
$BMD_BMAI = "release\\bmai";
$BMD_BMAI_INPUT = "bmai_in.txt";
$BMD_BMAI_OUTPUT_PREFIX = "bmai_out";
#$BMD_BMAI_PATH = "";
$BMD_HISTORY = $BMD_BMAI_PATH . "history.txt";
(-e $BMD_HISTORY) || die "Could not find: $BMD_HISTORY - is working dir set?";
$DATE = "c:\\bin\\date";
$TEMP_FILE = "out";
$ADMIN_USER = "lunatic";
$ADMIN_EMAIL = "denis\@accessdenied.net";
$SMTP_SERVER = "fnord.accessdenied.net"; ##mail.houston.rr.com";
$VERBOSE_USERS = "(lunatic|zaph|ahowald|jl8e)";
# drp071006 - removed BattleOmega from list
#$LAMERS = "(BattleOmega)";
$LAMERS = "()";
$STATS_TOOL = "stats.pl";
#$PERL_EXE = "d:\\bin\\perl\\bin\\perl";
$PERL_EXE = "perl";

$USERNAME = "bmai";
$PASSWORD = "****";

# WEBSITE URLs
$BMW_MAIN = "/menu.fcgi";
$BMW_MENU = $BMW_URL_BASE . $BMW_MAIN;
#$BMW_NEWUSER = $BMW_URL_BASE . "$BMW_MAIN?cmd=newuser";
$BMW_LOGIN = $BMW_URL_BASE . "$BMW_MAIN?cmd=login";
$BMW_JOIN = $BMW_URL_BASE . "$BMW_MAIN?cmd=list_open";
$BMW_CREATE = $BMW_URL_BASE . "$BMW_MAIN?cmd=create";
$BMW_PROFILE = $BMW_URL_BASE . "$BMW_MAIN?cmd=profile&view=";   # username
$BMW_PLAY = $BMW_URL_BASE . "/bm.cgi\?game="; # id
$BMW_PURGE = $BMW_URL_BASE . "$BMW_MAIN?cmd=purge&game=";       # id

# SCORING
$SCORE_ADMIN_BONUS = 100;
$SCORE_CHALLENGE_BONUS = 10;
$SCORE_JOIN_THRESHOLD = 50;

# CREATING NEW GAMES
$BMD_IDLE_DAYS_THRESHOLD = 14;
$BMD_DESIRED_ACTIVE_GAMES = 40; #35; #30; #50; #60;
$BMD_CREATE_OUTOFGAMES = 3;
@BMD_CREATE_GAME_SET = (
	'1999 Rare-Promo',
	'2000 Rare-Promo',
	'2002 Rare-Promo',
	# 'BladeMasters', # observed removal 1/4/03
	'Brawl',
	'Brom',
	'Bruno!',
	'Chicago Crew',
	'Club Foglio',
	'Diceland',
	# 'Dr. Oche', # observed that it was removed 10/27/02
	'Fairies',
	'Fantasy',
	'Four Horsemen',
	'Freaks',
	'Howling Wolf',
	'Iron Chef',
	'Legend of the Five Rings',
	'Lunch Money', 'Majesty',
	'Dork Victory', 'Presidential Button Men', 'Yoyodyne', 'Sluggy Freelance',
	'Renaissance',
	'Sailor Moon',
	'Sailor Moon 2',
	'Samurai',
	'Sanctum',
	'Save the Ogres',
	'Soldiers',
	'Tenchi Muyo',
	'The Metamorphers',
	'Vampyres',
	'Wonderland',
	'Geekz',	# added 091905
	'2003 Rare-Promo',	# added 091905
);

# AI SETTINGS
$PLY = 3;
$MIN_SIMS = 5;
$MAX_SIMS = 100;
$MAXBRANCH = 400;
# s_ply_decay is in bmai_ai.cpp

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
	$_[0] = $BMW_URL_BASE . $_[0] if ($_[0] =~ m|^/|);
	&get_url(undef, GET, $TEMP_FILE, @_);
}

# PARAM: url, vars
# POST: page is in $TEMP_FILE
sub bm_post_url
{
	$_[0] = $BMW_URL_BASE . $_[0] if ($_[0] =~ m|^/|);
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

	# clean up BMAI output file unless there was an error
	if (!defined($g_error)) {
		unlink $g_bmai_output_file;
	}

    print "Sending report to $ADMIN_EMAIL via $SMTP_SERVER\n";
    $smtp = new Net::SMTP($SMTP_SERVER); #, Debug=>1);
    print "Connected: $smtp\n";
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

#######################################################################################
# stats
#######################################################################################

# POST: fills %g_history_stats.  Index {stat type} gives ref to hash.  Index this hash
#	with the name give sa ref to an array of stats.
#
#	e.g., @{${$g_history_stats{'vs_opp'}}{'lunatic'}} = array of stats
sub bm_readstats
{
	my $state = undef;
	open (STATS,"$PERL_EXE $STATS_TOOL |") || die "could not run $STATS_TOOL";
	while (<STATS>)
	{
		chop;
		if (/STATISTIC: (.+)/)
		{
			$state = $1;
		}
		elsif (/^(.+)\s+(\d+) games \(\s*(\d+)\s+(\d+)\s+([\d\.]+\%)\)\s+\d+ matches /)
		{
			my $name = $1;
			my @stats = ($games,$win,$loss,$perc) = ($2,$3,$4,$5);
			$name =~ s/\s+$//;

			# fix percentage
			$stats[3] =~ s/%/%%/;

			$hash_ref = $g_history_stats{$state};
			if (!defined $hash_ref)
			{
				my %hash = ();
				$g_history_stats{$state} = \%hash;
				$hash_ref = $g_history_stats{$state};
			}
			$$hash_ref{$name} = \@stats;
		}
	}
	close STATS;
}

# PARAM: category, name
# RETURNS: array of stats
sub bm_gethistorystats
{
	my ($cat,$name) = @_;

	&debug(STATS, "Getting stats for $cat/$name\n");

	my $category_hash_ref = $g_history_stats{$cat};
	my $arr_ref = $$category_hash_ref{$name};

	return (defined $arr_ref) ? @$arr_ref : (0);
}

#######################################################################################
# interface to BMAI
#######################################################################################

$SIG{CHLD} = sub { wait };

sub bmai_init
{
	open (BMAI_IN, ">$BMD_BMAI_INPUT") || &error("error writing $BMD_BMAI_INPUT");
	$g_bmai_state = 'init';
    # every time this is called, pushes another dp_onexit.  But
    # function is unnecessary since not using open2 so removed
    #&dp_onexit('bmai_uninit');
}

# PARAM: game id
sub bmai_process
{
	my $g = shift;

	&error("bad bmai state") unless $g_bmai_state eq 'init';

	&bmai_uninit;

	# try to find a valid output file
	my $i = 1;
	while (1)
	{
		$g_bmai_output_file = sprintf($BMD_BMAI_OUTPUT_PREFIX . "-%d.txt", $i++);
		open ( BMAI_OUT, ">$g_bmai_output_file") && last;
	}
	close BMAI_OUT;

	$cmd = "$BMD_BMAI < $BMD_BMAI_INPUT > $g_bmai_output_file";
	system $cmd;
	(-s $g_bmai_output_file) || &bm_error("output file is empty (running '$cmd'): " . `dir`);
	open ( BMAI_OUT, "<$g_bmai_output_file") || &error("error reading $g_bmai_output_file");

	$g_bmai_best_move = undef;
	$g_bmai_stats = undef;

	my @scores = split(m|/|, &bm_ingame_score($g));
	my $best_move_games = $scores[0] + $scores[1] + $scores[2] + 1;

	# drain input until past 'action' (to avoid comments during move processing)
	while (<BMAI_OUT>)
	{
		last if /^action/;
		if (/best move/)
		{
			$g_bmai_best_move = "Game #" . $best_move_games . ": " . $_;
			chop $g_bmai_best_move;
			&debug(BMAI, $g_bmai_best_move . "\n");
		}
		elsif (/^stats/)
		{
			$g_bmai_stats = $_;
			chop $g_bmai_stats;
			&debug(BMAI, $g_bmai_stats . "\n");
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
		# drp060503 - website now contains file part of URL only
		$t_result = $1; ### $BMW_URL_BASE . $1;
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
$MM_CLASS = $i++;
$MM_ID = $i++;
$MM_TOURN = $i++;
$MM_FUN = $i++;
$MM_OPP = $i++;
$MM_STATUS = $i++;
$MM_MY_BM = $i++;
$MM_VS = $i++;
$MM_OPP_BM = $i++;
$MM_SCORE = $i++;
$MM_IDLETIME = $i++;
$MM_ACTION_NAME = $i++;
#$MM_DELETE_GAME = $i++;
$MM_MAX = $i++;

# values


#MM_STATUS
# Your Turn, [opponent]'s Turn, Preround

# PARAM: the class
sub bm_mainmenu_start_reading_game
{
	my $class = shift;

	@{game . $g_games} = ();
	$t_state++;

	push(@{game . $g_games},$class);
	$t_state++;

        &debug(MAIN, "start reading game ($t_state - $class)\n");
}

sub bm_mainmenu_end_reading_game
{
        print "ENDREADINGGAME: $t_state\n";
	# SPECIAL: completed games have an extra field
	# For normal games look at $MM_MAX + 1 since $t_state starts at 1 when parsing a game
	if ($t_state == $MM_MAX + 2 || $t_state == $MM_MAX + 1)
	{
		${game . $g_games . action} = $BMW_PLAY . ${game . $g_games}[$MM_ID];
		@game = @{game . $g_games};
		&debug(MAIN, "in game $g_games id $game[$MM_ID] status $game[$MM_STATUS] vs $game[$MM_OPP] playing $game[$MM_MY_BM] vs $game[$MM_OPP_BM]\n");
		$t_state = 0;
		$g_games++;
		$g_idle_games++ if ( ($game[$MM_IDLETIME] =~ /(\d+) DAYS/) && ($1>$BMD_IDLE_DAYS_THRESHOLD));
		($w, $l, $t) = split(m|/|, $game[$MM_SCORE]);
		$g_temp_wlt[($w>$l) ? 0 : (($l>$w) ? 1 : 2) ]++;
	}
	else
	{
		#&debug(MAIN, "unexpected <\/tr> in mainmenu ($t_state/$MM_MAX)\n");
		$t_state = 0;
	}
}

# TODO: if the last item is "View Game" it adds an unnecessary entry
sub parse_mainmenu
{
        #print "LINE:$t_state: $_";
	if (/You have tournaments/)	# this puts a <tr> which messes up game parsing
	{
		##&bm_error("There are tournaments awaiting BM selection [unhandled]\n");
	}
	elsif (m,tourn=(\d+).+View Tournament.+</tr>,)
	{
		&debug(MAIN, "Ignoring tournament $1\n");
	}
	elsif (/Mark news read/)
	{
		$g_actions .= "There is news\n\n";
		# TODO: log and clear news
	}
	elsif (/Read Mail/)
	{
		$g_actions .= "There is player mail\n\n";
		# TODO: handle mail
	}
	elsif (/You have been challenged/)
	{
		$g_actions .= "There is a challenge\n\n";
		# TODO: handle challenges
	}
	# drp102202 - notification center
	elsif (m,<table.+Notification Center,)
	{
		$t_state = notification;
	}
	elsif ($t_state eq 'notification')
	{
		#print "LINE: $_";
		if (m,</table>,)
		{
			$t_state = 0;
		}
		elsif (m,\"(.+id=)(\d+)(.+complete_game)\">Remove Notice,)
		{
			$g_gameover{$2} = $1 . $2 . $3;
		}
		elsif (m,\"(.+complete_game)(.+id=)(\d+)(.*)\">Remove Notice,)
		{
			#&debug(MAIN, "Game Over: $3\n");
			$g_gameover{$3} = $1 . $2 . $3 . $4;
		}
                elsif (m,\"(.+id=)(\d+)(.+complete_game)(.*)\">Remove Notice,)
                {
                        $g_gameover{$2} = $1 . $2 . $3 . $4;
                }
		elsif (m,complete_game.*\">Remove all Notices,i)
		{
			# ignore
		}
		elsif (m,complete_game,)
		{
			&bm_error("unrecognized complete_game: $_");
		}
	}
	elsif (/<tr class=\"(\S+) ?\">/ || /<tr>/)
	{
		#&debug(MAIN, "read " . $_);
                print "READ END - STATE $t_state\n";
		$t_class = $1;
		if ($t_state==$MM_MAX)
		{
			&bm_mainmenu_end_reading_game;
		}
		elsif ($t_state>0)
		{
			&debug(MAIN, "ignoring unexpected <tr> in mainmenu ($t_state)\n");
			$t_state = 0;
		}
		&bm_mainmenu_start_reading_game($t_class);
	}
	# also check for <tr>, since on 031801 was missing </tr>
	elsif (m,</tr>,) #### || ($t_state>0 && m,<tr>,))
	{
                &debug(MAIN, "spotted </tr>\n");
		&bm_mainmenu_end_reading_game;
		#### if was ended by a <tr>, duplicate code in the first block
		#### if (m,<tr>,)
		#### {
		####	    &bm_mainmenu_start_reading_game;
		#### }
	}
	elsif ($t_state>0)
	{
		# drp081505 - this code used to support multiple <td>
		#  tokens per line when website put multiple columsn on one
		#  line.  But website now has max one column per line.  Plus 
		#  they have a weird case where the Arena link is followed by
		#  an "&nbsp;" and the old code was confusing that for two 
		#  tokens == two columns which is wrong.
		# FIX: changed it to support max one column/token per line
		#debug(MAIN, "watch $t_state: $_");
		$string = $_;
		$l_tokens_this_line = 0;
		while ($token = &get_token(\$string))
		{
			# ignore blank tokens
			next if ($token =~ /^\s+$/);
			next if ($l_tokens_this_line>0 && $token eq "&nbsp;");
			$l_tokens_this_line++;
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


# POST: fills @{game...} and sets $g_games, $g_active_games
sub bm_mainmenu
{
	# drp021803 - need "menu=BM" to get full list
	&bm_post_url($BMW_MENU, 'cmd', 'menu', 'menu', 'BM');
	$g_games = 0;
	$t_state = 0;
	@g_temp_wlt = (0,0,0);
	$g_idle_games = 0;
	%g_gameover = ();
	&bm_parse('parse_mainmenu');
	#&bm_dump;

	# parse g_gameover (notification center)
	&debug(MAIN, "processing completed games\n");
	foreach $g (keys %g_gameover)
	{
		&bm_process_gameover($g);
	}

	$g_active_games = $g_games - $g_idle_games;

	# drp102202 - disappeared from site
	# &bm_error("Could not parse games stats") unless defined $g_stats_finished;

	# $g_results .= "Games Won $g_stats_won Lost $g_stats_lost Finished $g_stats_finished\n";
	$g_results .= "$g_games games found ($g_active_games active). Winning $g_temp_wlt[0] Losing $g_temp_wlt[1] Tied $g_temp_wlt[2]\n";

	&debug(MAIN, $g_results);

}

sub parse_gameover_view
{
	#print "PGOV: $_";
	if (m,This game was completed: (.+)<,)
	{
	}
	elsif ( (m,profile.+view=\S+>(\S+)</a>,)
		|| (m,view.+profile.*\">(\S+)</a>,) )
	{
		#print "2 $_";
		if ($1 eq $USERNAME) {
			$t_state = 'my_bm'
		} else {
			$opp = $1;
			$t_state = 'opp_bm';
		}
	}
	elsif (0 && m,view, && m,profile,)
	{
		die "mismatched: $_";
	}
	elsif (m,Button Man.+displaybm.*\">(.+)</a>,)
	{
		#print "3:$t_state: $_";
		($t_state eq 'my_bm') ? $my_bm = $1 : $opp_bm = $1;
	}
	elsif ($t_state eq 'my_bm')
	{
		#print "4 $_";
		if (m,Rounds.+<B>(.+)</B>,) {
			$score = $1;
		}
	}

	1;
}

# PRE: $g_gameover{$id} = the remove URL
#      bm.cgi?game=$id to view the results of the game
# NOTE: copies bm_purgegame()
sub bm_process_gameover
{
	my $id = shift;
	my $view_url = $BMW_PLAY . $id;

	my $date = localtime(time);
	$date =~ s/(\S+ +\S+ +\d+).+ (\d+)/\1 \2/;

	# determine opp, my_bm, opp_bm, score (w/l/t)
	$opp = $my_bm = $opp_bm = $score = undef;
	$t_state = undef;
	&bm_get_url($view_url);
	&bm_parse('parse_gameover_view');

	die "processing gameover - opp: $opp bm: $my_bm vs $opp_bm score: $score" unless defined $opp && defined $my_bm && defined $opp_bm && defined $score;

	$score =~ s/ //g;
	local($w, $l, $t) = split(m:/:, $score);
	my $result = ($w > $l) ? "win" : "loss";

	my $log = "$date\t$id\t$opp\t$my_bm\t$opp_bm\t$w\t$l\t$t\t$result\n";

	open(HISTORY,">>$BMD_HISTORY") || &bm_error("could not open $BMD_HISTORY: $!");
	print HISTORY $log;
	close HISTORY;

	$g_actions .= "\nCompleted: Game $id BM $my_bm VS $opp_bm ($opp) Score $score\n";

        $games_finished++;

	&bm_post_url($g_gameover{$id});
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
sub bm_ingame_delete_game { return &bm_ingame_param(shift, $MM_DELETE_GAME); }
sub bm_ingame_action { my $game = shift; return ${game . $game . action}; }
sub bm_ingame_opp { return &bm_ingame_param(shift,$MM_OPP); }
sub bm_ingame_my_bm { return &bm_ingame_param(shift,$MM_MY_BM); }
sub bm_ingame_opp_bm { return &bm_ingame_param(shift,$MM_OPP_BM); }


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
    $die .= "G" if ($text =~ s/Rage//);
	$die .= "f" if ($text =~ s/Focus//);
	$die .= "c" if ($text =~ s/Chance//);
	$die .= "m" if ($text =~ s/Morphing//);
	$die .= "%" if ($text =~ s/Radioactive//);
	$die .= "`" if ($text =~ s/Warrior//);
	$die .= "w" if ($text =~ s/Slow//);
	$die .= "u" if ($text =~ s/Unique//);
	$die .= "g" if ($text =~ s/Stinger//);
	$die .= "k" if ($text =~ s/Konstant//);	

	# option postfix (but before "defined sides")
	$option .= "!" if ($text =~ /Turbo/);           $text =~ s/Turbo//;
	$option .= "?" if ($text =~ /Mood/);            $text =~ s/Mood//;

	# sides (one)
	if ($text =~ s,Option +(\d+)/(\d+),,)
	{
		$die .= "$1/$2";
		$sides_defined = 1;
	}
	# BUG: drp063002 - weird bug with web site.  Option die without options defined.  Just treat it as a normal die
	elsif ($text =~ s,Option (\d+)-sided die,,)
	{
		$die .= $1;
		$sides_defined = 1;
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

		# indicate dizzy die
		$die .= "d" if ($text =~ s/\(Dizzy\)//);
	}

    &bm_error("Unsuccessfully parsed die: $text -> $die\n") if $text !~ /^\s*$/;

	return $die;
}


# PRE: parse_fight called
# RETURNS: taunt string
sub bm_process_taunting
{
	my $taunt;

	# vs_bm, same_bm, opp, bm

	# opp stats
	my $opp = ${game . $g}[$MM_OPP];
	@stats = &bm_gethistorystats('opp',$opp);
	if ($stats[0]>0)
	{
		my $perc = $stats[3];
		if ($opp =~ /^$LAMERS$/i) {
			$perc = "92.7%"; 
		}
		$games_string = ($stats[0]>1) ? "games" : "game";
		$taunt = "I am $perc in $stats[0] $games_string against you.";
	}
	else
	{
		$taunt = "I have never played you.";
	}

	# template: same bm
	my $my_bm = ${game . $g}[$MM_MY_BM];
	my $opp_bm = ${game . $g}[$MM_OPP_BM];
	if ($my_bm eq $opp_bm)
	{
		@stats = &bm_gethistorystats('same_bm',$my_bm);

		# template: same bm, played
		if ($stats[0]>0)
		{
			$games_string = ($stats[0]>1) ? "games" : "game";
			$taunt .= "   I am $stats[3] in $stats[0] $games_string when playing $my_bm against itself.";
		}
		# template: same bm, never played
		else
		{
			$taunt .= "   I have never played $my_bm against itself.";
		}
	}

	# my bm
	@stats = &bm_gethistorystats('bm',$my_bm);

	# template: played my bm
	if ($stats[0]>0)
	{
		$games_string = ($stats[0]>1) ? "games" : "game";
		$taunt .= "   I am $stats[3] in $stats[0] $games_string when playing $my_bm.";
	}
	# template: never played my bm
	else
	{
		$taunt .= "   I have never played $my_bm.";
	}

	# opp bm
	@stats = &bm_gethistorystats('vs_bm',$opp_bm);

	# template: played against opp bm
	if ($stats[0]>0)
	{
		$games_string = ($stats[0]>1) ? "games" : "game";
		$taunt .= "   I am $stats[3] in $stats[0] $games_string when playing against $opp_bm.";
	}
	# template: never played against opp bm
	else
	{
		$taunt .= "   I have never played against $opp_bm.";
	}

	&debug(FIGHT, "TAUNT: $taunt\n");

	# remove extra "%%"
	$taunt =~ s/%%/%/g;

	return $taunt;
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
	# print "#$t_state: $_";
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
			my $l_size = $#{g_dice . $t_id};
			&debug(PLAYPREROUND, "DICE$t_id: #$l_size = ${g_dice . $t_id}[$l_size]\n");
		}
		# drp072105 - new reserve syntax
		# drp091905 - made regexp more general so catches twin dice
		elsif (/td valign=top>(.+ die.*)<br>/i)
		{
			&debug(PLAYPREROUND, "RESERVE-TD PARSING: $1\n");
			push(@{g_dice . $t_id} , &bm_text_to_die($1));
			my $l_size = $#{g_dice . $t_id};
			&debug(PLAYPREROUND, "RESERVE DICE$t_id: #$l_size = ${g_dice . $t_id}[$l_size]\n");
		}
		# drp072105 - new reserve syntax
		# drp091905 - made regexp more general so catches twin dice
		elsif (/(.+ die.*)<br>$/i)	     
		{
			&debug(PLAYPREROUND, "RESERVE-BR PARSING: $1\n");
			push(@{g_dice . $t_id} , &bm_text_to_die($1));
			my $l_size = $#{g_dice . $t_id};
			&debug(PLAYPREROUND, "RESERVE DICE$t_id: #$l_size = ${g_dice . $t_id}[$l_size]\n");
		}
		# drp072105 - new reserve syntax
		elsif (/^<\/td>/)
		{
			$t_id++;
			#$t_state = 'readaction' if ($t_id>$ID_OPP);
			# now we're looking for the reserve choices (<option value>)
			$t_state = 'reserveaction' if ($t_id>$ID_OPP);
			&debug(PLAYPREROUND, "NEXT PLAYER ($t_state)\n");
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
				print "TITLE PARSE: $die\n";
				push(@{g_dice . $t_id}, &bm_text_to_die($die));
				my $l_size = $#{g_dice . $t_id};
				&debug(PLAYPREROUND, "GROUP - DICE$t_id: #$l_size = ${g_dice . $t_id}[$l_size]\n");
			}
		}
		elsif (m,you may select a reserve die,)
		{
			$t_state = 'reserveaction';
			&debug(PLAYPREROUND, "looking for reserve action\n");
		}
	}
	# drp072105 - new reserve syntax		
	elsif (m,Choose Reserve Die,i)
	{
		$t_state = 'reserveaction';
		&debug(PLAYPREROUND, "end of reserve action\n");
		
		# don't bother reading actions
		return 0;
	}
	elsif (m,You have Chance Dice,)
	{
		$t_state = 'chanceaction';
		&debug(PLAYPREROUND, "looking for chance action\n");

		# don't bother reading actions
		return 0;
	}
	elsif (m,You have Focus Dice,)
	{
		#&bm_error("focus support not finished");
		$t_state = 'focusaction';
		&debug(PLAYPREROUND, "looking for focus action\n");

		# don't bother reading actions
		return 0;
	}
	# reserve action (will ignore the "-1" no reserve option)
	# drp080405: reserve dice were populated above, so we don't add this die, we find it in the list
	elsif ($t_state eq 'reserveaction' && m,<option value=(\d+)>(.+),)
	{
		$value = $1;
		$die = &bm_text_to_die($2);
		print "RESERVE OPTION:$value DIE:$die\n";
		
		return 1 if ($value<0);
		
		#push(@{g_dice . 0}, &bm_text_to_die($2));
		my $l_size = $#{g_dice . 0};
		my $slot;
		my $i;
		for ($i=0; $i<=$l_size; $i++) 
		{
			if ($die eq ${g_dice . 0}[$i])
			{
				$slot = $i;
				last;
			}
		}
		&bm_error("could not find reserve die ($die)") unless defined $slot;	
		&debug(PLAYPREROUND, "RESERVE: $value = ${g_dice . 0}[$slot]\n");
		${g_reservevalue . 0}[$slot] = $value;
		# &bm_error("value $value mismatches # dice $l_size") unless $l_size == $value;
	}
	# reserveaction - end of actions
	elsif ($t_state eq 'reserveaction' && m,input type=submit,)
	{
		return 0;
	}
	# readaction - end of actions
	# - ignore the chance action "No Initiative Response"
	elsif ($t_state eq 'readaction' && m,input type=submit, && $_ !~ /No Initi?ative Response/)
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
	# FOCUS success unchallenged
	elsif ($t_action =~ /^focus/ && $_ =~ /To attack, click on one of your dice/)
	{
		$t_result = "focus success unchallenged";
		return 0;
	}
	# FOCUS success challenged
	elsif ($t_action =~ /^focus/ && $_ =~ /can now respond with/)
	{
		$t_result = "focus success challenged";
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
		&bm_playturn_fix_dice($g, $p);
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
	my $action = $t_state;
	&bmai_write("game\n");
	if ($t_state eq "reserveaction") {
		&bmai_write("reserve\n");
	} elsif ($t_state eq "chanceaction") {
		&bmai_write("chance\n");
	} elsif ($t_state eq "focusaction") {
		&bmai_write("focus\n");
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
    &bmai_write("ply $PLY\nmax_sims $MAX_SIMS\nmin_sims $MIN_SIMS\nmaxbranch $MAXBRANCH\n");
	&bmai_write("getaction\n");
	&bmai_process($g);

	$g_actions .= "$g_bmai_best_move\n";
	$g_actions .= "$g_bmai_stats\n";

	# send a 'taunt' of the best move score if opponent is admin
	if (&bm_ingame_opp($g) =~ /^$VERBOSE_USERS$/i)
	{
		@taunt = ('taunt', $g_bmai_best_move);
	}

	# get action
	my @vars = ();
	my $pushed_chance_dice = 0;
	my $pushed_focus_dice = 0;
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
			&bm_error("g_reservevalue missing") unless defined $value;
		}
		elsif (/^chance (\d+)/)
		{
			push @vars, "setme", "1" if (!$pushed_chance_dice);
			push @vars, "chance$1";
			push @vars, "1";
			$pushed_chance_dice++;
		}
		elsif (/^focus (\d+) (\d+)/)
		{
			push @vars, "setme", "1" if (!$pushed_focus_dice);
			$idx = $1 + 1;					# focus is 1-based
			push @vars, "focus$idx";
			push @vars, $2;
			$pushed_focus_dice++;
		}
		elsif (/^pass/) # pass on chance or focus
		{
			# drp072105 - new reserve syntax
			# drp080205 - "chance/focus" still use bail
			if ($action eq "reserveaction") {
				push @vars, "reserve", "-1";
				push @vars, "attack_type", "";
			} else {
				push @vars, "bail", "1";
			}
		}

		$t_action = $_;
	}
	&bmai_uninit();

	if ($::DEBUG_GAME)
	{
		&debug(PLAY, "Debugging game - not sending action\n");
		return 1;
	}

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
	elsif (m|(Your\s)?Button Man:.+>(.+)<\/a>|)
	{
		# redundant
		# ($name,$bm) = ($1,$2);
		# $name =~ s/\s+$//;
		# $name = "Opponent" if ($name eq "");
		# $g_game_bm[$g_id_name{$name}] = $bm;
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
		$g_standing_games_played = $1 + $2 + $3;
	}
	elsif (m,(Your\s)?Captured Dice: <B>(.+)</,)
	{
		($name,$dice) = ($1,$2);
		$name =~ s/\s+$//;
		$name = "Opponent" if ($name eq "");
		$dice = "" if ($dice eq "None");
		# print "READ CAPTURED DICE: $name DICE: $dice\n";
		$t_id = $g_id_name{$name};
		$g_captured_dice[$t_id] = $dice;
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
		#print "READ $die\n";
		push(@{g_dice . $t_id}, &bm_text_to_die($die));
	}
	elsif ($t_state eq 'readvalue')
	{
		&error("error reading value $_") unless m,>(\d+)<,;
		$val = $1;

		# if reading MY dice and dont see "pick_attack" then the die is dizzy
		$dizzy = ($_ !~ /pick_attack>/ && $t_id == $ID_ME);
		$val .= "d" if $dizzy;
		push(@{g_value . $t_id}, $val);

	}
	elsif (/Player Communication/)
	{
		$t_state = 'readcomments';
	}
	elsif ($t_state eq 'readcomments' && /<tr>/)
	{
		$t_state = 'readcomments_name';
	}
	elsif ($t_state eq 'readcomments_name')
	{
		# <Td ...>name<...
		# then <td...><font...>text<...
		$g_comments = $_;
		$t_state = 'finished';

		# TODO: parse comments.  Complex since </table> and all comments on the same line.
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
	elsif (/SURRENDERED this round/)
	{
		$t_state = "surrendered";
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
	$g_comments = undef;
	&bm_parse('parse_fight');

	#die $g_comments if (&bm_ingame_opp($g) eq "lunatic");

	if ($t_state eq 'no_turn_needed')
	{
		&debug(PLAYFIGHT, "no turn needed\n");
		return;
	}

	# VARS THAT WERE SET UP
	# $g_score[id]
	# $g_standing[]
	# $g_wins
	# $g_comments: if there were any

	# send state to bmai
	&bmai_init();
	&bmai_write("game\n");
	&bmai_write("fight\n");
	for ($p=0; $p<2; $p++)
	{
		my $num_dice = $#{g_dice . $p} + 1;
		my $log = "p$p/$g_score[$p]: ";
		&bmai_write("player $p $num_dice $g_score[$p]\n");
		&bm_playturn_fix_dice($g, $p);
		for ($n=0; $n<$num_dice; $n++)
		{
			$log .= "${g_dice . $p}[$n]:${g_value . $p}[$n] ";
			&bmai_write("${g_dice . $p}[$n]:${g_value . $p}[$n]\n");
		}
		&debug(FIGHT, "$log\n");
		$g_actions .= "$log\n";
	}
    &bmai_write("ply $PLY\nmax_sims $MAX_SIMS\nmin_sims $MIN_SIMS\nmaxbranch $MAXBRANCH\n");
	&bmai_write("getaction\n");
	&bmai_process($g);

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
	$g_actions .= "$g_bmai_stats\n";
	$g_actions .= defined $att ? "$att $att_type $def\n" : "pass\n";

	# submit request

	# taunting

	my @taunt = ();
	my $taunt = "";

	# for first game, send a taunt with stats on this matchup.
	# TODO: doesn't work if opponent moves first and sends a taunt
	if ($g_standing_games_played == 0 && (!defined $g_comments))
	{
		$taunt = &bm_process_taunting;
	}

	# send a 'taunt' of the best move score if opponent is admin
	my @taunt = ();
	if (&bm_ingame_opp($g) =~ /^$VERBOSE_USERS$/i)
	{
		$taunt .= "   " if ($taunt ne "");
		$taunt .= "$g_bmai_best_move";
	}

	@taunt = ('taunt', $taunt) if defined $taunt;

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
	elsif ($att_type eq "surrender")
	{
		@vars = ();
		push @vars, 'next_game', "";
		push @vars, "attack_type", "Surrender Round";
		push @vars, "attack_die", "-1";
		push @vars, "game", "$id";
		push @vars, "action", "Beat People UP!";        # this is the confirmation
	}

	push @vars, @taunt;

	&bm_post_url("$BMW_PLAY$id", @vars);
	$t_state = undef;

	&bm_parse('parse_fight_result');

	&debug(PLAY, "Attack Result: $t_state\n");

	&bm_error("attack failed game $id\n") unless defined $t_state;

	&bm_error("did not recognize turbo ($turbo)\n") unless $t_state ne 'turbo';

	return 1;
}

# DESC: handles special rules, such as "The Flying Squirrel"
# PARAM: playturn game #, player #
# PRE: %{game . $g} hash setup
sub bm_playturn_fix_dice
{
	my $g = shift;
	my $p = shift;
	my $bm = ($p==0) ? &bm_ingame_my_bm($g) : &bm_ingame_opp_bm($g);
	my $num_dice = $#{g_dice . $p} + 1;
	my $dice_key = 'g_dice' . $p;

	if ($bm eq 'The Flying Squirrel')
	{
		for ($n=0; $n<$num_dice; $n++)
		{
			${$dice_key}[$n] = "k" . ${$dice_key}[$n];
		}
	}
	elsif ($bm eq 'Guillermo')
	{
		for ($n=0; $n<$num_dice; $n++)
		{
			${$dice_key}[$n] = "u" . ${$dice_key}[$n] if (${$dice_key}[$n] =~ /[A-Z]/);
		}
	}
}

sub bm_playturn
{
	my $g = shift;
	my $phase = shift;
	my $id = &bm_ingame_id($g);
	my $date = `$DATE`;
	chop $date;
	($date ne "") || &bm_error("Missing: $date");

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
	my $stale = shift;
	my $id = &bm_ingame_id($g);


	my $date = localtime(time);
	$date =~ s/(\S+ +\S+ +\d+).+ (\d+)/\1 \2/;
	my $opp = &bm_ingame_opp($g);
	my $my_bm = &bm_ingame_param($g, $MM_MY_BM);
	my $opp_bm = &bm_ingame_param($g, $MM_OPP_BM);
	my $score = &bm_ingame_param($g, $MM_SCORE);
	local($w, $l, $t) = split(m:/:, $score);

	my $result = ($w > $l) ? "win" : "loss";

	my $log = "$date\t$id\t$opp\t$my_bm\t$opp_bm\t$w\t$l\t$t\t$result\n";

	# only write to history if not stale
	if (!$stale)
	{
		open(HISTORY,">>$BMD_HISTORY") || &bm_error("could not open $BMD_HISTORY: $!");
		print HISTORY $log;
		close HISTORY;

                $games_finished++;

                $g_actions .= "\nCompleted: Game $id BM $my_bm VS $opp_bm ($opp) Score $score\n";
	}
        else
        {
                $g_actions .= "\nPurging: Game $id BM $my_bm VS $opp_bm ($opp) Score $score\n";
        }

	&debug(PURGE, "purging game $id: $log");

	&bm_post_url("$BMW_PURGE$id");
}

sub bm_playturns
{
	$games_played = 0;
	$games_ignored = 0;
	for ($g=0; $g<$g_games; $g++)
	{

		&debug(PLAY, "Playing game $g score " . &bm_ingame_score($g) . " status " . &bm_ingame_status($g)
			. " action " . &bm_ingame_action_name($g)
			. " " . &bm_ingame_delete_game($g)
			. "\n");

		my $opp = &bm_ingame_opp($g);
		my $delete_option =
			&bm_ingame_delete_game($g)
			&& ( (&bm_ingame_status($g) eq 'Waiting')
			   ||(&bm_ingame_status($g) eq 'Preround') );
		my $id = &bm_ingame_id($g);
		
		# ignore annoying players - don't even purge stale games. But play if winning or 1-1
		if ($opp =~ /^$LAMERS$/i)
		{
			my @scores = split(m|[\/\(\)]|, &bm_ingame_score($g));
			my $left = $scores[3] - max($scores[0],$scores[1]);
			my $played = $scores[0] + $scores[1];
			my $delta = $scores[0] - $scores[1];
			print "SCORE: $scores[0] - $scores[1] - $scores[2] - $scores[3]\n";
			print "LEFT: $left PLAYED: $played DELTA: $delta\n";
			#if ($delta<0 || ($delta==0 && $left==1))
			if (($delta<=0 && $left==1))
			{
				my $id = &bm_ingame_id($g);
				$games_ignored++;
				&debug(PLAY, "Ignoring game $id with lamer $opp\n");
				next;
			}
		}

		# HACK: purge stale games
		if ($::PURGE_STALE_GAMES && $delete_option)
		{
			&debug(PLAY, "Purging stale game $id\n");
			&bm_purgegame($g,1);
			next;
		}
		
		# DEBUG_GAME - skip if doesn't match
		if ($::DEBUG_GAME && $id != $::DEBUG_GAME)
		{
			&debug(PLAY, "Skipping non-debug game $id\n");
			next;
		}

		if (&bm_ingame_param($g,$MM_CLASS) ne "ready") {
			next;
		}
		elsif (&bm_ingame_status($g) eq 'Your Turn')
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
                # TODO: is this code even called anymore?
		elsif ( (&bm_ingame_status($g) eq 'Game Over') )
		{
			$games_finished++;
			&bm_purgegame($g);
		}
	}

	$g_results .= "$games_finished games completed\n" if ($games_finished>0);
	$g_results .= "$games_ignored games ignored\n" if ($games_ignored>0);
	$g_results .= "$games_played games played\n";
}

#######################################################################################
# create games
#######################################################################################

sub parse_creategame
{
	if (/Your game has been created/)
	{
		$t_result = 'success';
		return 0;
	}
	1;
}

# PARAM: the set, its val
sub bm_creategame
{
	my $set = shift;
	my $val = shift;

	&debug(CREATE, "Creating game in set $set ($val)\n");

	&bm_post_url($BMW_CREATE,
					'posted',1,
					'outof',$BMD_CREATE_OUTOFGAMES,
					'comment',"Play the AI! You will be assimilated and stuff.",
					'player','select',              # select my BM/sets
					'player_choice',$val."g", # comma delimited option list
					#'player_list',undef,   # the possible BM/sets in list form - not needed
					'opp','select',                 # select opp BM/sets
					'opp_choice',$val."g",  # comma delimited option list
					#'opp_list',undef,              # the possible BM/sets in list form - not needed
					'opp_random',1,                 # opp select randomly from choices
					);

	$t_result = undef;
	&bm_parse('parse_creategame');

	&bm_error("Failure to create game: $t_result") unless $t_result eq 'success';

	$g_actions .= "Created game in set $set.\n\n";
}

# DESC: find the IDs for sets
# POST: updates %t_setid
sub parse_creategames
{
	if ($t_state eq 'seeking') {
		if (/select.+player_sets/) {
			$t_state = 'reading';
		}
	} elsif ($t_state eq 'reading') {
		if (/option value=\"(\S+)\"\>(.+) \(/)
		{
			($id,$name) = ($1,$2);
			$name =~ s/ \(unlicensed\)$//i;
			$t_setid{$name} = $id;
		}
		elsif (m|/select|)
		{
			$t_state = 'finished';
			return 0;
		}
	}
	1;
}

# POST: update $t_open_games
# drp090103 - changed parsing rules (wasn't recognizing my open games)
sub parse_joingames
{
	if ($t_state eq 'seeking')
	{
		if (/my open games/i) {
			$t_state = 'reading';
		}
	}
	elsif ($t_state eq 'reading')
	{
		if (/Delete Game/) {
			$t_open_games++;
		}
		elsif (m|/table|)
		{
			$t_state = 'finished';
			return 0;
		}
	}
	1;
}

# PRE: $g_active_games
sub bm_creategames
{
	my $create_games = $BMD_DESIRED_ACTIVE_GAMES - $g_active_games;

	return unless $create_games>0;

	# check out JOIN GAMES and see how many I have open for opponents

	$t_state = 'seeking';
	$t_open_games = 0;
	&bm_get_url($BMW_JOIN);
	&bm_parse('parse_joingames');
	$create_games -= $t_open_games;

	if ($t_open_games > 0 )
	{
		&debug(CREATE, "Have $t_open_games open games.\n");
		$g_results .= "$t_open_games open games\n";
	}

	return unless $create_games>0;

	$t_state = 'seeking';
	&bm_get_url($BMW_CREATE);
	&bm_parse('parse_creategames');

	&debug(CREATE, "Creating $create_games games.\n");

	my $created = $create_games;

	while ($create_games-->0)
	{
		# pick a set
		my $set = $BMD_CREATE_GAME_SET[&dp_rand($#BMD_CREATE_GAME_SET+1)];

		&bm_error("bm_creategames: Could not find set ID for set $set") unless defined $t_setid{$set};

		# make the game
		&bm_creategame($set,$t_setid{$set});
	}

	$g_results .= "$created games created\n";
}

#######################################################################################
# main
#######################################################################################

# DETERMINE DATE
$g_date = `$DATE`;
chop $g_date;

# SET DEBUGGING LEVELS
&set_debug(undef,1);

# READ STATS
&bm_readstats;

# LOG IN
&dp_onexit('bm_onexit');
&bm_login();
# TODO: LOAD DATA or UPDATE DATA (stats on players and BM)
# &bm_listgames();
# &bm_scoregames();
# &bm_joingames();
&bm_mainmenu();
&bm_creategames();
## &bm_error("No games\n") unless $g_games>0;

&bm_playturns();

&dp_exit(0);

__END__

