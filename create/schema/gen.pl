#!/usr/bin/perl
#
# ZABBIX
# Copyright (C) 2000-2005 SIA Zabbix
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

use utf8;

use Switch;
use File::Basename;

$file = dirname($0)."/schema.sql";	# Name the file
open(INFO, $file);			# Open the file
@lines = <INFO>;			# Read it into an array
close(INFO);				# Close the file

local $output;

%mysql=(
	"database"	=>	"mysql",
	"type"		=>	"sql",
	"before"	=>	"",
	"after"		=>	"",
	"table_options"	=>	" type=InnoDB",
	"t_bigint"	=>	"bigint unsigned",
	"t_id"		=>	"bigint unsigned",
	"t_integer"	=>	"integer",
	"t_time"	=>	"integer",
# It does not work for MySQL 3.x and <4.x (4.11?)
#	"t_serial"	=>	"serial",
	"t_serial"	=>	"bigint unsigned not null auto_increment unique",
	"t_double"	=>	"double(16,4)",
	"t_percentage"	=>	"double(5,2)",
	"t_varchar"	=>	"varchar",
	"t_char"	=>	"char",
	"t_image"	=>	"longblob",
	"t_history_log"	=>	"text",
	"t_history_text"=>	"text",
	"t_blob"	=>	"blob",
	"t_item_param"	=>	"text"
);

%c=(	"type"		=>	"code",
	"database"	=>	"",
	"after"		=>	"\t{0}\n};\n",
	"t_bigint"	=>	"ZBX_TYPE_UINT",
	"t_id"		=>	"ZBX_TYPE_ID",
	"t_integer"	=>	"ZBX_TYPE_INT",
	"t_time"	=>	"ZBX_TYPE_INT",
	"t_serial"	=>	"ZBX_TYPE_UINT",
	"t_double"	=>	"ZBX_TYPE_FLOAT",
	"t_percentage"	=>	"ZBX_TYPE_FLOAT",
	"t_varchar"	=>	"ZBX_TYPE_CHAR",
	"t_char"	=>	"ZBX_TYPE_CHAR",
	"t_image"	=>	"ZBX_TYPE_BLOB",
	"t_history_log"	=>	"ZBX_TYPE_TEXT",
	"t_history_text"=>	"ZBX_TYPE_TEXT",
	"t_blob"	=>	"ZBX_TYPE_BLOB",
	"t_item_param"	=>	"ZBX_TYPE_TEXT"
);

$c{"before"}="
#define ZBX_FIELD struct zbx_field_type
ZBX_FIELD
{
	char    *name;
	int	type;
	int	flags;
};

#define ZBX_TABLE struct zbx_table_type
ZBX_TABLE
{
	char    	*table;
	char		*recid;
	int		flags;
	ZBX_FIELD	fields[64];
};

