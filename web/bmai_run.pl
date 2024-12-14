#!/usr/bin/perl --
# Copyright © 2008 Denis Papp

$BMAI = "bmai.pl";
#$PERL = "\\bin\\perl\\bin\\perl";
$PERL = "perl";
$ITER = 3;

# HACK
# Task Scheduler: doesn't have Y: drive mapped and starts in "C:\Windows"
system "net use y: \\\\NAS\\SHARE";
chdir("y:\\dev\\bmai");

&log("run.pl started\n");

for ($i = 0; $i<$ITER; $i++)
{
	&log("*** RUNNNING ITERATION: $iter\n");
        open(BMAI, "$PERL $BMAI|") || die $!;
	&log("bmai started\n");
        while (<BMAI>)
        {
                print;
        }
        close BMAI;
}

sub log
{
	my $msg = shift;

	open(F,">>test.log");
	open(F2,">>y:/dev/bmai/test2.log");
	open(F3,">>\\\\NAS\\SHARE\\dev\\bmai\\test3.log");
	print F "$msg";
	print F2 "$msg";
	print F3 "$msg";
	close F;
	close F2;
	close F3;
}
