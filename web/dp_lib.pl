##################################################################
# dp_lib.pl
#
# Copyright (c) 2001-2020 Denis Papp. All rights reserved.
# denis@accessdenied.net
# https://github.com/pappde/bmai
#
# DESCRIPTION: useful functions library
#
# VERSION: 1.0.1
#
# REVISION HISTORY:
# drp011901 - created
# drp031302 - added &dp_rand()
##################################################################

# DEPENDENCIES
require FileHandle;
require LWP::UserAgent;
require HTTP::Cookies;

# SETTINGS
$REFERER_PAGE = undef;
$AGENT = 'Mozilla/4.5';	# could add [en] (Win95; I)
$DEFAULT_TRIES = 3;
$RETRY_WAIT = 30;		# seconds between get_url() retries
$UA_TIMEOUT = 180;

# GLOBALS
my $g_ua = undef;
my $g_cook = undef;			# cookie jar
my %g_debug_component;
my $g_default_all = 0;
my @g_exit_list = ();
my $g_initialized = 0;

##############################################################################################
# general functions
##############################################################################################

# PARAM: exit code
sub dp_exit
{
	my $code = shift;
	my $func;
	foreach $func (@g_exit_list) { print "dp_exit: $func\n"; &$func(); }
	exit($code);
}

# PARAM: function name to call on exit
sub dp_onexit
{
	my $func = shift;
	push(@g_exit_list,$func);
}

# PARAM: fmt, params
sub error
{	
	my $fmt = shift;
	$fmt = "ERROR: $fmt";
	&debug(-1, $fmt, @_);
	&dp_exit(1);
}

# PARAM: component, format, parameters.  Component undef means force.
sub debug
{
	my $component = shift;
	return unless (!defined $component || $g_debug_component{$component} || $g_debug_all);
	my $fmt = shift;
	print sprintf("$component: " . $fmt, @_);
}

# PARAM: component, on/off (1/0).  Component undef means all
sub set_debug
{
	my $component = shift;
	my $on = shift;
	if (!defined $component)
	{
		$g_debug_all = $on;
	}
	else
	{
		$g_debug_component{$component} = $on;
	}
}

##############################################################################################
# utilities
##############################################################################################

# RETURNS: random int from 0..param-1
sub dp_rand
{
	return int(rand()*shift); 
}

##############################################################################################
# HTTP functions
##############################################################################################

sub init
{
	$g_ua = new LWP::UserAgent;
	$g_ua->timeout($UA_TIMEOUT);
	$g_ua->agent($AGENT);	

	$g_cook = HTTP::Cookies->new(file => 'cookies');
	$g_ua->cookie_jar($g_cook);

	$g_initalized = 1;

	&dp_onexit('uninit');
}

sub uninit
{
	return unless $g_initialized;

	undef $g_cook; undef $g_ua;

	$g_initialized = 0;
}

# PARAM: tries (undef for default), method (GET/POST), outfile, url, postvar, postval, postvar, postval, ...
# POST: called recursively
# RETURN: none. On error, calls error()
sub get_url
{
	local(@save_param) = @_;
	my $retry = shift;
	my $method = shift;
	my $outfile = shift;
	my $url = shift;
	my $req; 
	my $var;
	my $res;

	# set default retry
	$save_param[0] = $retry = $DEFAULT_TRIES unless defined $retry;

	# handle POST
	if ($method eq POST)
	{
		my $added = 0;
		# if $url already contains a '?', then modify $added
		$added++ if ($url =~ m,\?,);
		while ($var = shift(@_))
		{
			$val = shift @_;
			&debug(GET, "posting $var = $val\n");
			$url .= ($added++>0) ? '&' : '?';
			$url .= &escape($var);
			$url .= '=';
			$url .= &escape($val);
		}
	}

	# set up request
	$req = new HTTP::Request GET => $url;
	if ($url =~ m,//(\S+):(\S+)@,) {
	  $req->authorization_basic($1,$2);
	}
	# Qedit doesn't like lines >160
	my $debug_url = $url;
	if (length($debug_url)>120)
	{
		$debug_url = substr($url,0,120);
		$debug_url .= " ...";
	}
	&debug(GET, "$method $debug_url\n");
	#$req->referer($MAIN_PAGE);
	#$req->content_type('application/x-www-form-urlencoded');
	$req->header('Referer' => $REFERER_PAGE) if defined $REFERER_PAGE;
	$req->header('User-Agent' => $AGENT);
	if ($method ne POST)
	{
		while ($var = shift(@_))
		{
			$val = shift @_;
			&debug(GET, "pushing $var = $val\n");
			$req->push_header($var,$val);
		}
	}
	$res = $g_ua->request($req,$outfile);
	if ($retry>0 && $res->message =~ /Could not connect/) {
	  undef $req; undef $res;
	  #&debug("Waiting $g_cyclewait seconds and retrying $url\n");
	  sleep($RETRY_WAIT);
	  &get_url(@save_param);
	} else {
	  &error("%s (%s)\n",$res->message,$res->code)
		if ($res->message ne OK);
	  undef $req; undef $res;
	}
}

# DESC: 'http-escapes' given text
sub escape
{
    my $var = shift;
    $new = "";
    for ($i=0;$i<length($var);$i++)
    {
	$char = substr($var,$i,1);
	$char = sprintf("%%%X",unpack("c",$char))
		if ($char !~ /[a-zA-Z0-9 ]/);
	$new .= $char;
    }
    $new =~ s/ /\+/g;
    return $new;
}

##############################################################################################
# HTML parsing functions
##############################################################################################

# DESC: one simple function to give to parse.  It simply outputs the result
sub parse_print
{
	print;
	return 1;
}

# PARAM: line handler (return 0 to terminate prematurely) and file
sub parse
{
	my $handler = shift;
	my $file = shift;

	&error("parse function ($handler) is not defined") unless defined $handler;

	open (PARSE_IN, "<$file") || &error("reading $file");
	while (<PARSE_IN>)
	{
		last unless &$handler;
	}
	close PARSE_OUT;
}

# PARAM: reference to the string to parse
# POST: modifies param
# RETURNS: returns first non-empty token outside HTML tags
# FIX: there is a better way!
sub get_token
{
	my $stringref = shift;
	my $string = $$stringref;
	my $r; 
	my $token;
	## $string =~ s/\&nbsp\;//g;

	# strip leading space
	$string =~ s/^\s*//;

	#&debug(TOKEN, "$string\n");

	# string first tags (skips blank tokens)
	while ($string =~ s/^<.+?>//)
	{
		#&debug(TOKEN,"step $string\n");
	}

	#&debug(TOKEN, "shortened to $string\n");

	# break into token and remainder of string
	if ($string =~ /</)
	{
		($token, $r) = $string =~ /(.*?)(<.+)/;
		$string = $r;
	}
	else
	{
		$token = $string;
		$string = "";
	}
	#&debug(TOKEN, "match $token remaining $string\n");

	# modify passed string
	$$stringref = $string;
	
	return $token;
}

sub max
{
	my $l = shift;
	my $r = shift;
	return $r > $l ? $r : $l;
}	 

1;

__END__














