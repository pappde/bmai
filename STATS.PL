##################################################################
# stats.pl
#
# Copyright (c) 2001 Denis Papp. All rights reserved.
# denis@accessdenied.net
# http://www.accessdenied.net/bmai
#
# DESCRIPTION: read in a history file (BMAI games completed) and
# output some statistics
#
# REVISION HISTORY:
# drp011901 - created
##################################################################

$HISTORY = "history.txt";

# drp073101 - some BM get deleted, so read old and new data files
@BM = ( "bm.dat", "bm2.dat" );

# ignore everything up to this line (to do partial stats)
$IGNORE = 111;

&read_bm;

# with set
# against set
# with BM
# against BM
# against opp

# this array contains: statname, varname
%stats = (
 'set', 'my_set',
 'vs_set', 'opp_set',
 'bm', 'my_bm',
 'vs_bm', 'opp_bm',
 'opp', 'opp',
 'same_bm', 'same_bm',
);

my $lines = 0;
open (HISTORY, "<$HISTORY") || die $!;
while (<HISTORY>)
{
	next if ++$lines < $IGNORE;
	chop;
	@fields = split(/\t/, $_);
	if ($#fields<8)
	{
		print "COULD NOT PARSE: $_\n";
		next;
	}

	($date, $id, $opp, $my_bm, $opp_bm, $w, $l, $t, $result) = @fields;
	$my_set = $set{$my_bm};
	$opp_set = $set{$opp_bm};
	## $set = "Unknown" if ($set eq "" || !defined $set);

	die "unknown set '$my_bm'" unless defined $my_set;
	die "unknown set '$opp_bm'" unless defined $opp_set;

	$same_bm = ($my_bm eq $opp_bm) ? $my_bm : "No Match";

	foreach $s (keys %stats)
	{
		${$s . '_games'}{${$stats{$s}}}++;
		($result eq 'win')
			? ${$s . '_games_won'}{${$stats{$s}}}++
			: ${$s . '_games_lost'}{${$stats{$s}}}++;
		${$s . '_matches_won'}{${$stats{$s}}}+=$w;
		${$s . '_matches_lost'}{${$stats{$s}}}+=$l;
		${$s . '_matches_tied'}{${$stats{$s}}}+=$t;
		${$s . '_matches'}{${$stats{$s}}}+= $w+$l+$t;
	}
}
close HISTORY;

foreach $s (keys %stats)
{
	print "\nSTATISTIC: $s\n";
	&dump_stats($s);
}

exit;

# END

sub dump_stats
{
	my $s = shift;
	my @lines = ();
	foreach $k (keys %{$s . '_games'})
	{
		$w = ${$s . '_games_won'}{$k};
		$l = ${$s . '_games_lost'}{$k};
		$p = $w / ($w + $l) * 100;
		$l = sprintf("%-24s%3d games (%2d %2d %5.1f%%) %3d matches (%4d %4d %3d)\n",
			$k,
			${$s . '_games'}{$k},
			$w,
			$l,
			$p,
			${$s . '_matches'}{$k},
			${$s . '_matches_won'}{$k},
			${$s . '_matches_lost'}{$k},
			${$s . '_matches_tied'}{$k} );
		push @lines, $l;
	}

	@sorted = sort {
		($aval) = $a =~ /\s(\d+) games/;
		($bval) = $b =~ /\s(\d+) games/;
		$bval <=> $aval;
	} @lines;

	foreach $l (@sorted)
	{
		print $l;
	}
}

sub read_bm
{
	$state = 'seek';
	foreach $BM (@BM)
	{
	  open(BM, "<$BM") || die $!;
	  while (<BM>)
	  {
		if (m,displaybm.+bm.+>(.+)</a>,)
		{
			if ($state eq 'seek')
			{
				$bm_name = $1;
				$state = 'seekset';
			}
			else # no set?
			{
				$found_name = $1;
				$set = $bm_name;
				&read_bm_found_set;
				print "UNKNOWN SET, SETTING BM $bm_name -> SET $set\n";

				$bm_name = $found_name;
				$state = 'seekset';
			}
		}
		elsif (m,displaybm.+group.+>(.+)</a>,)
		{
			if ($state eq 'seekset')
			{
				$set = $1;
				&read_bm_found_set;
			}
			else
			{
				die "mismatched group line $_";
			}
		}
	  }
	  close BM;
	}
}

sub read_bm_found_set
{
	$set =~ s/\s+$//;
	$set{$bm_name} = $set;
	$state = 'seek';
}
