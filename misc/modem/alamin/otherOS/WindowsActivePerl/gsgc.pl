#!/usr/bin/perl -w
#=====================================================================
# Patched version for Windows NT with ActivePerl+Digest::MD5
#=====================================================================
#---------------------------------------------------------------------
# gsgc
# Alamin GSM SMS Gateway Client
#---------------------------------------------------------------------
# (C) Andrés Seco Hernández, April 2000-May 2001
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
#---------------------------------------------------------------------

use strict;
use IO::Socket;
#use Sys::Syslog qw(:DEFAULT setlogsock);
use Digest::MD5 qw(md5_hex);

#--------------------
# Global vars
#--------------------
my $program_name = "gsgc";
my $program_version = "v0.3.6 - May 6, 2001";

my $exitcode = 0;
my $maxanswers = 10;
my $fullresponse = "";
my $challenge = "";

my $configfile = "/etc/alamin/gsgc.conf";

my $opt_debug = 1;
my $opt_verbose = 1;
my $opt_copyright = 0;

my $opt_port = "smsqp";
my $opt_port_number = 11201;
my $opt_host = "gsm";

my $opt_syslog = 1;
my $opt_syslogfacility = "local4";

my $opt_priority = 5;

my $opt_smsc = "default";

my $opt_user = "anonymous";
my $opt_password = "anonymous";

my $opt_function = "nothing";
my $opt_par1 = "";
my $opt_par2 = "";

read_config();
validate_options();

#setlogsock "unix";
#openlog($program_name,"pid",$opt_syslogfacility);

logit("info","Starting Alamin GSM SMS Gateway - Client") if ($opt_verbose);

if ($opt_copyright) {
  print "$program_name - $program_version\n";
  display_copyright();
  print localtime(time())."\n\n";
}

if ($opt_port =~ /\D/) {
# need to search in services table
  my $tmp_opt_port_number = getservbyname($opt_port,'tcp');
  if ($tmp_opt_port_number) {
    $opt_port_number = $tmp_opt_port_number;
  } else {
    logit("warning","Service ".$opt_port." not found. Using ".$opt_port_number." instead.") if ($opt_verbose);
  }
} else {
# just only a number port
  $opt_port_number = $opt_port;
}

# Opening socket
my $sock = IO::Socket::INET->new(PeerAddr => $opt_host,
                                   PeerPort => $opt_port_number,
                                   Proto    => 'tcp');

if ($sock) {
  if (waitfor("READY")) {
    my $auth_ok = 1;
    if ($opt_user ne "anonymous") {
      $auth_ok = process_auth();
    }
    if ($auth_ok) {
      CASE_FUNCTION: {
        if ($opt_function eq "send") {
          process_send(); last CASE_FUNCTION; }
      }
    }
  } else {
    $exitcode = 2;
  }
# Closing socket
  close $sock;
} else {
  print "Can't connect with server ".$opt_host.":".$opt_port."\n" if ($opt_verbose);
  logit("err","Can't connect with server ".$opt_host.":".$opt_port);
  $exitcode = 1;
}

#closelog;

exit $exitcode;

sub logit {
  my ($log_type, $log_message) = @_;
  if ($opt_syslog) {
#    system "logger","-p",$opt_syslogfacility.".".$log_type,"-t",$program_name."[".$$."]",$log_message;
#    openlog($program_name,"pid",$opt_syslogfacility);
#    syslog($log_type,$log_message);
    print localtime(time())." ".$program_name." ".$log_type." ".$log_message."\n";
#    closelog;
  }
}

#---------------------------------------------------------------------
sub process_auth {
  sendthis("user ".$opt_user."\n");
  while (1) {
    my $result = waitfor("OK","ERROR","READY");
    if ($result) {
      if ($result eq 2) {
        logit("err","Error login into server.");
        $exitcode = 4;
	return 0;
      } else {
        if ($result eq 3) {
          last;
        } else {
          $challenge = substr($fullresponse,3);
          logit("debug","User accepted.");
        }
      }
    } else {
      $exitcode = 2;
      return 0;
    }
  }
  if ($challenge) {
    sendthis("password ".md5_hex($challenge.$opt_password)."\n");
    while (1) {
      my $result = waitfor("OK","ERROR","READY");
      if ($result) {
        if ($result eq 2) {
          logit("err","Error login into server.");
          $exitcode = 4;
          return 0;
        } else {
          if ($result eq 3) {
            last;
          } else {
            logit("debug","Logged into server.");
          }
        }
      } else {
        $exitcode = 2;
        return 0;
      }
    }
    return 1;
  } else {
    $exitcode = 2;
    return 0;
  }
}

