# test BMAI settings

$GAMES = 100;
$IN_FILE = "in.txt";
$OUT_FILE = "out.txt";

$| = 1;

# the BM
@BM = (
	'12', 's8', '4', # '8'
);
$BM_DICE = $#BM + 1;

# for AI type 0 (BMAI1)
$PLY = 2;
$BRANCH = 1500;
$SIMS = 150;

# for AI type 2 (BMAI2)
@PLY = (2,3,4);
@BRANCH = (1000,1500,2500);
@SIMS = (100,150,250);

# @AI is a set of all types of players. Each entry
# is an array ref with 'type/ply/branch/sims'
push(@AI, [0,$PLY,$BRANCH,$SIMS]);
push(@AI, [1,$PLY,$BRANCH,$SIMS]);
foreach $p (@PLY)
{
foreach $b (@BRANCH)
{
foreach $s (@SIMS)
{
	push(@AI, [2, $p,$b,$s]);
}
}	
}

foreach $ai1 (@AI)
{
foreach $ai2 (@AI)
{
	next if ++$skip<26;
	&run_test;
}
}

sub run_test
{
	open (IN, ">$IN_FILE") || die $!;
	print IN "game\npreround\n";
	for ($p=0; $p<2; $p++)
	{
		print IN "player $p $BM_DICE 0\n";
		for ($d=0; $d<$BM_DICE; $d++)
		{
			print IN "$BM[$d]\n";
		}
	}
	print IN "debug SIMULATION 0\n";
	&print_ai(0,$ai1);
	&print_ai(1,$ai2);
	
	print IN "compare $GAMES\n";
	close IN;

	print "$$ai1[0] $$ai1[1] $$ai1[2] $$ai1[3] ";
	print "vs ";
	print "$$ai2[0] $$ai2[1] $$ai2[2] $$ai2[3] ";
	
	system "release\\bmai < $IN_FILE > $OUT_FILE";
	
	open(OUT, "<$OUT_FILE") || die $!;
	while (<OUT>)
	{
		if (/matches over (\d+) - (\d+)/)
		{
			print "= ";
			print "$1 - $2\n";
		}
	}
	close OUT;
}

sub print_ai
{
	my $p = shift;
	my $ai = shift;
	my $ply = $PLY;
	my $branch = $BRANCH;
	my $sims = $SIMS;
	
	print IN "ai $p ${$ai}[0]\n";
	print IN "ply $p $$ai[1]\nmaxbranch $p $$ai[2]\nsims $p $$ai[3]\n";
}	