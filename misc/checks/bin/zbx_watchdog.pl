#!/usr/bin/perl -w
#
# check processes for overdrafts
#
# $Id: watchdog.pl,v 1.10 2007-10-31 08:43:07 andozer Exp $
#
use strict;

my $me  = $0;
$me     =~ s#^.*/([^.]+)\.[^/]*$#$1#o;
my $cf  = "$ENV{ZBX_CONFDIR}/checks/$me.conf";
my $cf2 = "/home/monitor/etc/watchdog.conf";
my %def = (
        'time_warn'     => 120,
        'time_crit'     => 600,
        'cpu_warn'      => 50,
        'cpu_crit'      => 90,
        'state_crit'    => 'R',
);
my %var = (
        'gzip' => { 'cpu_warn' => 99, 'cpu_crit' => 100 },
        'bzip2' => { 'cpu_warn' => 99, 'cpu_crit' => 100 },
        'cpio' => { 'cpu_warn' => 99, 'cpu_crit' => 100 },
);

my $conf;

foreach $conf (($cf, $cf2)) {
    if (-s $conf and open(CONF, "<$conf")) {
        while (<CONF>) {
                next if /^\s*(#.*)*$/o; # skip comments and empty lines
                next unless /^(\S+)\s*=\s*([^#]*)/o;

                my ($key, $val) = ($1, $2);
                chomp $val;

                if ($val =~ /^'(.*)'$/o) {
                        $val = $2;
                } elsif ($val =~ /^"(.*)"$/o) {
                        $val = $2;
                } elsif ($val =~ /=>/o) {
                        $var{$key} = { $val =~ /(\S+)\s*=>\s*([^,\s]+)/go };
                        next;
                } elsif ($val =~ /,/o) {
                        $var{$key} = [ split(/,\s/, $val) ];
                        next;
                }

                $var{$key} = $val;
        }

        close(CONF);
    }
}

my (@warn, @crit);

open(PS, "ps auwwx | tail -n +2 |") || die "cannot execute: $!\n";

while (<PS>) {
        my ($owner, $pid, $cpu, $state, $start, $time, $rest, $cmd, $args) = 
        /^(\S+)\s+(\d+)\s+(\d+)[,.]?\d?\s+\d+[,.]\d\s+\d+\s+\d+\s+\S+\s+(\S)\S*\s+(\S+)\s+(\d+):(\S+)\s+(\S+)\s*(.*)$/o;

        next if ($cmd =~ /idle/o || $cmd =~ /swi\d/o);

        my ($cpu_crit, $time_crit, $state_crit, $cpu_warn, $time_warn);

        if (!defined $var{$cmd} && $cmd =~ m,/([^/]*)$,) {
                $cmd = $1;
        }
        if (defined $var{$cmd}) {
                $cpu_crit = (defined $var{$cmd}->{'cpu_crit'}) ?
                                $var{$cmd}->{'cpu_crit'} : $def{"cpu_crit"};
                $cpu_warn = (defined $var{$cmd}->{'cpu_warn'}) ?
                                $var{$cmd}->{'cpu_warn'} : $def{"cpu_warn"};
                $state_crit = (defined $var{$cmd}->{'state_crit'}) ?
                                $var{$cmd}->{'state_crit'} : $def{"state_crit"};
                $time_crit = (defined $var{$cmd}->{'time_crit'}) ?
                                $var{$cmd}->{'time_crit'} : $def{"time_crit"};
                $time_warn = (defined $var{$cmd}->{'time_warn'}) ?
                                $var{$cmd}->{'time_warn'} : $def{"time_warn"};
        } else {
                ($cpu_crit, $time_crit, $state_crit, $cpu_warn, $time_warn) =
                ($def{"cpu_crit"}, $def{"time_crit"}, $def{"state_crit"},
                                        $def{"cpu_warn"}, $def{"time_warn"});
        }

        if ($cpu >= $cpu_crit && $time >= $time_crit && $state eq $state_crit)
        {
                push(@crit, "$cmd\[$pid\]: $state, cpu $cpu, spent $time:$rest since $start");
                next;
        }

        if ($cpu >= $cpu_warn && $time >= $time_warn && $state eq $state_crit)
        {
                push(@warn, "$cmd\[$pid\]: $state, cpu $cpu, spent $time:$rest since $start");
        }
}

my $out = "PASSIVE-CHECK:$me";

if ($#crit != -1) {
    print "2\n";
    print STDERR join(", ", @crit) , "\n"
} elsif ($#warn != -1) {
    print "1\n";
    print STDERR join(", ", @warn) , "\n"
} else {
    print "0\n";
}
