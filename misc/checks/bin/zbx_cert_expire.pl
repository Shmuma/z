#!/usr/bin/perl -s
use Date::Parse;

# scripts parses output of openssl x509 for given certificate file and returns number of days to expire
sub finish 
{
    ($ret, $msg) = @_;

    print STDERR "$msg\n";
    print "$ret\n";
    exit 0;
}

finish (0, "You must provide name of certificate file in argument") if $#ARGV != 0;
finish (0, "Certificate file $ARGV[0] doesn't exists or not readable") unless -r $ARGV[0];

$date = `cat $ARGV[0] | openssl x509 -enddate -noout -text 2>/dev/null | grep notAfter | cut -f 2 -d =`;
chomp $date;

$time = str2time ($date);

finish (0, "Date '$date' parse error") if $time <= 0;
$days = ($time - time ()) / 86400;

if ($days < 0) {
    $days = -$days;
    finish (0, "Certificate expired $days days ago");
}

$days = int ($days);

print "$days\n";
