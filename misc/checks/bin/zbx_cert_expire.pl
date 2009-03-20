#!/usr/bin/perl -s
use Time::Local;

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

my ($hour, $min, $sec, $day, $mon, $year) = ($3, $4, $5, $2, $1, $6) if $date =~ /\s*(\w+)\s+(\d+)\s+(\d+):(\d+):(\d+)\s+(\d+)/;
my $month = 0;

$month = 0 if $mon eq "Jan";
$month = 1 if $mon eq "Feb";
$month = 2 if $mon eq "Mar";
$month = 3 if $mon eq "Apr";
$month = 4 if $mon eq "May";
$month = 5 if $mon eq "Jun";
$month = 6 if $mon eq "Jul";
$month = 7 if $mon eq "Aug";
$month = 8 if $mon eq "Sep";
$month = 9 if $mon eq "Oct";
$month = 10 if $mon eq "Nov";
$month = 11 if $mon eq "Dec";

$time = timegm ($sec, $min, $hour, $day, $month, $year);

finish (0, "Date '$date' parse error") if $time <= 0;
$days = ($time - time ()) / 86400;

if ($days < 0) {
    $days = -$days;
    finish (0, "Certificate expired $days days ago");
}

$days = int ($days);

print "$days\n";
