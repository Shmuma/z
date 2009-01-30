#!/usr/bin/perl
#
# $Id: unispace.pl,v 1.14 2007-10-08 15:43:17 rnov Exp $
#
# This script checks all available space on all local
# filesystems and complains loudly when the usage
# exceeds the limit.
#
# TODO: possibly set different limits
# for different filesystems
#

use Sys::Hostname;
use strict;

my $me  = $0;
$me     =~ s#^.*/([^.]+)\.[^/]*$#$1#o;
my $cf  = "$ENV{ZBX_CONFDIR}/checks/$me.conf";
my $cf2  = "$ENV{HOME}/etc/unispace.conf";
my %var = ();
my %limit = ();
my $defaultlimit;
my $thislimit;
my $name;

my %warn = ();
my %crit = ();
my %unit = ();

my @F;
my $thislimitcrit;
my $thislimitwarn;
my $thisunit;
my $thisunitcod;
my $defaultlimitcrit;
my $defaultlimitwarn;
my $defaultunit;

my ($key, $conf);

foreach $conf (($cf, $cf2)) {
    if (-s $conf and open(CONF, "<$conf"))
    {
        while (<CONF>)
        {
                next if /^\s*(#.*)*$/o; # skip comments and empty lines
                next unless /^(\S+)\s*=\s*([^#]*)/o;

                my ($key, $val) = ($1, $2);
                chomp $val;

                if ($val =~ /^'(.*)'$/o)
                {
                        $val = $2;
                }
                elsif ($val =~ /^"(.*)"$/o)
                {
                        $val = $2;
                }
                elsif ($val =~ /=>/o)
                {
                        $var{$key} = { $val =~ /(\S+)\s*=>\s*(\S+)/go };
                        next;
                }
                elsif ($val =~ /,/o) 
                {
                        $var{$key} = [ split(/,\s/, $val) ];
                        next;
                }

                $var{$key} = $val;
        }

        close(CONF);
    }
}

foreach $key (keys %var)
{
        chomp $var{$key};
        $var{$key} =~ s/\s+//g;
        if ($key eq "unit")
        {
                next;
        }
        if ($var{$key} =~ /(.+)!(.+)!(.+)/)
        {
                $warn{$key} = $1;
                $crit{$key} = $2;
                $unit{$key} = $3;
        }
        else
        {
                if ($var{$key} =~ /(.+)!(.+)/)
                {
                        $warn{$key} = $1;
                        $crit{$key} = $2;
                }
                else
                {
                        $crit{$key} = $var{$key};
                }
        }
}

$defaultlimitcrit = (defined $crit{'limit'}) ? $crit{'limit'} : 0.99;
$defaultlimitwarn = (defined $warn{'limit'}) ? $warn{'limit'} : 0;
$defaultunit = (defined $unit{'limit'}) ? $unit{'limit'} : '%';
my $msg = "";
my $crit_flag = 0;
my $value;

my $result;

sub get_result
{
        my $v1 = $_[0];
        my $v2 = $_[1];
        my $res;

        $res = (1 - ($v1 / ($v1 + $v2)))*100;

        return int($res);
}

sub dfLinux
{
        open(DF, "df -P -l -k -t ext2 -t ext3 -t xfs |") || die "df: $!";
        $_ = <DF>;
        while (<DF>)
        {
                chomp;
                @F = split;
                $name = $F[5];
                $thislimitcrit = (defined $crit{$name}) ? $crit{$name} : $defaultlimitcrit;
                $thislimitwarn = (defined $warn{$name}) ? $warn{$name} : $defaultlimitwarn;
                $thisunit = (defined $unit{$name}) ? $unit{$name} : $defaultunit;
                $thisunitcod = 4;
                if ($thisunit =~ /k/i)
                {
                        $thisunitcod = 1;
                }
                if ($thisunit =~ /m/i)
                {
                        $thisunitcod = 2;
                }
                if ($thisunit =~ /g/i)
                {
                        $thisunitcod = 3;
                }
                if ($thisunitcod < 4) # it is Kb, Mb or Gb input limits
                {
                        if ($thisunitcod == 1)
                        {
                                $value = $F[3];
                        }
                        if ($thisunitcod == 2)
                        {
                                $value = int($F[3] / 1024);
                        }
                        if ($thisunitcod == 3)
                        {
                                $value = int($F[3] / 1024 / 1024);
                        }
                        if ($value < $thislimitcrit)
                        {
                                $msg .= "$value"."$thisunit free on $name. ";
                                $crit_flag = 1;
                        }
                        else
                        {
                                if ($value < $thislimitwarn)
                                {
                                        $msg .= "$value"."$thisunit free on $name. ";
                                }
                        }
                }
                else # it is % input limits
                {
                        if ($F[2] / ($F[2] + $F[3]) > $thislimitcrit)
                        {
                                $result = get_result($F[2], $F[3]);
                                $msg .= "$result"."$thisunit free on $name. ";
                                $crit_flag = 1;
                        }
                        else
                        {
                                if ($thislimitwarn)
                                {
                                        if ($F[2] / ($F[2] + $F[3]) > $thislimitwarn)
                                        {
                                                $result = get_result($F[2], $F[3]);
                                                $msg .= "$result"."$thisunit free on $name. ";
                                        }
                                }
                        }
                }
        }
        open(DF, "df -P -l -k -i -t ext2 -t ext3 -t xfs |") || die "df: $!";
        $_ = <DF>;
        while (<DF>)
        {
                chomp;
                @F = split;
                $name = $F[5];
                $thislimitcrit = (defined $crit{$name}) ? $crit{$name} : $defaultlimitcrit;
                $thislimitwarn = (defined $warn{$name}) ? $warn{$name} : $defaultlimitwarn;
                $thisunit = (defined $unit{$name}) ? $unit{$name} : $defaultunit;
                $thisunitcod = 4;
                if ($thisunit =~ /k/i)
                {
                        $thisunitcod = 1;
                }
                if ($thisunit =~ /m/i)
                {
                        $thisunitcod = 2;
                }
                if ($thisunit =~ /g/i)
                {
                        $thisunitcod = 3;
                }
                if ($thisunitcod < 4)
                {
                        $thislimitcrit = 0.99;
                        $thislimitwarn = 0;
                }
                if ($F[2] / ($F[2] + $F[3]) > $thislimitcrit)
                {
                        $result = get_result($F[2], $F[3]);
                        $msg .= "$result"."inodes free on $name. ";
                        $crit_flag = 1;
                }
                else
                {
                        if ($thislimitwarn)
                        {
                                if ($F[2] / ($F[2] + $F[3]) > $thislimitwarn)
                                {
                                        $result = get_result($F[2], $F[3]);
                                        $msg .= "$result"."inodes free on $name. ";
                                }
                        }
                }
        }
}

sub dfBSD
{
        open(DF, "df -k -i -t ufs |") || die "df: $!";
        $_ = <DF>;
        while (<DF>)
        {
                chomp;
                @F = split;
                $name = $F[8];
                $thislimitcrit = (defined $crit{$name}) ? $crit{$name} : $defaultlimitcrit;
                $thislimitwarn = (defined $warn{$name}) ? $warn{$name} : $defaultlimitwarn;
                $thisunit = (defined $unit{$name}) ? $unit{$name} : $defaultunit;
                $thisunitcod = 4;
                if ($thisunit =~ /k/i)
                {
                        $thisunitcod = 1;
                }
                if ($thisunit =~ /m/i)
                {
                        $thisunitcod = 2;
                }
                if ($thisunit =~ /g/i)
                {
                        $thisunitcod = 3;
                }
                if ($thisunitcod < 4) # it is Kb, Mb, or Gb input limits
                {
                        if ($thisunitcod == 1)
                        {
                                $value = $F[3];
                        }
                        if ($thisunitcod == 2)
                        {
                                $value = int($F[3] / 1024);
                        }
                        if ($thisunitcod == 3)
                        {
                                $value = int($F[3] / 1024 / 1024);
                        }
                        if ($value < $thislimitcrit)
                        {
                                $msg .= "$value"."$thisunit free on $name. ";
                                $crit_flag = 1;
                        }
                        else
                        {
                                if ($value < $thislimitwarn)
                                {
                                        $msg .= "$value"."$thisunit free on $name. ";
                                }
                        }
                }
                else  # it is % input limits
                {
                        if ($F[2] / ($F[2] + $F[3]) > $thislimitcrit)
                        {
                                $result = get_result($F[2], $F[3]);
                                $msg .= "$result"."$thisunit free on $name. ";
                                $crit_flag = 1;
                        }
                        else
                        {
                                if ($thislimitwarn)
                                {
                                        if ($F[2] / ($F[2] + $F[3]) > $thislimitwarn)
                                        {
                                                $result = get_result($F[2], $F[3]);
                                                $msg .= "$result"."$thisunit free on $name. ";
                                        }
                                }
                        }
                }
                if ($thisunitcod < 4)
                {
                        $thislimitcrit = 0.99;
                        $thislimitwarn = 0;
                }
                if ($F[5] / ($F[5] + $F[6]) > $thislimitcrit)
                {
                        $result = get_result($F[5], $F[6]);
                        $msg .= "$result inodes free on $name. ";
                        $crit_flag = 1;
                }
                else
                {
                        if ($thislimitwarn)
                        {
                                if ($F[5] / ($F[5] + $F[6]) > $thislimitwarn)
                                {
                                        $result = get_result($F[5], $F[6]);
                                        $msg .= "$result inodes free on $name. ";
                                }
                        }
                }
        }
}

sub Netsaint 
{
        my ($serv, $state, $mesg) = @_;
        print "$state\n";
        print STDERR "$mesg";
}

if ($^O =~ /linux/i) 
{
        dfLinux();
} 
else 
{
        dfBSD();
}

if ($msg ne "") 
{
        if ($crit_flag)
        {
                Netsaint($me, 2, $msg);
        }
        else
        {
                Netsaint($me, 1, $msg);
        }
} 
else 
{
        Netsaint($me, 0, "");
}