#---------------------------------------------------------------------
sub process_send {
  sendthis("send ".$opt_priority." - ".$opt_smsc." <list>\n");
  my @destinations;
  @destinations = split(/,/,$opt_par1);
  foreach (@destinations) {
    sendthis($_."\n");
  }
  sendthis("<EOL>\n");
  sendthis($opt_par2."<EOM>\n");
  while (1) {
    my $result = waitfor("OK","ERROR","READY");
    if ($result) {
      if ($result eq 2) {
        logit("err","Error sending message.");
        $exitcode = 3;
      } else {
        if ($result eq 3) {
          sendthis("CLOSE\n");
          if (!waitfor("BYE")) {
            $exitcode = 2;
          }
          last;
        } else {
          print "Message sent.\n" if ($opt_verbose);
          logit("info","Message sent.") if ($opt_verbose);
        }
      }
    } else {
      $exitcode = 2;
    }
  }
}

sub waitfor {
  my (@answer) = @_;
  my $exitfound = 0;
  my $counter = 0;
  while (<$sock>) {
    chomp;
    if (/\r$/) { chop; }
    chomp;
    $fullresponse = $_;
    logit("debug","Received: ".$_) if ($opt_debug);
    if ($answer[0]) {
      if (/^$answer[0]/) {
        $exitfound = 1;
        last;
      }
    }
    if ($answer[1]) {
      if (/^$answer[1]/) {
        $exitfound = 2;
        last;
      }
    }
    if ($answer[2]) {
      if (/^$answer[2]/) {
        $exitfound = 3;
        last;
      }
    }
    $counter++;
    if ($counter eq $maxanswers) {
      logit("err","Something is not good in the server, ",$counter," loops waiting for READY");
      last;
    }
  }
  return $exitfound;
}

sub sendthis {
  my ($texttosend) = @_;
  logit("debug","Sending: ".$texttosend) if ($opt_debug);
  print $sock $texttosend;
}

sub read_config {
#--------------------
# check for --configfile option
#--------------------
  my $nextconfigfile = 0;
  foreach (@ARGV) {
    /--version/ && do { display_version(); exit; };
    /--help/ && do { display_usage(); exit; };
    /--configfile/ && do { $nextconfigfile = 1; next; };
    if ($nextconfigfile) {
      $configfile = $_;
      $nextconfigfile = 0;
      next;
    }
  }
  if ($nextconfigfile) {
    print "Lost parameter for --configfile\n";
    exit 2;
  }

#--------------------
# read config file
#--------------------
  if (-f $configfile) {
    open (CONFFILE,"<$configfile");
    while (<CONFFILE>) {
      if (substr($_,0,1) ne "#") {
        my @field = split (/\s+/,$_);
        if ($field[0]) {
          CASE_CF: {
            if ($field[0] eq "debug") {
              $opt_debug = 1; last CASE_CF; }
            if ($field[0] eq "nodebug") {
              $opt_debug = 0; last CASE_CF; }
            if ($field[0] eq "verbose") {
              $opt_verbose = 1; last CASE_CF; }
            if ($field[0] eq "noverbose") {
              $opt_verbose = 0; last CASE_CF; }
            if ($field[0] eq "copyright") {
              $opt_copyright = 1; last CASE_CF; }
            if ($field[0] eq "nocopyright") {
              $opt_copyright = 0; last CASE_CF; }
            if ($field[0] eq "port") {
              $opt_port = $field[1]; last CASE_CF; }
            if ($field[0] eq "host") {
              $opt_host = $field[1]; last CASE_CF; }
            if ($field[0] eq "syslog") {
              $opt_syslog = 1; $opt_syslogfacility = $field[1]; last CASE_CF; }
	    if ($field[0] eq "nosyslog") {
	      $opt_syslog = 0; last CASE_CF; }
            if ($field[0] eq "priority") {
              $opt_priority = $field[1]; last CASE_CF; }
            if ($field[0] eq "smsc") {
              $opt_smsc = $field[1]; last CASE_CF; }
            if ($field[0] eq "user") {
              $opt_user = $field[1]; last CASE_CF; }
            if ($field[0] eq "password") {
              $opt_password = $field[1]; last CASE_CF; }
#	    logit("debug","Not recognised option in config file ignored: ".$field[0]) if ($opt_debug);
#	    print("Not recognised option in config file ignored: ".$field[0]."\n") if ($opt_debug);
          }
        }
      }
    }
    close (CONFFILE);
  }

#--------------------
# command-line options
#--------------------
  my $nextport = 0;
  my $nexthost = 0;
  my $nextsyslog = 0;
  my $nextpriority = 0;
  my $nextsmsc = 0;
  my $nextuser = 0;
  my $nextpassword = 0;
  my $nextfunction = 0;
  foreach (@ARGV) {
    /--configfile/ && do { $nextconfigfile = 1; next; };
    /--debug/ && do { $opt_debug = 1; next; };
    /--nodebug/ && do { $opt_debug = 0; next; };
    /--verbose/ && do { $opt_verbose = 1; next; };
    /--noverbose/ && do { $opt_verbose = 0; next; };
    /--copyright/ && do { $opt_copyright = 1; next; };
    /--nocopyright/ && do { $opt_copyright = 0; next; };
    /--port/ && do { $nextport = 1; next; };
    /--host/ && do { $nexthost = 1; next; };
    /--syslog/ && do { $opt_syslog = 1; $nextsyslog = 1; next; };
    /--nosyslog/ && do { $opt_syslog = 0; next; };
    /--priority/ && do { $nextpriority = 1; next; };
    /--smsc/ && do { $nextsmsc = 1; next; };
    /--user/ && do { $nextuser = 1; next; };
    /--password/ && do { $nextpassword = 1; next; };
    /--send/ && do { $opt_function = "send"; $nextfunction = 1; next; };
    if ($nextconfigfile) {
      $nextconfigfile = 0;
      next;
    }
    if ($nextport) {
      $opt_port = $_;
      $nextport = 0;
      next;
    }
    if ($nexthost) {
      $opt_host = $_;
      $nexthost = 0;
      next;
    }
    if ($nextsyslog) {
      $opt_syslogfacility = $_;
      $nextsyslog = 0;
      next;
    }
    if ($nextpriority) {
      $opt_priority = $_;
      $nextpriority = 0;
      next;
    }
    if ($nextsmsc) {
      $opt_smsc = $_;
      $nextsmsc = 0;
      next;
    }
    if ($nextuser) {
      $opt_user = $_;
      $nextuser = 0;
      next;
    }
    if ($nextpassword) {
      $opt_password = $_;
      $nextpassword = 0;
      next;
    }
    if ($nextfunction eq 1) {
      $opt_par1 = $_;
      $nextfunction = 2;
      next;
    }
    if ($nextfunction eq 2) {
      $opt_par2 = $_;
      $nextfunction = 0;
      next;
    }
    print "Invalid parameter: $_\n";
    display_usage();
    exit 1;
  }

  if ($nextport) {
    print "Lost parameter for --port\n";
    exit 2;
  }
  if ($nexthost) {
    print "Lost parameter for --host\n";
    exit 2;
  }
  if ($nextsyslog) {
    print "Lost parameter for --syslog\n";
    exit 2;
  }
  if ($nextpriority) {
    print "Lost parameter for --priority\n";
    exit 2;
  }
  if ($nextsmsc) {
    print "Lost parameter for --smsc\n";
    exit 2;
  }
  if ($nextsmsc) {
    print "Lost parameter for --user\n";
    exit 2;
  }
  if ($nextsmsc) {
    print "Lost parameter for --password\n";
    exit 2;
  }
  if ($nextfunction eq 1) {
    print "Lost parameter for --".$opt_function."\n";
    exit 2;
  }
}

