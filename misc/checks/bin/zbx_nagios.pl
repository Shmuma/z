#!/usr/bin/perl -sw


sub zdie
{
    my ($msg) = @_;
    print "2\n";
    print STDERR "$msg\n";
    exit (0);
}


zdie ("Not enough arguments!") if $#ARGV < 0;

my $check=$ARGV[0];

$check =~ s/\/|\.\.//g;

$check = "/home/monitor/agents/modules/".$check;

zdie ("File $check not found or not accessible") unless -x $check;
zdie ("Cannot spawn $check") unless open (RES, "$check |");

my $result=<RES>;

close RES;

my ($state, $msg) = $result =~ /.*;(\d);(.*)$/;

print "$state\n";
print STDERR "$msg\n";
