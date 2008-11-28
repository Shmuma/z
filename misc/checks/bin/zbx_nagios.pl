#!/usr/bin/perl -sw

die "Not enough arguments!" if $#ARGV < 1;

my $check=$ARGV[0];

my $result=`$check`;

my ($state, $msg) = $result =~ /.*;(\d);(.*)$/;
print $state;
print STDERR $msg;
