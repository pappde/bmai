##################################################################
# stats.pl
#
# Copyright (c) 2001-2020 Denis Papp. All rights reserved.
# denis@accessdenied.net
# https://github.com/pappde/bmai
#
# DESCRIPTION: read in a history file (BMAI games completed) and
# output some statistics
#
# USAGE: "perl stats.pl [games]"
# - if "games" specified dumps stats for games up to present and
#   stats for games since that period
# - if no params dumps overall stats only
#
# REVISION HISTORY:
# drp011901 - created
# drp032902 - dump overall
# drp033002 - "read X games" parameter and dump date of last game
##################################################################

$HISTORY = "history.txt";

# drp073101 - some BM get deleted, so read old and new data files. Read
# new first since BM changed sets.
@BM = ( "bm12.dat",
        "bm11.dat",
        "bm10.dat",
        "bm9.dat",
        "bm6.dat",
        "bm.dat" );

# pre-defined button men sets (for deleted BM that may not be in above files)
%set = ( 'Alpha', 'Deleted' );

# ignore everything up to this line (to do partial stats)
$IGNORE = 111;

$num_games = shift;

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
 'overall', 'overall_field',
);

# fix value of $overall_field so all games fall under the one category
$overall_field = 'total';

# optional param: stast for first $num_games
if (defined $num_games) {
        $stats{'period'} = 'period_field';
        $period_field = "initial";
}

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
        $last_date = $date;
	## $set = "Unknown" if ($set eq "" || !defined $set);

	die "unknown set '$my_bm'" unless defined $my_set;
	die "unknown set '$opp_bm'" unless defined $opp_set;

	$same_bm = ($my_bm eq $opp_bm) ? $my_bm : "No Match";

        $games++;

        # optional param: change which period games fall in once
        # have read $num_games. Also remember the date.
        if ($games==$num_games+1) {
                $initial_period_date = $date;
                $period_field = "recent"
        }

	foreach $s (keys %stats)
	{
                my $var = ${$stats{$s}};

                ${$s . '_games'}{$var}++;
		($result eq 'win')
                        ? ${$s . '_games_won'}{$var}++
                        : ${$s . '_games_lost'}{$var}++;
                ${$s . '_matches_won'}{$var}+=$w;
                ${$s . '_matches_lost'}{$var}+=$l;
                ${$s . '_matches_tied'}{$var}+=$t;
                ${$s . '_matches'}{$var}+= $w+$l+$t;
	}

}
close HISTORY;

print "\nLast Date: $last_date\n";
print "Initial Period: $initial_period_date\n" if defined $initial_period_date;

# dump 'overall' first
foreach $s ('period', 'overall', keys %stats)
{
        next if ($s eq 'period' && !defined $num_games);
        next if $dumped_stats{$s};
	print "\nSTATISTIC: $s\n";
	&dump_stats($s);
        $dumped_stats{$s} = 1;
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
                # bm5.dat - sometimes bm actually on next line
                chop $_;
                $_ .= <BM> if /displaybm/;
                if (m,displaybm.+bm=.+>(.+)</a>,i)
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
                                print "$BM: UNKNOWN SET - SETTING BM $bm_name -> SET $set\n";

				$bm_name = $found_name;
				$state = 'seekset';
			}
		}
                elsif (m,displaybm.+group=.+>(.+)</a>,i
                    || m,group=.+displaybm.+>(.+)</a>,i)
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
                elsif (m,group,i && ($state eq 'seekset'))
                {
                        die "unrecognized group line $_";
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