sub validate_options {
  my @vo = ();

  if ($opt_smsc ne "default") {
    if (! isvalidnumber($opt_smsc)) {
      push @vo,["smsc","invalid number ".$opt_smsc];
    }
  }

  if ($opt_function eq "nothing") {
    push @vo,["function","you must specify an action (send)"];
  }

  if (@vo) {
    foreach (@vo) {
      print "Not valid value. Option: ".$_->[0].". Cause: ".$_->[1]."\n";
    }
    exit 3;
  }
}

sub isvalidnumber {
  my ($numbertocheck) = @_;
  my $ivn_response = 1;
  if ($numbertocheck =~ /([+]?)([\d]+)/) {
    my $tmp_numbertocheck = $1.$2;
    if ($numbertocheck ne $tmp_numbertocheck) {
      $ivn_response = 0;
    }
    if (length($tmp_numbertocheck) > 20) {
      $ivn_response = 0;
    }
  } else {
    $ivn_response = 0;
  }
  return $ivn_response;
}

sub display_copyright {
  print "Alamin GSM SMS Gateway\n";
  print "Copyright (C) Andres Seco Hernandez and others.\n\n";
  print "This program is free software; you can redistribute it and/or\n";
  print "modify it under the terms of the GNU General Public License\n";
  print "as published by the Free Software Foundation; either version 2\n";
  print "of the License, or (at your option) any later version.\n\n";
  print "This program is distributed in the hope that it will be useful,\n";
  print "but WITHOUT ANY WARRANTY; without even the implied warranty of\n";
  print "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n";
  print "GNU General Public License for more details.\n\n";
}

sub display_usage {
  print "Usage: $program_name \[--version\|--help\]\n";
  print "       $program_name \[--configfile config_file_name\]\n";
  print "            \[--debug\|--nodebug\]\n";
  print "            \[--verbose\|--noverbose\]\n";
  print "            \[--copyright\|--nocopyright\]\n";
  print "            \[--port port_number\]\n";
  print "            \[--host host_name\]\n";
  print "            \[--syslog facility\|--nosyslog\]\n";
  print "            \[--priority 1-9\]\n";
  print "            \[--smsc short_message_service_center_number\]\n";
  print "            \[--user username\]\n";
  print "            \[--password secret_password\]\n";
  print "            \[\[--send dest_number\[,more_dests\]... message_content\]\n";
  print "            \]\n";
}

sub display_version {
  print "$program_name - $program_version\n";
  display_copyright();
  print "To contact developers, please send mail to \<info\@alamin.org\>.\n";
  print "To ask for help, please send mail to \<alamin-user\@lists.sourceforge.net\>.\n";
  print "See the project web site at URL http://www.alamin.org\n";
}