static	ZBX_TABLE	tables[]={
";

%oracle=("t_bigint"	=>	"number(20)",
	"database"	=>	"oracle",
	"before"	=>	"",
	"after"		=>	"",
	"type"		=>	"sql",
	"t_id"		=>	"number(20)",
	"t_integer"	=>	"number(10)",
	"t_time"	=>	"number(10)",
	"t_serial"	=>	"number(20)",
	"t_double"	=>	"number(20,4)",
	"t_percentage"	=>	"number(5,2)",
	"t_varchar"	=>	"varchar2",
	"t_char"	=>	"varchar2",
	"t_image"	=>	"blob",
	"t_history_log"	=>	"varchar2(2048)",
	"t_history_text"=>	"varchar2(2000)",
	"t_blob"	=>	"varchar2(2048)",
	"t_item_param"	=>	"varchar2(2048)"
);

$copy_conv = "Nvl(To_Char(:new.field),'null')";
$copy_asis = "Replace(:new.field,'''','''''')||''''";

%oracle_copy=(
	"t_bigint"	=>	"copy_conv",
	"database"	=>	"oracle_copy",
	"before"	=>	"",
	"after"		=>	"",
	"type"		=>	"triggers",
	"t_id"		=>	"copy_conv",
	"t_integer"	=>	"copy_conv",
	"t_time"	=>	"copy_conv",
	"t_serial"	=>	"copy_conv",
	"t_double"	=>	"copy_conv",
	"t_percentage"	=>	"copy_conv",
	"t_varchar"	=>	"copy_asis",
	"t_char"	=>	"copy_asis",
	"t_image"	=>	"copy_asis",
	"t_history_log"	=>	"copy_asis",
	"t_history_text"=>	"copy_asis",
	"t_blob"	=>	"copy_asis",
	"t_item_param"	=>	"copy_asis"
);


$oracle_copy{"before"} = "
set define off

create or replace type system.TDMLRepCmd
as object
(
  csqlCmd varchar2(4000)
);
/

create or replace and resolve java source named system.\"TReplicate2MySQL\"
as

import java.io.*;
import java.sql.*;
import com.mysql.jdbc.jdbc2.optional.MysqlDataSource;

public class TReplicate2MySQL
{
 public static void RunSQL
 (
   String csqlCmd
 ) throws java.sql.SQLException
 {
   MysqlDataSource dsrTgt;
   dsrTgt = new MysqlDataSource();
   dsrTgt.setURL(\"jdbc:mysql://monitor-eto.yandex.net/zabbix?user=ztop&password=oracle\");
   Connection ssnTgt = dsrTgt.getConnection();
   PreparedStatement qryInsertHost = ssnTgt.prepareStatement
   (
     csqlCmd
   );
   qryInsertHost.execute();
   ssnTgt.close();
 }
}
/

create or replace package system.Replicate2MySQL
as
  procedure RunSQL(csqlCmd in varchar2);
  procedure PostRunSQL
  (
    context in raw,
    reginfo in sys.aq\$_reg_info,
    descr in sys.aq\$_descriptor,
    payload in varchar2,
    payloadl in number
  );  
end;
/

create or replace package body system.Replicate2MySQL
as
  procedure RunSQL_Immediate(csqlCmd in varchar2) is language java name 'TReplicate2MySQL.RunSQL(java.lang.String)';
  procedure PostRunSQL
  (
    context in raw,
    reginfo in sys.aq\$_reg_info,
    descr in sys.aq\$_descriptor,
    payload in varchar2,
    payloadl in number
  )
  is 
    optsDeq Dbms_AQ.Dequeue_Options_T;
    prpMsg Dbms_AQ.Message_Properties_T;
    hMsg raw(16);
    msgReplCmd TDMLRepCmd;
  begin
    optsDeq.msgid := descr.msg_id;
    optsDeq.consumer_name := descr.consumer_name;
    Dbms_AQ.Dequeue
    (
      queue_name => descr.queue_name,
      dequeue_options => optsDeq,
      message_properties => prpMsg,
      payload => msgReplCmd,
      msgid => hMsg
    );
    RunSQL_Immediate(msgReplCmd.csqlCmd);
    commit;
  end;
    
  procedure RunSQL(csqlCmd in varchar2)
  is 
    optsEnq  dbms_aq.enqueue_options_t;
    prpMsg dbms_aq.message_properties_t;
    idMsg       RAW(16);
  --  rcpt_list dbms_aq.aq\$_recipient_list_t;
  begin
    Dbms_AQ.Enqueue
    (
      queue_name => 'system.q_replcmd',
      enqueue_options => optsEnq,
      message_properties => prpMsg,
      payload => TDMLRepCmd(csqlCmd),
      msgid => idMsg
    );
  end;
end;
/

begin
  Dbms_AQAdm.Create_Queue_Table
  (
    queue_table => 'system.tq_replcmd',
    multiple_consumers => true,
    queue_payload_type => 'system.TDMLRepCmd',
    primary_instance => 1,
    secondary_instance => 2
  );
end;
/

begin
  Dbms_AQAdm.Create_Queue
  (
    queue_name  => 'system.q_replcmd',
    queue_table => 'system.tq_replcmd'
  );
end;
/

begin
  dbms_aqadm.add_subscriber
  (
    queue_name => 'system.q_replcmd',
    subscriber => sys.aq\$_agent( 'recipient', null, null )
  );
end;
/

declare
  regCallback sys.aq\$_reg_info;
  lstReg sys.aq\$_reg_info_list;
begin
  regCallback := sys.AQ\$_reg_info
  (
    'system.q_replcmd:recipient',
    Dbms_AQ.Namespace_AQ,
    'plsql://system.Replicate2MySQL.PostRunSQL?PR=1',
    HEXTORAW('FF')
  );
  lstReg := sys.AQ\$_reg_info_list(regCallback);
  Dbms_AQ.Register(reg_list => lstReg, reg_count => 1);
  commit;
end;
/

begin
  Dbms_AQAdm.Start_Queue
  (
    queue_name  => 'system.q_replcmd'
  );
end;
/
grant execute on replicate2mysql to zabbix;
/
create public synonym Repicate2MySQL for system.Replicate2MySQL;
/
";

%postgresql=("t_bigint"	=>	"bigint",
	"database"	=>	"postgresql",
	"before"	=>	"",
	"after"		=>	"",
	"type"		=>	"sql",
	"table_options"	=>	" with OIDS",
	"t_id"		=>	"bigint",
	"t_integer"	=>	"integer",
	"t_serial"	=>	"serial",
	"t_double"	=>	"numeric(16,4)",
	"t_percentage"	=>	"numeric(5,2)",
	"t_varchar"	=>	"varchar",
	"t_char"	=>	"char",
	"t_image"	=>	"bytea",
	"t_history_log"	=>	"varchar(255)",
	"t_history_text"=>	"text",
	"t_time"	=>	"integer",
	"t_blob"	=>	"text",
	"t_item_param"	=>	"text"
);

%sqlite=("t_bigint"	=>	"bigint",
	"database"	=>	"sqlite",
	"before"	=>	"",
	"after"		=>	"",
	"type"		=>	"sql",
	"t_id"		=>	"bigint",
	"t_integer"	=>	"integer",
	"t_time"	=>	"integer",
	"t_serial"	=>	"serial",
	"t_double"	=>	"double(16,4)",
	"t_percentage"	=>	"double(5,2)",
	"t_varchar"	=>	"varchar",
	"t_char"	=>	"char",
	"t_image"	=>	"longblob",
	"t_history_log"	=>	"text",
	"t_history_text"=>	"text",
	"t_blob"	=>	"blob",
	"t_item_param"	=>	"text"
);

sub newstate
{
	local $new=$_[0];

	switch ($state)
	{
		case "field"	{
			if($output{"type"} eq "sql" && $new eq "index") { print $pkey; }
			if($output{"type"} eq "sql" && $new eq "table") { print $pkey; }
			if($output{"type"} eq "code" && $new eq "table") { print ",\n\t\t{0}\n\t\t}\n\t},\n"; }

			if($output{"type"} eq "triggers" && $new ne "field" && !defined ($oracle_exc{$table_name}))
                        {
                            # insert trigger
                            $typed_fields = join ",", @tr_typed_fields;
                            $typed_fields =~ s/\([^\)]*\)//g;
                            $tr_ins =~ s/:new\.//g;

                            print "create or replace function zabbix.ins_${table_name} ($typed_fields) return varchar2\n";
                            print "is\nbegin\nreturn\n";
                            print "'insert into ${table_name} (".(join ",", @tr_fields).") values ('\n";
                            print "${tr_ins}\n";
                            print "||')';\nend;\n/\nshow errors\n";

                            print "create or replace trigger zabbix.trai_${table_name} after insert on zabbix.${table_name} for each row\n".
                                "begin\nReplicate2MySQL.RunSQL\n(\n";
                            print "zabbix.ins_${table_name} (:new.".(join ",:new.", @tr_fields).")\n";
                            print ");\nend;\n/\nshow errors\n";

                            # update trigger
                            print "create or replace trigger zabbix.trau_${table_name} after update on zabbix.${table_name} for each row\n".
                                "begin\nReplicate2MySQL.RunSQL\n(\n";
                            print "'update zabbix.${table_name} set '\n";
                            print "${tr_upd}";
                            $op = " || ' where ";
                            foreach $k (split (/,\s*/, $tr_pk_field)) {
                                if ($fields_kind{$k} eq "copy_conv") {
                                    print "$op ${k}='||:old.${k} ";
                                }
                                else {
                                    print "$op ${k}='''||:old.${k}||'''' ";
                                }
                                $op = "|| ' and ";
                            }
                            print ");\nend;\n/\nshow errors\n";

                            # delete trigger
                            print "create or replace trigger zabbix.trad_${table_name} after delete on zabbix.${table_name} for each row\n".
                                "begin\nReplicate2MySQL.RunSQL\n(\n".
                                "'delete from zabbix.${table_name} '\n";

                            $op = " || ' where ";
                            foreach $k (split (/,\s*/, $tr_pk_field)) {
                                if ($fields_kind{$k} eq "copy_conv") {
                                    print "$op ${k}='||:old.${k} ";
                                }
                                else {
                                    print "$op ${k}='''||:old.${k}||'''' ";
                                }
                                $op = "|| ' and ";
                            }
                            print "\n);\nend;\n/\nshow errors\n";
                        }

                        if($output{"type"} ne "triggers" && $new eq "field") { print ",\n" }
		}
		case "index"	{
			if($output{"type"} eq "sql" && $new eq "table") { print "\n"; }
			if($output{"type"} eq "code" && $new eq "table") { print ",\n\t\t{0}\n\t\t}\n\t},\n"; }
		}
	 	case "table"	{
			print "";
		}
	}
	$state=$new;
}

sub process_table
{
	local $line=$_[0];
	local $tmp;

	newstate("table");
	($table_name,$pkey,$flags)=split(/\|/, $line,4);

	if($output{"type"} eq "code")
	{
#	        {"services",    "serviceid",    ZBX_SYNC,
		if($flags eq "")
		{
			$flags="0";
		}
		print "\t{\"${table_name}\",\t\"${pkey}\",\t${flags},\n\t\t{\n";
	}
        elsif($output{"type"} eq "triggers")
	{
            unless (defined $oracle_exc{$table_name}) 
            {
                $tr_upd = $tr_ins = "";
                $tr_pk_field = $pkey;
                @tr_fields = ();
                @tr_typed_fields = ();
                %fields_kind = ();
            }
        }
	else
	{
		if($pkey ne "")
		{
			$pkey=",\n\tPRIMARY KEY ($pkey)\n)";
		}
		else
		{
			$pkey="\n)";
		}
		$tmp=$output{"table_options"};
		$pkey="$pkey$tmp;\n";
		print "CREATE TABLE $table_name (\n";
	}
}

sub process_field
{
	local $line=$_[0];

	newstate("field");
	($name,$type,$default,$null,$flags)=split(/\|/, $line,5);
	($type_short)=split(/\(/, $type,2);
	if($output{"type"} eq "code")
	{
		$type=$output{$type_short};
#{"linkid",      ZBX_TYPE_INT,   ZBX_SYNC},
		if($flags eq "")
		{
			$flags="0";
		}
		print "\t\t{\"${name}\",\t$type,\t${flags}}";
	}
        elsif($output{"type"} eq "triggers")
        {
            unless (defined $oracle_exc{$table_name}) 
            {
                push @tr_fields, $name;
                push @tr_typed_fields, "$name in $oracle{$type_short}";
                $out = $output{$type_short};
                $out = $out eq "copy_conv" ? $copy_conv : $copy_asis;
                $out =~ s/field/$name/g;
                $tr_ins .= "||','\n" if $tr_ins;
                $tr_upd .= "||','\n" if $tr_upd;
                if ($output{$type_short} eq "copy_conv") {
                    $tr_ins .= "||$out";
                    $tr_upd .= "||' $name='||$out";
                }
                else {
                    $tr_ins .= "||''''||$out";
                    $tr_upd .= "||' $name='''||$out";
                }
                $fields_kind{$name} = $output{$type_short};
            }
        }
	else
	{
		$a=$output{$type_short};
		$_=$type;
		s/$type_short/$a/g;
		$type_2=$_;
		if($default ne "")	{ $default="DEFAULT $default"; }
		# Special processing for Oracle "default 'ZZZ' not null" -> "default 'ZZZ'. NULL=='' in Oracle!"
		if(($output{"database"} eq "oracle") && (0==index($type_2,"varchar2")))
		{
		#	$default="DEFAULT NULL";
			$null="";
		}
		print "\t$name\t\t$type_2\t\t$default\t$null";
	}
}

sub process_index
{
	local $line=$_[0];
	local $unique=$_[1];

	newstate("index");

	if($output{"type"} eq "code" || $output{"type"} eq "triggers")
	{
		return;
	}

	($name,$fields)=split(/\|/, $line,2);
	if($unique == 1)
	{
		print "CREATE UNIQUE INDEX ${table_name}_$name\ on $table_name ($fields);\n";
	}
	else
	{
		print "CREATE INDEX ${table_name}_$name\ on $table_name ($fields);\n";
	}
}

sub usage
{
	printf "Usage: $0 [c|mysql|oracle|oracle_copy|php|postgresql|sqlite]\n";
	printf "The script generates ZABBIX SQL schemas and C/PHP code for different database engines.\n";
	exit;
}

sub main
{
	if($#ARGV!=0)
	{
		usage();
	};

	$format=$ARGV[0];
	switch ($format) {
		case "c"		{ %output=%c; }
		case "mysql"		{ %output=%mysql; }
		case "oracle"		{ %output=%oracle; }
		case "oracle_copy"	{ %output=%oracle_copy; }
		case "php"		{ %output=%php; }
		case "postgresql"	{ %output=%postgresql; }
		case "sqlite"		{ %output=%sqlite; }
		else			{ usage(); }
	}

	print $output{"before"};

	foreach $line (@lines)
	{
		$_ = $line;
		$line = tr/\t//d;
		$line=$_;

		chop($line);

		($type,$line)=split(/\|/, $line,2);

		utf8::decode($type);

		switch ($type) {
			case "TABLE"	{ process_table($line); }
			case "INDEX"	{ process_index($line,0); }
			case "UNIQUE"	{ process_index($line,1); }
			case "FIELD"	{ process_field($line); }
		}
	}

}

# read exclude list for oracle
open EXC, "<oracle_rep_exclude.txt" || die "Cannot open exclude file for oracle";
while (<EXC>) { chomp; $oracle_exc{$_} = 1; };
close EXC;

main();
newstate("table");
print $output{"after"};
