
set define off

create or replace type system.TDMLRepCmd
as object
(
  csqlCmd varchar2(4000)
);
/

create or replace and resolve java source named system."TReplicate2MySQL"
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
   dsrTgt.setURL("jdbc:mysql://monitor-eto.yandex.net/zabbix?user=ztop&password=oracle");
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
    reginfo in sys.aq$_reg_info,
    descr in sys.aq$_descriptor,
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
    reginfo in sys.aq$_reg_info,
    descr in sys.aq$_descriptor,
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
  --  rcpt_list dbms_aq.aq$_recipient_list_t;
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
    subscriber => sys.aq$_agent( 'recipient', null, null )
  );
end;
/

declare
  regCallback sys.aq$_reg_info;
  lstReg sys.aq$_reg_info_list;
begin
  regCallback := sys.AQ$_reg_info
  (
    'system.q_replcmd:recipient',
    Dbms_AQ.Namespace_AQ,
    'plsql://system.Replicate2MySQL.PostRunSQL?PR=1',
    HEXTORAW('FF')
  );
  lstReg := sys.AQ$_reg_info_list(regCallback);
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
create or replace function zabbix.ins_slideshows (slideshowid in number,name in varchar2,delay in number) return varchar2
is
begin
return
'insert into slideshows (slideshowid,name,delay) values ('
||Nvl(To_Char(slideshowid),'null')||','
||''''||Replace(name,'''','''''')||''''||','
||Nvl(To_Char(delay),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_slideshows after insert on zabbix.slideshows for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_slideshows (:new.slideshowid,:new.name,:new.delay)
);
end;
/
show errors
create or replace trigger zabbix.trau_slideshows after update on zabbix.slideshows for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.slideshows set '
||' slideshowid='||Nvl(To_Char(:new.slideshowid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||''''||','
||' delay='||Nvl(To_Char(:new.delay),'null') || ' where  slideshowid='||:old.slideshowid );
end;
/
show errors
create or replace trigger zabbix.trad_slideshows after delete on zabbix.slideshows for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.slideshows '
 || ' where  slideshowid='||:old.slideshowid 
);
end;
/
show errors
create or replace function zabbix.ins_slides (slideid in number,slideshowid in number,screenid in number,step in number,delay in number) return varchar2
is
begin
return
'insert into slides (slideid,slideshowid,screenid,step,delay) values ('
||Nvl(To_Char(slideid),'null')||','
||Nvl(To_Char(slideshowid),'null')||','
||Nvl(To_Char(screenid),'null')||','
||Nvl(To_Char(step),'null')||','
||Nvl(To_Char(delay),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_slides after insert on zabbix.slides for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_slides (:new.slideid,:new.slideshowid,:new.screenid,:new.step,:new.delay)
);
end;
/
show errors
create or replace trigger zabbix.trau_slides after update on zabbix.slides for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.slides set '
||' slideid='||Nvl(To_Char(:new.slideid),'null')||','
||' slideshowid='||Nvl(To_Char(:new.slideshowid),'null')||','
||' screenid='||Nvl(To_Char(:new.screenid),'null')||','
||' step='||Nvl(To_Char(:new.step),'null')||','
||' delay='||Nvl(To_Char(:new.delay),'null') || ' where  slideid='||:old.slideid );
end;
/
show errors
create or replace trigger zabbix.trad_slides after delete on zabbix.slides for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.slides '
 || ' where  slideid='||:old.slideid 
);
end;
/
show errors
create or replace function zabbix.ins_drules (druleid in number,name in varchar2,iprange in varchar2,delay in number,nextcheck in number,status in number) return varchar2
is
begin
return
'insert into drules (druleid,name,iprange,delay,nextcheck,status) values ('
||Nvl(To_Char(druleid),'null')||','
||''''||Replace(name,'''','''''')||''''||','
||''''||Replace(iprange,'''','''''')||''''||','
||Nvl(To_Char(delay),'null')||','
||Nvl(To_Char(nextcheck),'null')||','
||Nvl(To_Char(status),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_drules after insert on zabbix.drules for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_drules (:new.druleid,:new.name,:new.iprange,:new.delay,:new.nextcheck,:new.status)
);
end;
/
show errors
create or replace trigger zabbix.trau_drules after update on zabbix.drules for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.drules set '
||' druleid='||Nvl(To_Char(:new.druleid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||''''||','
||' iprange='''||Replace(:new.iprange,'''','''''')||''''||','
||' delay='||Nvl(To_Char(:new.delay),'null')||','
||' nextcheck='||Nvl(To_Char(:new.nextcheck),'null')||','
||' status='||Nvl(To_Char(:new.status),'null') || ' where  druleid='||:old.druleid );
end;
/
show errors
create or replace trigger zabbix.trad_drules after delete on zabbix.drules for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.drules '
 || ' where  druleid='||:old.druleid 
);
end;
/
show errors
create or replace function zabbix.ins_dchecks (dcheckid in number,druleid in number,type in number,key_ in varchar2,snmp_community in varchar2,ports in varchar2) return varchar2
is
begin
return
'insert into dchecks (dcheckid,druleid,type,key_,snmp_community,ports) values ('
||Nvl(To_Char(dcheckid),'null')||','
||Nvl(To_Char(druleid),'null')||','
||Nvl(To_Char(type),'null')||','
||''''||Replace(key_,'''','''''')||''''||','
||''''||Replace(snmp_community,'''','''''')||''''||','
||''''||Replace(ports,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_dchecks after insert on zabbix.dchecks for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_dchecks (:new.dcheckid,:new.druleid,:new.type,:new.key_,:new.snmp_community,:new.ports)
);
end;
/
show errors
create or replace trigger zabbix.trau_dchecks after update on zabbix.dchecks for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.dchecks set '
||' dcheckid='||Nvl(To_Char(:new.dcheckid),'null')||','
||' druleid='||Nvl(To_Char(:new.druleid),'null')||','
||' type='||Nvl(To_Char(:new.type),'null')||','
||' key_='''||Replace(:new.key_,'''','''''')||''''||','
||' snmp_community='''||Replace(:new.snmp_community,'''','''''')||''''||','
||' ports='''||Replace(:new.ports,'''','''''')||'''' || ' where  dcheckid='||:old.dcheckid );
end;
/
show errors
create or replace trigger zabbix.trad_dchecks after delete on zabbix.dchecks for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.dchecks '
 || ' where  dcheckid='||:old.dcheckid 
);
end;
/
show errors
create or replace function zabbix.ins_dhosts (dhostid in number,druleid in number,ip in varchar2,status in number,lastup in number,lastdown in number) return varchar2
is
begin
return
'insert into dhosts (dhostid,druleid,ip,status,lastup,lastdown) values ('
||Nvl(To_Char(dhostid),'null')||','
||Nvl(To_Char(druleid),'null')||','
||''''||Replace(ip,'''','''''')||''''||','
||Nvl(To_Char(status),'null')||','
||Nvl(To_Char(lastup),'null')||','
||Nvl(To_Char(lastdown),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_dhosts after insert on zabbix.dhosts for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_dhosts (:new.dhostid,:new.druleid,:new.ip,:new.status,:new.lastup,:new.lastdown)
);
end;
/
show errors
create or replace trigger zabbix.trau_dhosts after update on zabbix.dhosts for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.dhosts set '
||' dhostid='||Nvl(To_Char(:new.dhostid),'null')||','
||' druleid='||Nvl(To_Char(:new.druleid),'null')||','
||' ip='''||Replace(:new.ip,'''','''''')||''''||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' lastup='||Nvl(To_Char(:new.lastup),'null')||','
||' lastdown='||Nvl(To_Char(:new.lastdown),'null') || ' where  dhostid='||:old.dhostid );
end;
/
show errors
create or replace trigger zabbix.trad_dhosts after delete on zabbix.dhosts for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.dhosts '
 || ' where  dhostid='||:old.dhostid 
);
end;
/
show errors
create or replace function zabbix.ins_dservices (dserviceid in number,dhostid in number,type in number,key_ in varchar2,value in varchar2,port in number,status in number,lastup in number,lastdown in number) return varchar2
is
begin
return
'insert into dservices (dserviceid,dhostid,type,key_,value,port,status,lastup,lastdown) values ('
||Nvl(To_Char(dserviceid),'null')||','
||Nvl(To_Char(dhostid),'null')||','
||Nvl(To_Char(type),'null')||','
||''''||Replace(key_,'''','''''')||''''||','
||''''||Replace(value,'''','''''')||''''||','
||Nvl(To_Char(port),'null')||','
||Nvl(To_Char(status),'null')||','
||Nvl(To_Char(lastup),'null')||','
||Nvl(To_Char(lastdown),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_dservices after insert on zabbix.dservices for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_dservices (:new.dserviceid,:new.dhostid,:new.type,:new.key_,:new.value,:new.port,:new.status,:new.lastup,:new.lastdown)
);
end;
/
show errors
create or replace trigger zabbix.trau_dservices after update on zabbix.dservices for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.dservices set '
||' dserviceid='||Nvl(To_Char(:new.dserviceid),'null')||','
||' dhostid='||Nvl(To_Char(:new.dhostid),'null')||','
||' type='||Nvl(To_Char(:new.type),'null')||','
||' key_='''||Replace(:new.key_,'''','''''')||''''||','
||' value='''||Replace(:new.value,'''','''''')||''''||','
||' port='||Nvl(To_Char(:new.port),'null')||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' lastup='||Nvl(To_Char(:new.lastup),'null')||','
||' lastdown='||Nvl(To_Char(:new.lastdown),'null') || ' where  dserviceid='||:old.dserviceid );
end;
/
show errors
create or replace trigger zabbix.trad_dservices after delete on zabbix.dservices for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.dservices '
 || ' where  dserviceid='||:old.dserviceid 
);
end;
/
show errors
create or replace function zabbix.ins_ids (nodeid in number,table_name in varchar2,field_name in varchar2,nextid in number) return varchar2
is
begin
return
'insert into ids (nodeid,table_name,field_name,nextid) values ('
||Nvl(To_Char(nodeid),'null')||','
||''''||Replace(table_name,'''','''''')||''''||','
||''''||Replace(field_name,'''','''''')||''''||','
||Nvl(To_Char(nextid),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_ids after insert on zabbix.ids for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_ids (:new.nodeid,:new.table_name,:new.field_name,:new.nextid)
);
end;
/
show errors
create or replace trigger zabbix.trau_ids after update on zabbix.ids for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.ids set '
||' nodeid='||Nvl(To_Char(:new.nodeid),'null')||','
||' table_name='''||Replace(:new.table_name,'''','''''')||''''||','
||' field_name='''||Replace(:new.field_name,'''','''''')||''''||','
||' nextid='||Nvl(To_Char(:new.nextid),'null') || ' where  nodeid='||:old.nodeid || ' and  table_name='''||:old.table_name||'''' || ' and  field_name='''||:old.field_name||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_ids after delete on zabbix.ids for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.ids '
 || ' where  nodeid='||:old.nodeid || ' and  table_name='''||:old.table_name||'''' || ' and  field_name='''||:old.field_name||'''' 
);
end;
/
show errors
create or replace function zabbix.ins_httptest (httptestid in number,name in varchar2,applicationid in number,lastcheck in number,nextcheck in number,curstate in number,curstep in number,lastfailedstep in number,delay in number,status in number,macros in varchar2,agent in varchar2,time in number,error in varchar2) return varchar2
is
begin
return
'insert into httptest (httptestid,name,applicationid,lastcheck,nextcheck,curstate,curstep,lastfailedstep,delay,status,macros,agent,time,error) values ('
||Nvl(To_Char(httptestid),'null')||','
||''''||Replace(name,'''','''''')||''''||','
||Nvl(To_Char(applicationid),'null')||','
||Nvl(To_Char(lastcheck),'null')||','
||Nvl(To_Char(nextcheck),'null')||','
||Nvl(To_Char(curstate),'null')||','
||Nvl(To_Char(curstep),'null')||','
||Nvl(To_Char(lastfailedstep),'null')||','
||Nvl(To_Char(delay),'null')||','
||Nvl(To_Char(status),'null')||','
||''''||Replace(macros,'''','''''')||''''||','
||''''||Replace(agent,'''','''''')||''''||','
||Nvl(To_Char(time),'null')||','
||''''||Replace(error,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_httptest after insert on zabbix.httptest for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_httptest (:new.httptestid,:new.name,:new.applicationid,:new.lastcheck,:new.nextcheck,:new.curstate,:new.curstep,:new.lastfailedstep,:new.delay,:new.status,:new.macros,:new.agent,:new.time,:new.error)
);
end;
/
show errors
create or replace trigger zabbix.trau_httptest after update on zabbix.httptest for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.httptest set '
||' httptestid='||Nvl(To_Char(:new.httptestid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||''''||','
||' applicationid='||Nvl(To_Char(:new.applicationid),'null')||','
||' lastcheck='||Nvl(To_Char(:new.lastcheck),'null')||','
||' nextcheck='||Nvl(To_Char(:new.nextcheck),'null')||','
||' curstate='||Nvl(To_Char(:new.curstate),'null')||','
||' curstep='||Nvl(To_Char(:new.curstep),'null')||','
||' lastfailedstep='||Nvl(To_Char(:new.lastfailedstep),'null')||','
||' delay='||Nvl(To_Char(:new.delay),'null')||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' macros='''||Replace(:new.macros,'''','''''')||''''||','
||' agent='''||Replace(:new.agent,'''','''''')||''''||','
||' time='||Nvl(To_Char(:new.time),'null')||','
||' error='''||Replace(:new.error,'''','''''')||'''' || ' where  httptestid='||:old.httptestid );
end;
/
show errors
create or replace trigger zabbix.trad_httptest after delete on zabbix.httptest for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.httptest '
 || ' where  httptestid='||:old.httptestid 
);
end;
/
show errors
create or replace function zabbix.ins_httpstep (httpstepid in number,httptestid in number,name in varchar2,no in number,url in varchar2,timeout in number,posts in varchar2,required in varchar2,status_codes in varchar2) return varchar2
is
begin
return
'insert into httpstep (httpstepid,httptestid,name,no,url,timeout,posts,required,status_codes) values ('
||Nvl(To_Char(httpstepid),'null')||','
||Nvl(To_Char(httptestid),'null')||','
||''''||Replace(name,'''','''''')||''''||','
||Nvl(To_Char(no),'null')||','
||''''||Replace(url,'''','''''')||''''||','
||Nvl(To_Char(timeout),'null')||','
||''''||Replace(posts,'''','''''')||''''||','
||''''||Replace(required,'''','''''')||''''||','
||''''||Replace(status_codes,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_httpstep after insert on zabbix.httpstep for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_httpstep (:new.httpstepid,:new.httptestid,:new.name,:new.no,:new.url,:new.timeout,:new.posts,:new.required,:new.status_codes)
);
end;
/
show errors
create or replace trigger zabbix.trau_httpstep after update on zabbix.httpstep for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.httpstep set '
||' httpstepid='||Nvl(To_Char(:new.httpstepid),'null')||','
||' httptestid='||Nvl(To_Char(:new.httptestid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||''''||','
||' no='||Nvl(To_Char(:new.no),'null')||','
||' url='''||Replace(:new.url,'''','''''')||''''||','
||' timeout='||Nvl(To_Char(:new.timeout),'null')||','
||' posts='''||Replace(:new.posts,'''','''''')||''''||','
||' required='''||Replace(:new.required,'''','''''')||''''||','
||' status_codes='''||Replace(:new.status_codes,'''','''''')||'''' || ' where  httpstepid='||:old.httpstepid );
end;
/
show errors
create or replace trigger zabbix.trad_httpstep after delete on zabbix.httpstep for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.httpstep '
 || ' where  httpstepid='||:old.httpstepid 
);
end;
/
show errors
create or replace function zabbix.ins_httpstepitem (httpstepitemid in number,httpstepid in number,itemid in number,type in number) return varchar2
is
begin
return
'insert into httpstepitem (httpstepitemid,httpstepid,itemid,type) values ('
||Nvl(To_Char(httpstepitemid),'null')||','
||Nvl(To_Char(httpstepid),'null')||','
||Nvl(To_Char(itemid),'null')||','
||Nvl(To_Char(type),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_httpstepitem after insert on zabbix.httpstepitem for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_httpstepitem (:new.httpstepitemid,:new.httpstepid,:new.itemid,:new.type)
);
end;
/
show errors
create or replace trigger zabbix.trau_httpstepitem after update on zabbix.httpstepitem for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.httpstepitem set '
||' httpstepitemid='||Nvl(To_Char(:new.httpstepitemid),'null')||','
||' httpstepid='||Nvl(To_Char(:new.httpstepid),'null')||','
||' itemid='||Nvl(To_Char(:new.itemid),'null')||','
||' type='||Nvl(To_Char(:new.type),'null') || ' where  httpstepitemid='||:old.httpstepitemid );
end;
/
show errors
create or replace trigger zabbix.trad_httpstepitem after delete on zabbix.httpstepitem for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.httpstepitem '
 || ' where  httpstepitemid='||:old.httpstepitemid 
);
end;
/
show errors
create or replace function zabbix.ins_httptestitem (httptestitemid in number,httptestid in number,itemid in number,type in number) return varchar2
is
begin
return
'insert into httptestitem (httptestitemid,httptestid,itemid,type) values ('
||Nvl(To_Char(httptestitemid),'null')||','
||Nvl(To_Char(httptestid),'null')||','
||Nvl(To_Char(itemid),'null')||','
||Nvl(To_Char(type),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_httptestitem after insert on zabbix.httptestitem for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_httptestitem (:new.httptestitemid,:new.httptestid,:new.itemid,:new.type)
);
end;
/
show errors
create or replace trigger zabbix.trau_httptestitem after update on zabbix.httptestitem for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.httptestitem set '
||' httptestitemid='||Nvl(To_Char(:new.httptestitemid),'null')||','
||' httptestid='||Nvl(To_Char(:new.httptestid),'null')||','
||' itemid='||Nvl(To_Char(:new.itemid),'null')||','
||' type='||Nvl(To_Char(:new.type),'null') || ' where  httptestitemid='||:old.httptestitemid );
end;
/
show errors
create or replace trigger zabbix.trad_httptestitem after delete on zabbix.httptestitem for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.httptestitem '
 || ' where  httptestitemid='||:old.httptestitemid 
);
end;
/
show errors
create or replace function zabbix.ins_nodes (nodeid in number,name in varchar2,timezone in number,ip in varchar2,port in number,slave_history in number,slave_trends in number,event_lastid in number,history_lastid in number,history_str_lastid in number,history_uint_lastid in number,nodetype in number,masterid in number) return varchar2
is
begin
return
'insert into nodes (nodeid,name,timezone,ip,port,slave_history,slave_trends,event_lastid,history_lastid,history_str_lastid,history_uint_lastid,nodetype,masterid) values ('
||Nvl(To_Char(nodeid),'null')||','
||''''||Replace(name,'''','''''')||''''||','
||Nvl(To_Char(timezone),'null')||','
||''''||Replace(ip,'''','''''')||''''||','
||Nvl(To_Char(port),'null')||','
||Nvl(To_Char(slave_history),'null')||','
||Nvl(To_Char(slave_trends),'null')||','
||Nvl(To_Char(event_lastid),'null')||','
||Nvl(To_Char(history_lastid),'null')||','
||Nvl(To_Char(history_str_lastid),'null')||','
||Nvl(To_Char(history_uint_lastid),'null')||','
||Nvl(To_Char(nodetype),'null')||','
||Nvl(To_Char(masterid),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_nodes after insert on zabbix.nodes for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_nodes (:new.nodeid,:new.name,:new.timezone,:new.ip,:new.port,:new.slave_history,:new.slave_trends,:new.event_lastid,:new.history_lastid,:new.history_str_lastid,:new.history_uint_lastid,:new.nodetype,:new.masterid)
);
end;
/
show errors
create or replace trigger zabbix.trau_nodes after update on zabbix.nodes for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.nodes set '
||' nodeid='||Nvl(To_Char(:new.nodeid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||''''||','
||' timezone='||Nvl(To_Char(:new.timezone),'null')||','
||' ip='''||Replace(:new.ip,'''','''''')||''''||','
||' port='||Nvl(To_Char(:new.port),'null')||','
||' slave_history='||Nvl(To_Char(:new.slave_history),'null')||','
||' slave_trends='||Nvl(To_Char(:new.slave_trends),'null')||','
||' event_lastid='||Nvl(To_Char(:new.event_lastid),'null')||','
||' history_lastid='||Nvl(To_Char(:new.history_lastid),'null')||','
||' history_str_lastid='||Nvl(To_Char(:new.history_str_lastid),'null')||','
||' history_uint_lastid='||Nvl(To_Char(:new.history_uint_lastid),'null')||','
||' nodetype='||Nvl(To_Char(:new.nodetype),'null')||','
||' masterid='||Nvl(To_Char(:new.masterid),'null') || ' where  nodeid='||:old.nodeid );
end;
/
show errors
create or replace trigger zabbix.trad_nodes after delete on zabbix.nodes for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.nodes '
 || ' where  nodeid='||:old.nodeid 
);
end;
/
show errors
create or replace function zabbix.ins_node_cksum (cksumid in number,nodeid in number,tablename in varchar2,fieldname in varchar2,recordid in number,cksumtype in number,cksum in varchar2) return varchar2
is
begin
return
'insert into node_cksum (cksumid,nodeid,tablename,fieldname,recordid,cksumtype,cksum) values ('
||Nvl(To_Char(cksumid),'null')||','
||Nvl(To_Char(nodeid),'null')||','
||''''||Replace(tablename,'''','''''')||''''||','
||''''||Replace(fieldname,'''','''''')||''''||','
||Nvl(To_Char(recordid),'null')||','
||Nvl(To_Char(cksumtype),'null')||','
||''''||Replace(cksum,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_node_cksum after insert on zabbix.node_cksum for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_node_cksum (:new.cksumid,:new.nodeid,:new.tablename,:new.fieldname,:new.recordid,:new.cksumtype,:new.cksum)
);
end;
/
show errors
create or replace trigger zabbix.trau_node_cksum after update on zabbix.node_cksum for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.node_cksum set '
||' cksumid='||Nvl(To_Char(:new.cksumid),'null')||','
||' nodeid='||Nvl(To_Char(:new.nodeid),'null')||','
||' tablename='''||Replace(:new.tablename,'''','''''')||''''||','
||' fieldname='''||Replace(:new.fieldname,'''','''''')||''''||','
||' recordid='||Nvl(To_Char(:new.recordid),'null')||','
||' cksumtype='||Nvl(To_Char(:new.cksumtype),'null')||','
||' cksum='''||Replace(:new.cksum,'''','''''')||'''' || ' where  cksumid='||:old.cksumid );
end;
/
show errors
create or replace trigger zabbix.trad_node_cksum after delete on zabbix.node_cksum for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.node_cksum '
 || ' where  cksumid='||:old.cksumid 
);
end;
/
show errors
create or replace function zabbix.ins_node_configlog (conflogid in number,nodeid in number,tablename in varchar2,recordid in number,operation in number,sync_master in number,sync_slave in number) return varchar2
is
begin
return
'insert into node_configlog (conflogid,nodeid,tablename,recordid,operation,sync_master,sync_slave) values ('
||Nvl(To_Char(conflogid),'null')||','
||Nvl(To_Char(nodeid),'null')||','
||''''||Replace(tablename,'''','''''')||''''||','
||Nvl(To_Char(recordid),'null')||','
||Nvl(To_Char(operation),'null')||','
||Nvl(To_Char(sync_master),'null')||','
||Nvl(To_Char(sync_slave),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_node_configlog after insert on zabbix.node_configlog for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_node_configlog (:new.conflogid,:new.nodeid,:new.tablename,:new.recordid,:new.operation,:new.sync_master,:new.sync_slave)
);
end;
/
show errors
create or replace trigger zabbix.trau_node_configlog after update on zabbix.node_configlog for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.node_configlog set '
||' conflogid='||Nvl(To_Char(:new.conflogid),'null')||','
||' nodeid='||Nvl(To_Char(:new.nodeid),'null')||','
||' tablename='''||Replace(:new.tablename,'''','''''')||''''||','
||' recordid='||Nvl(To_Char(:new.recordid),'null')||','
||' operation='||Nvl(To_Char(:new.operation),'null')||','
||' sync_master='||Nvl(To_Char(:new.sync_master),'null')||','
||' sync_slave='||Nvl(To_Char(:new.sync_slave),'null') || ' where  nodeid='||:old.nodeid || ' and  conflogid='||:old.conflogid );
end;
/
show errors
create or replace trigger zabbix.trad_node_configlog after delete on zabbix.node_configlog for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.node_configlog '
 || ' where  nodeid='||:old.nodeid || ' and  conflogid='||:old.conflogid 
);
end;
/
show errors
create or replace function zabbix.ins_acknowledges (acknowledgeid in number,userid in number,eventid in number,clock in number,message in varchar2) return varchar2
is
begin
return
'insert into acknowledges (acknowledgeid,userid,eventid,clock,message) values ('
||Nvl(To_Char(acknowledgeid),'null')||','
||Nvl(To_Char(userid),'null')||','
||Nvl(To_Char(eventid),'null')||','
||Nvl(To_Char(clock),'null')||','
||''''||Replace(message,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_acknowledges after insert on zabbix.acknowledges for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_acknowledges (:new.acknowledgeid,:new.userid,:new.eventid,:new.clock,:new.message)
);
end;
/
show errors
create or replace trigger zabbix.trau_acknowledges after update on zabbix.acknowledges for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.acknowledges set '
||' acknowledgeid='||Nvl(To_Char(:new.acknowledgeid),'null')||','
||' userid='||Nvl(To_Char(:new.userid),'null')||','
||' eventid='||Nvl(To_Char(:new.eventid),'null')||','
||' clock='||Nvl(To_Char(:new.clock),'null')||','
||' message='''||Replace(:new.message,'''','''''')||'''' || ' where  acknowledgeid='||:old.acknowledgeid );
end;
/
show errors
create or replace trigger zabbix.trad_acknowledges after delete on zabbix.acknowledges for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.acknowledges '
 || ' where  acknowledgeid='||:old.acknowledgeid 
);
end;
/
show errors
create or replace function zabbix.ins_actions (actionid in number,name in varchar2,eventsource in number,evaltype in number,status in number) return varchar2
is
begin
return
'insert into actions (actionid,name,eventsource,evaltype,status) values ('
||Nvl(To_Char(actionid),'null')||','
||''''||Replace(name,'''','''''')||''''||','
||Nvl(To_Char(eventsource),'null')||','
||Nvl(To_Char(evaltype),'null')||','
||Nvl(To_Char(status),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_actions after insert on zabbix.actions for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_actions (:new.actionid,:new.name,:new.eventsource,:new.evaltype,:new.status)
);
end;
/
show errors
create or replace trigger zabbix.trau_actions after update on zabbix.actions for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.actions set '
||' actionid='||Nvl(To_Char(:new.actionid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||''''||','
||' eventsource='||Nvl(To_Char(:new.eventsource),'null')||','
||' evaltype='||Nvl(To_Char(:new.evaltype),'null')||','
||' status='||Nvl(To_Char(:new.status),'null') || ' where  actionid='||:old.actionid );
end;
/
show errors
create or replace trigger zabbix.trad_actions after delete on zabbix.actions for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.actions '
 || ' where  actionid='||:old.actionid 
);
end;
/
show errors
create or replace function zabbix.ins_operations (operationid in number,actionid in number,operationtype in number,object in number,objectid in number,shortdata in varchar2,longdata in varchar2) return varchar2
is
begin
return
'insert into operations (operationid,actionid,operationtype,object,objectid,shortdata,longdata) values ('
||Nvl(To_Char(operationid),'null')||','
||Nvl(To_Char(actionid),'null')||','
||Nvl(To_Char(operationtype),'null')||','
||Nvl(To_Char(object),'null')||','
||Nvl(To_Char(objectid),'null')||','
||''''||Replace(shortdata,'''','''''')||''''||','
||''''||Replace(longdata,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_operations after insert on zabbix.operations for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_operations (:new.operationid,:new.actionid,:new.operationtype,:new.object,:new.objectid,:new.shortdata,:new.longdata)
);
end;
/
show errors
create or replace trigger zabbix.trau_operations after update on zabbix.operations for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.operations set '
||' operationid='||Nvl(To_Char(:new.operationid),'null')||','
||' actionid='||Nvl(To_Char(:new.actionid),'null')||','
||' operationtype='||Nvl(To_Char(:new.operationtype),'null')||','
||' object='||Nvl(To_Char(:new.object),'null')||','
||' objectid='||Nvl(To_Char(:new.objectid),'null')||','
||' shortdata='''||Replace(:new.shortdata,'''','''''')||''''||','
||' longdata='''||Replace(:new.longdata,'''','''''')||'''' || ' where  operationid='||:old.operationid );
end;
/
show errors
create or replace trigger zabbix.trad_operations after delete on zabbix.operations for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.operations '
 || ' where  operationid='||:old.operationid 
);
end;
/
show errors
create or replace function zabbix.ins_applications (applicationid in number,hostid in number,name in varchar2,templateid in number) return varchar2
is
begin
return
'insert into applications (applicationid,hostid,name,templateid) values ('
||Nvl(To_Char(applicationid),'null')||','
||Nvl(To_Char(hostid),'null')||','
||''''||Replace(name,'''','''''')||''''||','
||Nvl(To_Char(templateid),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_applications after insert on zabbix.applications for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_applications (:new.applicationid,:new.hostid,:new.name,:new.templateid)
);
end;
/
show errors
create or replace trigger zabbix.trau_applications after update on zabbix.applications for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.applications set '
||' applicationid='||Nvl(To_Char(:new.applicationid),'null')||','
||' hostid='||Nvl(To_Char(:new.hostid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||''''||','
||' templateid='||Nvl(To_Char(:new.templateid),'null') || ' where  applicationid='||:old.applicationid );
end;
/
show errors
create or replace trigger zabbix.trad_applications after delete on zabbix.applications for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.applications '
 || ' where  applicationid='||:old.applicationid 
);
end;
/
show errors
create or replace function zabbix.ins_auditlog (auditid in number,userid in number,clock in number,action in number,resourcetype in number,details in varchar2) return varchar2
is
begin
return
'insert into auditlog (auditid,userid,clock,action,resourcetype,details) values ('
||Nvl(To_Char(auditid),'null')||','
||Nvl(To_Char(userid),'null')||','
||Nvl(To_Char(clock),'null')||','
||Nvl(To_Char(action),'null')||','
||Nvl(To_Char(resourcetype),'null')||','
||''''||Replace(details,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_auditlog after insert on zabbix.auditlog for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_auditlog (:new.auditid,:new.userid,:new.clock,:new.action,:new.resourcetype,:new.details)
);
end;
/
show errors
create or replace trigger zabbix.trau_auditlog after update on zabbix.auditlog for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.auditlog set '
||' auditid='||Nvl(To_Char(:new.auditid),'null')||','
||' userid='||Nvl(To_Char(:new.userid),'null')||','
||' clock='||Nvl(To_Char(:new.clock),'null')||','
||' action='||Nvl(To_Char(:new.action),'null')||','
||' resourcetype='||Nvl(To_Char(:new.resourcetype),'null')||','
||' details='''||Replace(:new.details,'''','''''')||'''' || ' where  auditid='||:old.auditid );
end;
/
show errors
create or replace trigger zabbix.trad_auditlog after delete on zabbix.auditlog for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.auditlog '
 || ' where  auditid='||:old.auditid 
);
end;
/
show errors
create or replace function zabbix.ins_conditions (conditionid in number,actionid in number,conditiontype in number,operator in number,value in varchar2) return varchar2
is
begin
return
'insert into conditions (conditionid,actionid,conditiontype,operator,value) values ('
||Nvl(To_Char(conditionid),'null')||','
||Nvl(To_Char(actionid),'null')||','
||Nvl(To_Char(conditiontype),'null')||','
||Nvl(To_Char(operator),'null')||','
||''''||Replace(value,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_conditions after insert on zabbix.conditions for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_conditions (:new.conditionid,:new.actionid,:new.conditiontype,:new.operator,:new.value)
);
end;
/
show errors
create or replace trigger zabbix.trau_conditions after update on zabbix.conditions for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.conditions set '
||' conditionid='||Nvl(To_Char(:new.conditionid),'null')||','
||' actionid='||Nvl(To_Char(:new.actionid),'null')||','
||' conditiontype='||Nvl(To_Char(:new.conditiontype),'null')||','
||' operator='||Nvl(To_Char(:new.operator),'null')||','
||' value='''||Replace(:new.value,'''','''''')||'''' || ' where  conditionid='||:old.conditionid );
end;
/
show errors
create or replace trigger zabbix.trad_conditions after delete on zabbix.conditions for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.conditions '
 || ' where  conditionid='||:old.conditionid 
);
end;
/
show errors
create or replace function zabbix.ins_config (configid in number,alert_history in number,event_history in number,refresh_unsupported in number,work_period in varchar2,alert_usrgrpid in number) return varchar2
is
begin
return
'insert into config (configid,alert_history,event_history,refresh_unsupported,work_period,alert_usrgrpid) values ('
||Nvl(To_Char(configid),'null')||','
||Nvl(To_Char(alert_history),'null')||','
||Nvl(To_Char(event_history),'null')||','
||Nvl(To_Char(refresh_unsupported),'null')||','
||''''||Replace(work_period,'''','''''')||''''||','
||Nvl(To_Char(alert_usrgrpid),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_config after insert on zabbix.config for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_config (:new.configid,:new.alert_history,:new.event_history,:new.refresh_unsupported,:new.work_period,:new.alert_usrgrpid)
);
end;
/
show errors
create or replace trigger zabbix.trau_config after update on zabbix.config for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.config set '
||' configid='||Nvl(To_Char(:new.configid),'null')||','
||' alert_history='||Nvl(To_Char(:new.alert_history),'null')||','
||' event_history='||Nvl(To_Char(:new.event_history),'null')||','
||' refresh_unsupported='||Nvl(To_Char(:new.refresh_unsupported),'null')||','
||' work_period='''||Replace(:new.work_period,'''','''''')||''''||','
||' alert_usrgrpid='||Nvl(To_Char(:new.alert_usrgrpid),'null') || ' where  configid='||:old.configid );
end;
/
show errors
create or replace trigger zabbix.trad_config after delete on zabbix.config for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.config '
 || ' where  configid='||:old.configid 
);
end;
/
show errors
create or replace function zabbix.ins_functions (functionid in number,itemid in number,triggerid in number,lastvalue in varchar2,function in varchar2,parameter in varchar2) return varchar2
is
begin
return
'insert into functions (functionid,itemid,triggerid,lastvalue,function,parameter) values ('
||Nvl(To_Char(functionid),'null')||','
||Nvl(To_Char(itemid),'null')||','
||Nvl(To_Char(triggerid),'null')||','
||''''||Replace(lastvalue,'''','''''')||''''||','
||''''||Replace(function,'''','''''')||''''||','
||''''||Replace(parameter,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_functions after insert on zabbix.functions for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_functions (:new.functionid,:new.itemid,:new.triggerid,:new.lastvalue,:new.function,:new.parameter)
);
end;
/
show errors
create or replace trigger zabbix.trau_functions after update on zabbix.functions for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.functions set '
||' functionid='||Nvl(To_Char(:new.functionid),'null')||','
||' itemid='||Nvl(To_Char(:new.itemid),'null')||','
||' triggerid='||Nvl(To_Char(:new.triggerid),'null')||','
||' lastvalue='''||Replace(:new.lastvalue,'''','''''')||''''||','
||' function='''||Replace(:new.function,'''','''''')||''''||','
||' parameter='''||Replace(:new.parameter,'''','''''')||'''' || ' where  functionid='||:old.functionid );
end;
/
show errors
create or replace trigger zabbix.trad_functions after delete on zabbix.functions for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.functions '
 || ' where  functionid='||:old.functionid 
);
end;
/
show errors
create or replace function zabbix.ins_graphs (graphid in number,name in varchar2,width in number,height in number,yaxistype in number,yaxismin in number,yaxismax in number,templateid in number,show_work_period in number,show_triggers in number,graphtype in number) return varchar2
is
begin
return
'insert into graphs (graphid,name,width,height,yaxistype,yaxismin,yaxismax,templateid,show_work_period,show_triggers,graphtype) values ('
||Nvl(To_Char(graphid),'null')||','
||''''||Replace(name,'''','''''')||''''||','
||Nvl(To_Char(width),'null')||','
||Nvl(To_Char(height),'null')||','
||Nvl(To_Char(yaxistype),'null')||','
||Nvl(To_Char(yaxismin),'null')||','
||Nvl(To_Char(yaxismax),'null')||','
||Nvl(To_Char(templateid),'null')||','
||Nvl(To_Char(show_work_period),'null')||','
||Nvl(To_Char(show_triggers),'null')||','
||Nvl(To_Char(graphtype),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_graphs after insert on zabbix.graphs for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_graphs (:new.graphid,:new.name,:new.width,:new.height,:new.yaxistype,:new.yaxismin,:new.yaxismax,:new.templateid,:new.show_work_period,:new.show_triggers,:new.graphtype)
);
end;
/
show errors
create or replace trigger zabbix.trau_graphs after update on zabbix.graphs for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.graphs set '
||' graphid='||Nvl(To_Char(:new.graphid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||''''||','
||' width='||Nvl(To_Char(:new.width),'null')||','
||' height='||Nvl(To_Char(:new.height),'null')||','
||' yaxistype='||Nvl(To_Char(:new.yaxistype),'null')||','
||' yaxismin='||Nvl(To_Char(:new.yaxismin),'null')||','
||' yaxismax='||Nvl(To_Char(:new.yaxismax),'null')||','
||' templateid='||Nvl(To_Char(:new.templateid),'null')||','
||' show_work_period='||Nvl(To_Char(:new.show_work_period),'null')||','
||' show_triggers='||Nvl(To_Char(:new.show_triggers),'null')||','
||' graphtype='||Nvl(To_Char(:new.graphtype),'null') || ' where  graphid='||:old.graphid );
end;
/
show errors
create or replace trigger zabbix.trad_graphs after delete on zabbix.graphs for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.graphs '
 || ' where  graphid='||:old.graphid 
);
end;
/
show errors
create or replace function zabbix.ins_graphs_items (gitemid in number,graphid in number,itemid in number,drawtype in number,sortorder in number,color in varchar2,yaxisside in number,calc_fnc in number,type in number,periods_cnt in number) return varchar2
is
begin
return
'insert into graphs_items (gitemid,graphid,itemid,drawtype,sortorder,color,yaxisside,calc_fnc,type,periods_cnt) values ('
||Nvl(To_Char(gitemid),'null')||','
||Nvl(To_Char(graphid),'null')||','
||Nvl(To_Char(itemid),'null')||','
||Nvl(To_Char(drawtype),'null')||','
||Nvl(To_Char(sortorder),'null')||','
||''''||Replace(color,'''','''''')||''''||','
||Nvl(To_Char(yaxisside),'null')||','
||Nvl(To_Char(calc_fnc),'null')||','
||Nvl(To_Char(type),'null')||','
||Nvl(To_Char(periods_cnt),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_graphs_items after insert on zabbix.graphs_items for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_graphs_items (:new.gitemid,:new.graphid,:new.itemid,:new.drawtype,:new.sortorder,:new.color,:new.yaxisside,:new.calc_fnc,:new.type,:new.periods_cnt)
);
end;
/
show errors
create or replace trigger zabbix.trau_graphs_items after update on zabbix.graphs_items for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.graphs_items set '
||' gitemid='||Nvl(To_Char(:new.gitemid),'null')||','
||' graphid='||Nvl(To_Char(:new.graphid),'null')||','
||' itemid='||Nvl(To_Char(:new.itemid),'null')||','
||' drawtype='||Nvl(To_Char(:new.drawtype),'null')||','
||' sortorder='||Nvl(To_Char(:new.sortorder),'null')||','
||' color='''||Replace(:new.color,'''','''''')||''''||','
||' yaxisside='||Nvl(To_Char(:new.yaxisside),'null')||','
||' calc_fnc='||Nvl(To_Char(:new.calc_fnc),'null')||','
||' type='||Nvl(To_Char(:new.type),'null')||','
||' periods_cnt='||Nvl(To_Char(:new.periods_cnt),'null') || ' where  gitemid='||:old.gitemid );
end;
/
show errors
create or replace trigger zabbix.trad_graphs_items after delete on zabbix.graphs_items for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.graphs_items '
 || ' where  gitemid='||:old.gitemid 
);
end;
/
show errors
create or replace function zabbix.ins_groups (groupid in number,name in varchar2) return varchar2
is
begin
return
'insert into groups (groupid,name) values ('
||Nvl(To_Char(groupid),'null')||','
||''''||Replace(name,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_groups after insert on zabbix.groups for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_groups (:new.groupid,:new.name)
);
end;
/
show errors
create or replace trigger zabbix.trau_groups after update on zabbix.groups for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.groups set '
||' groupid='||Nvl(To_Char(:new.groupid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||'''' || ' where  groupid='||:old.groupid );
end;
/
show errors
create or replace trigger zabbix.trad_groups after delete on zabbix.groups for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.groups '
 || ' where  groupid='||:old.groupid 
);
end;
/
show errors
create or replace function zabbix.ins_help_items (itemtype in number,key_ in varchar2,description in varchar2) return varchar2
is
begin
return
'insert into help_items (itemtype,key_,description) values ('
||Nvl(To_Char(itemtype),'null')||','
||''''||Replace(key_,'''','''''')||''''||','
||''''||Replace(description,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_help_items after insert on zabbix.help_items for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_help_items (:new.itemtype,:new.key_,:new.description)
);
end;
/
show errors
create or replace trigger zabbix.trau_help_items after update on zabbix.help_items for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.help_items set '
||' itemtype='||Nvl(To_Char(:new.itemtype),'null')||','
||' key_='''||Replace(:new.key_,'''','''''')||''''||','
||' description='''||Replace(:new.description,'''','''''')||'''' || ' where  itemtype='||:old.itemtype || ' and  key_='''||:old.key_||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_help_items after delete on zabbix.help_items for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.help_items '
 || ' where  itemtype='||:old.itemtype || ' and  key_='''||:old.key_||'''' 
);
end;
/
show errors
create or replace function zabbix.ins_hosts (hostid in number,host in varchar2,dns in varchar2,useip in number,ip in varchar2,port in number,status in number,disable_until in number,error in varchar2,available in number,errors_from in number,siteid in number) return varchar2
is
begin
return
'insert into hosts (hostid,host,dns,useip,ip,port,status,disable_until,error,available,errors_from,siteid) values ('
||Nvl(To_Char(hostid),'null')||','
||''''||Replace(host,'''','''''')||''''||','
||''''||Replace(dns,'''','''''')||''''||','
||Nvl(To_Char(useip),'null')||','
||''''||Replace(ip,'''','''''')||''''||','
||Nvl(To_Char(port),'null')||','
||Nvl(To_Char(status),'null')||','
||Nvl(To_Char(disable_until),'null')||','
||''''||Replace(error,'''','''''')||''''||','
||Nvl(To_Char(available),'null')||','
||Nvl(To_Char(errors_from),'null')||','
||Nvl(To_Char(siteid),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_hosts after insert on zabbix.hosts for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_hosts (:new.hostid,:new.host,:new.dns,:new.useip,:new.ip,:new.port,:new.status,:new.disable_until,:new.error,:new.available,:new.errors_from,:new.siteid)
);
end;
/
show errors
create or replace trigger zabbix.trau_hosts after update on zabbix.hosts for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.hosts set '
||' hostid='||Nvl(To_Char(:new.hostid),'null')||','
||' host='''||Replace(:new.host,'''','''''')||''''||','
||' dns='''||Replace(:new.dns,'''','''''')||''''||','
||' useip='||Nvl(To_Char(:new.useip),'null')||','
||' ip='''||Replace(:new.ip,'''','''''')||''''||','
||' port='||Nvl(To_Char(:new.port),'null')||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' disable_until='||Nvl(To_Char(:new.disable_until),'null')||','
||' error='''||Replace(:new.error,'''','''''')||''''||','
||' available='||Nvl(To_Char(:new.available),'null')||','
||' errors_from='||Nvl(To_Char(:new.errors_from),'null')||','
||' siteid='||Nvl(To_Char(:new.siteid),'null') || ' where  hostid='||:old.hostid );
end;
/
show errors
create or replace trigger zabbix.trad_hosts after delete on zabbix.hosts for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.hosts '
 || ' where  hostid='||:old.hostid 
);
end;
/
show errors
create or replace function zabbix.ins_hosts_groups (hostgroupid in number,hostid in number,groupid in number) return varchar2
is
begin
return
'insert into hosts_groups (hostgroupid,hostid,groupid) values ('
||Nvl(To_Char(hostgroupid),'null')||','
||Nvl(To_Char(hostid),'null')||','
||Nvl(To_Char(groupid),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_hosts_groups after insert on zabbix.hosts_groups for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_hosts_groups (:new.hostgroupid,:new.hostid,:new.groupid)
);
end;
/
show errors
create or replace trigger zabbix.trau_hosts_groups after update on zabbix.hosts_groups for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.hosts_groups set '
||' hostgroupid='||Nvl(To_Char(:new.hostgroupid),'null')||','
||' hostid='||Nvl(To_Char(:new.hostid),'null')||','
||' groupid='||Nvl(To_Char(:new.groupid),'null') || ' where  hostgroupid='||:old.hostgroupid );
end;
/
show errors
create or replace trigger zabbix.trad_hosts_groups after delete on zabbix.hosts_groups for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.hosts_groups '
 || ' where  hostgroupid='||:old.hostgroupid 
);
end;
/
show errors
create or replace function zabbix.ins_hosts_profiles (hostid in number,devicetype in varchar2,name in varchar2,os in varchar2,serialno in varchar2,tag in varchar2,macaddress in varchar2,hardware in varchar2,software in varchar2,contact in varchar2,location in varchar2,notes in varchar2) return varchar2
is
begin
return
'insert into hosts_profiles (hostid,devicetype,name,os,serialno,tag,macaddress,hardware,software,contact,location,notes) values ('
||Nvl(To_Char(hostid),'null')||','
||''''||Replace(devicetype,'''','''''')||''''||','
||''''||Replace(name,'''','''''')||''''||','
||''''||Replace(os,'''','''''')||''''||','
||''''||Replace(serialno,'''','''''')||''''||','
||''''||Replace(tag,'''','''''')||''''||','
||''''||Replace(macaddress,'''','''''')||''''||','
||''''||Replace(hardware,'''','''''')||''''||','
||''''||Replace(software,'''','''''')||''''||','
||''''||Replace(contact,'''','''''')||''''||','
||''''||Replace(location,'''','''''')||''''||','
||''''||Replace(notes,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_hosts_profiles after insert on zabbix.hosts_profiles for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_hosts_profiles (:new.hostid,:new.devicetype,:new.name,:new.os,:new.serialno,:new.tag,:new.macaddress,:new.hardware,:new.software,:new.contact,:new.location,:new.notes)
);
end;
/
show errors
create or replace trigger zabbix.trau_hosts_profiles after update on zabbix.hosts_profiles for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.hosts_profiles set '
||' hostid='||Nvl(To_Char(:new.hostid),'null')||','
||' devicetype='''||Replace(:new.devicetype,'''','''''')||''''||','
||' name='''||Replace(:new.name,'''','''''')||''''||','
||' os='''||Replace(:new.os,'''','''''')||''''||','
||' serialno='''||Replace(:new.serialno,'''','''''')||''''||','
||' tag='''||Replace(:new.tag,'''','''''')||''''||','
||' macaddress='''||Replace(:new.macaddress,'''','''''')||''''||','
||' hardware='''||Replace(:new.hardware,'''','''''')||''''||','
||' software='''||Replace(:new.software,'''','''''')||''''||','
||' contact='''||Replace(:new.contact,'''','''''')||''''||','
||' location='''||Replace(:new.location,'''','''''')||''''||','
||' notes='''||Replace(:new.notes,'''','''''')||'''' || ' where  hostid='||:old.hostid );
end;
/
show errors
create or replace trigger zabbix.trad_hosts_profiles after delete on zabbix.hosts_profiles for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.hosts_profiles '
 || ' where  hostid='||:old.hostid 
);
end;
/
show errors
create or replace function zabbix.ins_hosts_templates (hosttemplateid in number,hostid in number,templateid in number) return varchar2
is
begin
return
'insert into hosts_templates (hosttemplateid,hostid,templateid) values ('
||Nvl(To_Char(hosttemplateid),'null')||','
||Nvl(To_Char(hostid),'null')||','
||Nvl(To_Char(templateid),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_hosts_templates after insert on zabbix.hosts_templates for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_hosts_templates (:new.hosttemplateid,:new.hostid,:new.templateid)
);
end;
/
show errors
create or replace trigger zabbix.trau_hosts_templates after update on zabbix.hosts_templates for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.hosts_templates set '
||' hosttemplateid='||Nvl(To_Char(:new.hosttemplateid),'null')||','
||' hostid='||Nvl(To_Char(:new.hostid),'null')||','
||' templateid='||Nvl(To_Char(:new.templateid),'null') || ' where  hosttemplateid='||:old.hosttemplateid );
end;
/
show errors
create or replace trigger zabbix.trad_hosts_templates after delete on zabbix.hosts_templates for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.hosts_templates '
 || ' where  hosttemplateid='||:old.hosttemplateid 
);
end;
/
show errors
create or replace function zabbix.ins_housekeeper (housekeeperid in number,tablename in varchar2,field in varchar2,value in number) return varchar2
is
begin
return
'insert into housekeeper (housekeeperid,tablename,field,value) values ('
||Nvl(To_Char(housekeeperid),'null')||','
||''''||Replace(tablename,'''','''''')||''''||','
||''''||Replace(field,'''','''''')||''''||','
||Nvl(To_Char(value),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_housekeeper after insert on zabbix.housekeeper for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_housekeeper (:new.housekeeperid,:new.tablename,:new.field,:new.value)
);
end;
/
show errors
create or replace trigger zabbix.trau_housekeeper after update on zabbix.housekeeper for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.housekeeper set '
||' housekeeperid='||Nvl(To_Char(:new.housekeeperid),'null')||','
||' tablename='''||Replace(:new.tablename,'''','''''')||''''||','
||' field='''||Replace(:new.field,'''','''''')||''''||','
||' value='||Nvl(To_Char(:new.value),'null') || ' where  housekeeperid='||:old.housekeeperid );
end;
/
show errors
create or replace trigger zabbix.trad_housekeeper after delete on zabbix.housekeeper for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.housekeeper '
 || ' where  housekeeperid='||:old.housekeeperid 
);
end;
/
show errors
create or replace function zabbix.ins_items (itemid in number,type in number,snmp_community in varchar2,snmp_oid in varchar2,snmp_port in number,hostid in number,description in varchar2,key_ in varchar2,delay in number,history in number,trends in number,nextcheck in number,lastvalue in varchar2,lastclock in number,prevvalue in varchar2,status in number,value_type in number,trapper_hosts in varchar2,units in varchar2,multiplier in number,delta in number,prevorgvalue in varchar2,snmpv3_securityname in varchar2,snmpv3_securitylevel in number,snmpv3_authpassphrase in varchar2,snmpv3_privpassphrase in varchar2,formula in varchar2,error in varchar2,lastlogsize in number,logtimefmt in varchar2,templateid in number,valuemapid in number,delay_flex in varchar2,params in varchar2,siteid in number,stderr in varchar2) return varchar2
is
begin
return
'insert into items (itemid,type,snmp_community,snmp_oid,snmp_port,hostid,description,key_,delay,history,trends,nextcheck,lastvalue,lastclock,prevvalue,status,value_type,trapper_hosts,units,multiplier,delta,prevorgvalue,snmpv3_securityname,snmpv3_securitylevel,snmpv3_authpassphrase,snmpv3_privpassphrase,formula,error,lastlogsize,logtimefmt,templateid,valuemapid,delay_flex,params,siteid,stderr) values ('
||Nvl(To_Char(itemid),'null')||','
||Nvl(To_Char(type),'null')||','
||''''||Replace(snmp_community,'''','''''')||''''||','
||''''||Replace(snmp_oid,'''','''''')||''''||','
||Nvl(To_Char(snmp_port),'null')||','
||Nvl(To_Char(hostid),'null')||','
||''''||Replace(description,'''','''''')||''''||','
||''''||Replace(key_,'''','''''')||''''||','
||Nvl(To_Char(delay),'null')||','
||Nvl(To_Char(history),'null')||','
||Nvl(To_Char(trends),'null')||','
||Nvl(To_Char(nextcheck),'null')||','
||''''||Replace(lastvalue,'''','''''')||''''||','
||Nvl(To_Char(lastclock),'null')||','
||''''||Replace(prevvalue,'''','''''')||''''||','
||Nvl(To_Char(status),'null')||','
||Nvl(To_Char(value_type),'null')||','
||''''||Replace(trapper_hosts,'''','''''')||''''||','
||''''||Replace(units,'''','''''')||''''||','
||Nvl(To_Char(multiplier),'null')||','
||Nvl(To_Char(delta),'null')||','
||''''||Replace(prevorgvalue,'''','''''')||''''||','
||''''||Replace(snmpv3_securityname,'''','''''')||''''||','
||Nvl(To_Char(snmpv3_securitylevel),'null')||','
||''''||Replace(snmpv3_authpassphrase,'''','''''')||''''||','
||''''||Replace(snmpv3_privpassphrase,'''','''''')||''''||','
||''''||Replace(formula,'''','''''')||''''||','
||''''||Replace(error,'''','''''')||''''||','
||Nvl(To_Char(lastlogsize),'null')||','
||''''||Replace(logtimefmt,'''','''''')||''''||','
||Nvl(To_Char(templateid),'null')||','
||Nvl(To_Char(valuemapid),'null')||','
||''''||Replace(delay_flex,'''','''''')||''''||','
||''''||Replace(params,'''','''''')||''''||','
||Nvl(To_Char(siteid),'null')||','
||''''||Replace(stderr,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_items after insert on zabbix.items for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_items (:new.itemid,:new.type,:new.snmp_community,:new.snmp_oid,:new.snmp_port,:new.hostid,:new.description,:new.key_,:new.delay,:new.history,:new.trends,:new.nextcheck,:new.lastvalue,:new.lastclock,:new.prevvalue,:new.status,:new.value_type,:new.trapper_hosts,:new.units,:new.multiplier,:new.delta,:new.prevorgvalue,:new.snmpv3_securityname,:new.snmpv3_securitylevel,:new.snmpv3_authpassphrase,:new.snmpv3_privpassphrase,:new.formula,:new.error,:new.lastlogsize,:new.logtimefmt,:new.templateid,:new.valuemapid,:new.delay_flex,:new.params,:new.siteid,:new.stderr)
);
end;
/
show errors
create or replace trigger zabbix.trau_items after update on zabbix.items for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.items set '
||' itemid='||Nvl(To_Char(:new.itemid),'null')||','
||' type='||Nvl(To_Char(:new.type),'null')||','
||' snmp_community='''||Replace(:new.snmp_community,'''','''''')||''''||','
||' snmp_oid='''||Replace(:new.snmp_oid,'''','''''')||''''||','
||' snmp_port='||Nvl(To_Char(:new.snmp_port),'null')||','
||' hostid='||Nvl(To_Char(:new.hostid),'null')||','
||' description='''||Replace(:new.description,'''','''''')||''''||','
||' key_='''||Replace(:new.key_,'''','''''')||''''||','
||' delay='||Nvl(To_Char(:new.delay),'null')||','
||' history='||Nvl(To_Char(:new.history),'null')||','
||' trends='||Nvl(To_Char(:new.trends),'null')||','
||' nextcheck='||Nvl(To_Char(:new.nextcheck),'null')||','
||' lastvalue='''||Replace(:new.lastvalue,'''','''''')||''''||','
||' lastclock='||Nvl(To_Char(:new.lastclock),'null')||','
||' prevvalue='''||Replace(:new.prevvalue,'''','''''')||''''||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' value_type='||Nvl(To_Char(:new.value_type),'null')||','
||' trapper_hosts='''||Replace(:new.trapper_hosts,'''','''''')||''''||','
||' units='''||Replace(:new.units,'''','''''')||''''||','
||' multiplier='||Nvl(To_Char(:new.multiplier),'null')||','
||' delta='||Nvl(To_Char(:new.delta),'null')||','
||' prevorgvalue='''||Replace(:new.prevorgvalue,'''','''''')||''''||','
||' snmpv3_securityname='''||Replace(:new.snmpv3_securityname,'''','''''')||''''||','
||' snmpv3_securitylevel='||Nvl(To_Char(:new.snmpv3_securitylevel),'null')||','
||' snmpv3_authpassphrase='''||Replace(:new.snmpv3_authpassphrase,'''','''''')||''''||','
||' snmpv3_privpassphrase='''||Replace(:new.snmpv3_privpassphrase,'''','''''')||''''||','
||' formula='''||Replace(:new.formula,'''','''''')||''''||','
||' error='''||Replace(:new.error,'''','''''')||''''||','
||' lastlogsize='||Nvl(To_Char(:new.lastlogsize),'null')||','
||' logtimefmt='''||Replace(:new.logtimefmt,'''','''''')||''''||','
||' templateid='||Nvl(To_Char(:new.templateid),'null')||','
||' valuemapid='||Nvl(To_Char(:new.valuemapid),'null')||','
||' delay_flex='''||Replace(:new.delay_flex,'''','''''')||''''||','
||' params='''||Replace(:new.params,'''','''''')||''''||','
||' siteid='||Nvl(To_Char(:new.siteid),'null')||','
||' stderr='''||Replace(:new.stderr,'''','''''')||'''' || ' where  itemid='||:old.itemid );
end;
/
show errors
create or replace trigger zabbix.trad_items after delete on zabbix.items for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.items '
 || ' where  itemid='||:old.itemid 
);
end;
/
show errors
create or replace function zabbix.ins_items_applications (itemappid in number,applicationid in number,itemid in number) return varchar2
is
begin
return
'insert into items_applications (itemappid,applicationid,itemid) values ('
||Nvl(To_Char(itemappid),'null')||','
||Nvl(To_Char(applicationid),'null')||','
||Nvl(To_Char(itemid),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_items_applications after insert on zabbix.items_applications for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_items_applications (:new.itemappid,:new.applicationid,:new.itemid)
);
end;
/
show errors
create or replace trigger zabbix.trau_items_applications after update on zabbix.items_applications for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.items_applications set '
||' itemappid='||Nvl(To_Char(:new.itemappid),'null')||','
||' applicationid='||Nvl(To_Char(:new.applicationid),'null')||','
||' itemid='||Nvl(To_Char(:new.itemid),'null') || ' where  itemappid='||:old.itemappid );
end;
/
show errors
create or replace trigger zabbix.trad_items_applications after delete on zabbix.items_applications for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.items_applications '
 || ' where  itemappid='||:old.itemappid 
);
end;
/
show errors
create or replace function zabbix.ins_mappings (mappingid in number,valuemapid in number,value in varchar2,newvalue in varchar2) return varchar2
is
begin
return
'insert into mappings (mappingid,valuemapid,value,newvalue) values ('
||Nvl(To_Char(mappingid),'null')||','
||Nvl(To_Char(valuemapid),'null')||','
||''''||Replace(value,'''','''''')||''''||','
||''''||Replace(newvalue,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_mappings after insert on zabbix.mappings for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_mappings (:new.mappingid,:new.valuemapid,:new.value,:new.newvalue)
);
end;
/
show errors
create or replace trigger zabbix.trau_mappings after update on zabbix.mappings for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.mappings set '
||' mappingid='||Nvl(To_Char(:new.mappingid),'null')||','
||' valuemapid='||Nvl(To_Char(:new.valuemapid),'null')||','
||' value='''||Replace(:new.value,'''','''''')||''''||','
||' newvalue='''||Replace(:new.newvalue,'''','''''')||'''' || ' where  mappingid='||:old.mappingid );
end;
/
show errors
create or replace trigger zabbix.trad_mappings after delete on zabbix.mappings for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.mappings '
 || ' where  mappingid='||:old.mappingid 
);
end;
/
show errors
create or replace function zabbix.ins_media (mediaid in number,userid in number,mediatypeid in number,sendto in varchar2,active in number,severity in number,period in varchar2) return varchar2
is
begin
return
'insert into media (mediaid,userid,mediatypeid,sendto,active,severity,period) values ('
||Nvl(To_Char(mediaid),'null')||','
||Nvl(To_Char(userid),'null')||','
||Nvl(To_Char(mediatypeid),'null')||','
||''''||Replace(sendto,'''','''''')||''''||','
||Nvl(To_Char(active),'null')||','
||Nvl(To_Char(severity),'null')||','
||''''||Replace(period,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_media after insert on zabbix.media for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_media (:new.mediaid,:new.userid,:new.mediatypeid,:new.sendto,:new.active,:new.severity,:new.period)
);
end;
/
show errors
create or replace trigger zabbix.trau_media after update on zabbix.media for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.media set '
||' mediaid='||Nvl(To_Char(:new.mediaid),'null')||','
||' userid='||Nvl(To_Char(:new.userid),'null')||','
||' mediatypeid='||Nvl(To_Char(:new.mediatypeid),'null')||','
||' sendto='''||Replace(:new.sendto,'''','''''')||''''||','
||' active='||Nvl(To_Char(:new.active),'null')||','
||' severity='||Nvl(To_Char(:new.severity),'null')||','
||' period='''||Replace(:new.period,'''','''''')||'''' || ' where  mediaid='||:old.mediaid );
end;
/
show errors
create or replace trigger zabbix.trad_media after delete on zabbix.media for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.media '
 || ' where  mediaid='||:old.mediaid 
);
end;
/
show errors
create or replace function zabbix.ins_media_type (mediatypeid in number,type in number,description in varchar2,smtp_server in varchar2,smtp_helo in varchar2,smtp_email in varchar2,exec_path in varchar2,gsm_modem in varchar2,username in varchar2,passwd in varchar2) return varchar2
is
begin
return
'insert into media_type (mediatypeid,type,description,smtp_server,smtp_helo,smtp_email,exec_path,gsm_modem,username,passwd) values ('
||Nvl(To_Char(mediatypeid),'null')||','
||Nvl(To_Char(type),'null')||','
||''''||Replace(description,'''','''''')||''''||','
||''''||Replace(smtp_server,'''','''''')||''''||','
||''''||Replace(smtp_helo,'''','''''')||''''||','
||''''||Replace(smtp_email,'''','''''')||''''||','
||''''||Replace(exec_path,'''','''''')||''''||','
||''''||Replace(gsm_modem,'''','''''')||''''||','
||''''||Replace(username,'''','''''')||''''||','
||''''||Replace(passwd,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_media_type after insert on zabbix.media_type for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_media_type (:new.mediatypeid,:new.type,:new.description,:new.smtp_server,:new.smtp_helo,:new.smtp_email,:new.exec_path,:new.gsm_modem,:new.username,:new.passwd)
);
end;
/
show errors
create or replace trigger zabbix.trau_media_type after update on zabbix.media_type for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.media_type set '
||' mediatypeid='||Nvl(To_Char(:new.mediatypeid),'null')||','
||' type='||Nvl(To_Char(:new.type),'null')||','
||' description='''||Replace(:new.description,'''','''''')||''''||','
||' smtp_server='''||Replace(:new.smtp_server,'''','''''')||''''||','
||' smtp_helo='''||Replace(:new.smtp_helo,'''','''''')||''''||','
||' smtp_email='''||Replace(:new.smtp_email,'''','''''')||''''||','
||' exec_path='''||Replace(:new.exec_path,'''','''''')||''''||','
||' gsm_modem='''||Replace(:new.gsm_modem,'''','''''')||''''||','
||' username='''||Replace(:new.username,'''','''''')||''''||','
||' passwd='''||Replace(:new.passwd,'''','''''')||'''' || ' where  mediatypeid='||:old.mediatypeid );
end;
/
show errors
create or replace trigger zabbix.trad_media_type after delete on zabbix.media_type for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.media_type '
 || ' where  mediatypeid='||:old.mediatypeid 
);
end;
/
show errors
create or replace function zabbix.ins_profiles (profileid in number,userid in number,idx in varchar2,value in varchar2,valuetype in number) return varchar2
is
begin
return
'insert into profiles (profileid,userid,idx,value,valuetype) values ('
||Nvl(To_Char(profileid),'null')||','
||Nvl(To_Char(userid),'null')||','
||''''||Replace(idx,'''','''''')||''''||','
||''''||Replace(value,'''','''''')||''''||','
||Nvl(To_Char(valuetype),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_profiles after insert on zabbix.profiles for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_profiles (:new.profileid,:new.userid,:new.idx,:new.value,:new.valuetype)
);
end;
/
show errors
create or replace trigger zabbix.trau_profiles after update on zabbix.profiles for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.profiles set '
||' profileid='||Nvl(To_Char(:new.profileid),'null')||','
||' userid='||Nvl(To_Char(:new.userid),'null')||','
||' idx='''||Replace(:new.idx,'''','''''')||''''||','
||' value='''||Replace(:new.value,'''','''''')||''''||','
||' valuetype='||Nvl(To_Char(:new.valuetype),'null') || ' where  profileid='||:old.profileid );
end;
/
show errors
create or replace trigger zabbix.trad_profiles after delete on zabbix.profiles for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.profiles '
 || ' where  profileid='||:old.profileid 
);
end;
/
show errors
create or replace function zabbix.ins_rights (rightid in number,groupid in number,type in number,permission in number,id in number) return varchar2
is
begin
return
'insert into rights (rightid,groupid,type,permission,id) values ('
||Nvl(To_Char(rightid),'null')||','
||Nvl(To_Char(groupid),'null')||','
||Nvl(To_Char(type),'null')||','
||Nvl(To_Char(permission),'null')||','
||Nvl(To_Char(id),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_rights after insert on zabbix.rights for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_rights (:new.rightid,:new.groupid,:new.type,:new.permission,:new.id)
);
end;
/
show errors
create or replace trigger zabbix.trau_rights after update on zabbix.rights for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.rights set '
||' rightid='||Nvl(To_Char(:new.rightid),'null')||','
||' groupid='||Nvl(To_Char(:new.groupid),'null')||','
||' type='||Nvl(To_Char(:new.type),'null')||','
||' permission='||Nvl(To_Char(:new.permission),'null')||','
||' id='||Nvl(To_Char(:new.id),'null') || ' where  rightid='||:old.rightid );
end;
/
show errors
create or replace trigger zabbix.trad_rights after delete on zabbix.rights for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.rights '
 || ' where  rightid='||:old.rightid 
);
end;
/
show errors
create or replace function zabbix.ins_screens (screenid in number,name in varchar2,hsize in number,vsize in number) return varchar2
is
begin
return
'insert into screens (screenid,name,hsize,vsize) values ('
||Nvl(To_Char(screenid),'null')||','
||''''||Replace(name,'''','''''')||''''||','
||Nvl(To_Char(hsize),'null')||','
||Nvl(To_Char(vsize),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_screens after insert on zabbix.screens for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_screens (:new.screenid,:new.name,:new.hsize,:new.vsize)
);
end;
/
show errors
create or replace trigger zabbix.trau_screens after update on zabbix.screens for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.screens set '
||' screenid='||Nvl(To_Char(:new.screenid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||''''||','
||' hsize='||Nvl(To_Char(:new.hsize),'null')||','
||' vsize='||Nvl(To_Char(:new.vsize),'null') || ' where  screenid='||:old.screenid );
end;
/
show errors
create or replace trigger zabbix.trad_screens after delete on zabbix.screens for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.screens '
 || ' where  screenid='||:old.screenid 
);
end;
/
show errors
create or replace function zabbix.ins_screens_items (screenitemid in number,screenid in number,resourcetype in number,resourceid in number,width in number,height in number,x in number,y in number,colspan in number,rowspan in number,elements in number,valign in number,halign in number,style in number,url in varchar2) return varchar2
is
begin
return
'insert into screens_items (screenitemid,screenid,resourcetype,resourceid,width,height,x,y,colspan,rowspan,elements,valign,halign,style,url) values ('
||Nvl(To_Char(screenitemid),'null')||','
||Nvl(To_Char(screenid),'null')||','
||Nvl(To_Char(resourcetype),'null')||','
||Nvl(To_Char(resourceid),'null')||','
||Nvl(To_Char(width),'null')||','
||Nvl(To_Char(height),'null')||','
||Nvl(To_Char(x),'null')||','
||Nvl(To_Char(y),'null')||','
||Nvl(To_Char(colspan),'null')||','
||Nvl(To_Char(rowspan),'null')||','
||Nvl(To_Char(elements),'null')||','
||Nvl(To_Char(valign),'null')||','
||Nvl(To_Char(halign),'null')||','
||Nvl(To_Char(style),'null')||','
||''''||Replace(url,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_screens_items after insert on zabbix.screens_items for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_screens_items (:new.screenitemid,:new.screenid,:new.resourcetype,:new.resourceid,:new.width,:new.height,:new.x,:new.y,:new.colspan,:new.rowspan,:new.elements,:new.valign,:new.halign,:new.style,:new.url)
);
end;
/
show errors
create or replace trigger zabbix.trau_screens_items after update on zabbix.screens_items for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.screens_items set '
||' screenitemid='||Nvl(To_Char(:new.screenitemid),'null')||','
||' screenid='||Nvl(To_Char(:new.screenid),'null')||','
||' resourcetype='||Nvl(To_Char(:new.resourcetype),'null')||','
||' resourceid='||Nvl(To_Char(:new.resourceid),'null')||','
||' width='||Nvl(To_Char(:new.width),'null')||','
||' height='||Nvl(To_Char(:new.height),'null')||','
||' x='||Nvl(To_Char(:new.x),'null')||','
||' y='||Nvl(To_Char(:new.y),'null')||','
||' colspan='||Nvl(To_Char(:new.colspan),'null')||','
||' rowspan='||Nvl(To_Char(:new.rowspan),'null')||','
||' elements='||Nvl(To_Char(:new.elements),'null')||','
||' valign='||Nvl(To_Char(:new.valign),'null')||','
||' halign='||Nvl(To_Char(:new.halign),'null')||','
||' style='||Nvl(To_Char(:new.style),'null')||','
||' url='''||Replace(:new.url,'''','''''')||'''' || ' where  screenitemid='||:old.screenitemid );
end;
/
show errors
create or replace trigger zabbix.trad_screens_items after delete on zabbix.screens_items for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.screens_items '
 || ' where  screenitemid='||:old.screenitemid 
);
end;
/
show errors
create or replace function zabbix.ins_services (serviceid in number,name in varchar2,status in number,algorithm in number,triggerid in number,showsla in number,goodsla in number,sortorder in number) return varchar2
is
begin
return
'insert into services (serviceid,name,status,algorithm,triggerid,showsla,goodsla,sortorder) values ('
||Nvl(To_Char(serviceid),'null')||','
||''''||Replace(name,'''','''''')||''''||','
||Nvl(To_Char(status),'null')||','
||Nvl(To_Char(algorithm),'null')||','
||Nvl(To_Char(triggerid),'null')||','
||Nvl(To_Char(showsla),'null')||','
||Nvl(To_Char(goodsla),'null')||','
||Nvl(To_Char(sortorder),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_services after insert on zabbix.services for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_services (:new.serviceid,:new.name,:new.status,:new.algorithm,:new.triggerid,:new.showsla,:new.goodsla,:new.sortorder)
);
end;
/
show errors
create or replace trigger zabbix.trau_services after update on zabbix.services for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.services set '
||' serviceid='||Nvl(To_Char(:new.serviceid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||''''||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' algorithm='||Nvl(To_Char(:new.algorithm),'null')||','
||' triggerid='||Nvl(To_Char(:new.triggerid),'null')||','
||' showsla='||Nvl(To_Char(:new.showsla),'null')||','
||' goodsla='||Nvl(To_Char(:new.goodsla),'null')||','
||' sortorder='||Nvl(To_Char(:new.sortorder),'null') || ' where  serviceid='||:old.serviceid );
end;
/
show errors
create or replace trigger zabbix.trad_services after delete on zabbix.services for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.services '
 || ' where  serviceid='||:old.serviceid 
);
end;
/
show errors
create or replace function zabbix.ins_service_alarms (servicealarmid in number,serviceid in number,clock in number,value in number) return varchar2
is
begin
return
'insert into service_alarms (servicealarmid,serviceid,clock,value) values ('
||Nvl(To_Char(servicealarmid),'null')||','
||Nvl(To_Char(serviceid),'null')||','
||Nvl(To_Char(clock),'null')||','
||Nvl(To_Char(value),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_service_alarms after insert on zabbix.service_alarms for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_service_alarms (:new.servicealarmid,:new.serviceid,:new.clock,:new.value)
);
end;
/
show errors
create or replace trigger zabbix.trau_service_alarms after update on zabbix.service_alarms for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.service_alarms set '
||' servicealarmid='||Nvl(To_Char(:new.servicealarmid),'null')||','
||' serviceid='||Nvl(To_Char(:new.serviceid),'null')||','
||' clock='||Nvl(To_Char(:new.clock),'null')||','
||' value='||Nvl(To_Char(:new.value),'null') || ' where  servicealarmid='||:old.servicealarmid );
end;
/
show errors
create or replace trigger zabbix.trad_service_alarms after delete on zabbix.service_alarms for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.service_alarms '
 || ' where  servicealarmid='||:old.servicealarmid 
);
end;
/
show errors
create or replace function zabbix.ins_services_links (linkid in number,serviceupid in number,servicedownid in number,soft in number) return varchar2
is
begin
return
'insert into services_links (linkid,serviceupid,servicedownid,soft) values ('
||Nvl(To_Char(linkid),'null')||','
||Nvl(To_Char(serviceupid),'null')||','
||Nvl(To_Char(servicedownid),'null')||','
||Nvl(To_Char(soft),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_services_links after insert on zabbix.services_links for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_services_links (:new.linkid,:new.serviceupid,:new.servicedownid,:new.soft)
);
end;
/
show errors
create or replace trigger zabbix.trau_services_links after update on zabbix.services_links for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.services_links set '
||' linkid='||Nvl(To_Char(:new.linkid),'null')||','
||' serviceupid='||Nvl(To_Char(:new.serviceupid),'null')||','
||' servicedownid='||Nvl(To_Char(:new.servicedownid),'null')||','
||' soft='||Nvl(To_Char(:new.soft),'null') || ' where  linkid='||:old.linkid );
end;
/
show errors
create or replace trigger zabbix.trad_services_links after delete on zabbix.services_links for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.services_links '
 || ' where  linkid='||:old.linkid 
);
end;
/
show errors
create or replace function zabbix.ins_sysmaps_links (linkid in number,sysmapid in number,selementid1 in number,selementid2 in number,triggerid in number,drawtype_off in number,color_off in varchar2,drawtype_on in number,color_on in varchar2) return varchar2
is
begin
return
'insert into sysmaps_links (linkid,sysmapid,selementid1,selementid2,triggerid,drawtype_off,color_off,drawtype_on,color_on) values ('
||Nvl(To_Char(linkid),'null')||','
||Nvl(To_Char(sysmapid),'null')||','
||Nvl(To_Char(selementid1),'null')||','
||Nvl(To_Char(selementid2),'null')||','
||Nvl(To_Char(triggerid),'null')||','
||Nvl(To_Char(drawtype_off),'null')||','
||''''||Replace(color_off,'''','''''')||''''||','
||Nvl(To_Char(drawtype_on),'null')||','
||''''||Replace(color_on,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_sysmaps_links after insert on zabbix.sysmaps_links for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_sysmaps_links (:new.linkid,:new.sysmapid,:new.selementid1,:new.selementid2,:new.triggerid,:new.drawtype_off,:new.color_off,:new.drawtype_on,:new.color_on)
);
end;
/
show errors
create or replace trigger zabbix.trau_sysmaps_links after update on zabbix.sysmaps_links for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.sysmaps_links set '
||' linkid='||Nvl(To_Char(:new.linkid),'null')||','
||' sysmapid='||Nvl(To_Char(:new.sysmapid),'null')||','
||' selementid1='||Nvl(To_Char(:new.selementid1),'null')||','
||' selementid2='||Nvl(To_Char(:new.selementid2),'null')||','
||' triggerid='||Nvl(To_Char(:new.triggerid),'null')||','
||' drawtype_off='||Nvl(To_Char(:new.drawtype_off),'null')||','
||' color_off='''||Replace(:new.color_off,'''','''''')||''''||','
||' drawtype_on='||Nvl(To_Char(:new.drawtype_on),'null')||','
||' color_on='''||Replace(:new.color_on,'''','''''')||'''' || ' where  linkid='||:old.linkid );
end;
/
show errors
create or replace trigger zabbix.trad_sysmaps_links after delete on zabbix.sysmaps_links for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.sysmaps_links '
 || ' where  linkid='||:old.linkid 
);
end;
/
show errors
create or replace function zabbix.ins_sysmaps_elements (selementid in number,sysmapid in number,elementid in number,elementtype in number,iconid_off in number,iconid_on in number,iconid_unknown in number,label in varchar2,label_location in number,x in number,y in number,url in varchar2) return varchar2
is
begin
return
'insert into sysmaps_elements (selementid,sysmapid,elementid,elementtype,iconid_off,iconid_on,iconid_unknown,label,label_location,x,y,url) values ('
||Nvl(To_Char(selementid),'null')||','
||Nvl(To_Char(sysmapid),'null')||','
||Nvl(To_Char(elementid),'null')||','
||Nvl(To_Char(elementtype),'null')||','
||Nvl(To_Char(iconid_off),'null')||','
||Nvl(To_Char(iconid_on),'null')||','
||Nvl(To_Char(iconid_unknown),'null')||','
||''''||Replace(label,'''','''''')||''''||','
||Nvl(To_Char(label_location),'null')||','
||Nvl(To_Char(x),'null')||','
||Nvl(To_Char(y),'null')||','
||''''||Replace(url,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_sysmaps_elements after insert on zabbix.sysmaps_elements for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_sysmaps_elements (:new.selementid,:new.sysmapid,:new.elementid,:new.elementtype,:new.iconid_off,:new.iconid_on,:new.iconid_unknown,:new.label,:new.label_location,:new.x,:new.y,:new.url)
);
end;
/
show errors
create or replace trigger zabbix.trau_sysmaps_elements after update on zabbix.sysmaps_elements for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.sysmaps_elements set '
||' selementid='||Nvl(To_Char(:new.selementid),'null')||','
||' sysmapid='||Nvl(To_Char(:new.sysmapid),'null')||','
||' elementid='||Nvl(To_Char(:new.elementid),'null')||','
||' elementtype='||Nvl(To_Char(:new.elementtype),'null')||','
||' iconid_off='||Nvl(To_Char(:new.iconid_off),'null')||','
||' iconid_on='||Nvl(To_Char(:new.iconid_on),'null')||','
||' iconid_unknown='||Nvl(To_Char(:new.iconid_unknown),'null')||','
||' label='''||Replace(:new.label,'''','''''')||''''||','
||' label_location='||Nvl(To_Char(:new.label_location),'null')||','
||' x='||Nvl(To_Char(:new.x),'null')||','
||' y='||Nvl(To_Char(:new.y),'null')||','
||' url='''||Replace(:new.url,'''','''''')||'''' || ' where  selementid='||:old.selementid );
end;
/
show errors
create or replace trigger zabbix.trad_sysmaps_elements after delete on zabbix.sysmaps_elements for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.sysmaps_elements '
 || ' where  selementid='||:old.selementid 
);
end;
/
show errors
create or replace function zabbix.ins_sysmaps (sysmapid in number,name in varchar2,width in number,height in number,backgroundid in number,label_type in number,label_location in number) return varchar2
is
begin
return
'insert into sysmaps (sysmapid,name,width,height,backgroundid,label_type,label_location) values ('
||Nvl(To_Char(sysmapid),'null')||','
||''''||Replace(name,'''','''''')||''''||','
||Nvl(To_Char(width),'null')||','
||Nvl(To_Char(height),'null')||','
||Nvl(To_Char(backgroundid),'null')||','
||Nvl(To_Char(label_type),'null')||','
||Nvl(To_Char(label_location),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_sysmaps after insert on zabbix.sysmaps for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_sysmaps (:new.sysmapid,:new.name,:new.width,:new.height,:new.backgroundid,:new.label_type,:new.label_location)
);
end;
/
show errors
create or replace trigger zabbix.trau_sysmaps after update on zabbix.sysmaps for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.sysmaps set '
||' sysmapid='||Nvl(To_Char(:new.sysmapid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||''''||','
||' width='||Nvl(To_Char(:new.width),'null')||','
||' height='||Nvl(To_Char(:new.height),'null')||','
||' backgroundid='||Nvl(To_Char(:new.backgroundid),'null')||','
||' label_type='||Nvl(To_Char(:new.label_type),'null')||','
||' label_location='||Nvl(To_Char(:new.label_location),'null') || ' where  sysmapid='||:old.sysmapid );
end;
/
show errors
create or replace trigger zabbix.trad_sysmaps after delete on zabbix.sysmaps for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.sysmaps '
 || ' where  sysmapid='||:old.sysmapid 
);
end;
/
show errors
create or replace function zabbix.ins_triggers (triggerid in number,expression in varchar2,description in varchar2,url in varchar2,status in number,value in number,priority in number,lastchange in number,dep_level in number,comments in varchar2,error in varchar2,templateid in number) return varchar2
is
begin
return
'insert into triggers (triggerid,expression,description,url,status,value,priority,lastchange,dep_level,comments,error,templateid) values ('
||Nvl(To_Char(triggerid),'null')||','
||''''||Replace(expression,'''','''''')||''''||','
||''''||Replace(description,'''','''''')||''''||','
||''''||Replace(url,'''','''''')||''''||','
||Nvl(To_Char(status),'null')||','
||Nvl(To_Char(value),'null')||','
||Nvl(To_Char(priority),'null')||','
||Nvl(To_Char(lastchange),'null')||','
||Nvl(To_Char(dep_level),'null')||','
||''''||Replace(comments,'''','''''')||''''||','
||''''||Replace(error,'''','''''')||''''||','
||Nvl(To_Char(templateid),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_triggers after insert on zabbix.triggers for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_triggers (:new.triggerid,:new.expression,:new.description,:new.url,:new.status,:new.value,:new.priority,:new.lastchange,:new.dep_level,:new.comments,:new.error,:new.templateid)
);
end;
/
show errors
create or replace trigger zabbix.trau_triggers after update on zabbix.triggers for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.triggers set '
||' triggerid='||Nvl(To_Char(:new.triggerid),'null')||','
||' expression='''||Replace(:new.expression,'''','''''')||''''||','
||' description='''||Replace(:new.description,'''','''''')||''''||','
||' url='''||Replace(:new.url,'''','''''')||''''||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' value='||Nvl(To_Char(:new.value),'null')||','
||' priority='||Nvl(To_Char(:new.priority),'null')||','
||' lastchange='||Nvl(To_Char(:new.lastchange),'null')||','
||' dep_level='||Nvl(To_Char(:new.dep_level),'null')||','
||' comments='''||Replace(:new.comments,'''','''''')||''''||','
||' error='''||Replace(:new.error,'''','''''')||''''||','
||' templateid='||Nvl(To_Char(:new.templateid),'null') || ' where  triggerid='||:old.triggerid );
end;
/
show errors
create or replace trigger zabbix.trad_triggers after delete on zabbix.triggers for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.triggers '
 || ' where  triggerid='||:old.triggerid 
);
end;
/
show errors
create or replace function zabbix.ins_trigger_depends (triggerdepid in number,triggerid_down in number,triggerid_up in number) return varchar2
is
begin
return
'insert into trigger_depends (triggerdepid,triggerid_down,triggerid_up) values ('
||Nvl(To_Char(triggerdepid),'null')||','
||Nvl(To_Char(triggerid_down),'null')||','
||Nvl(To_Char(triggerid_up),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_trigger_depends after insert on zabbix.trigger_depends for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_trigger_depends (:new.triggerdepid,:new.triggerid_down,:new.triggerid_up)
);
end;
/
show errors
create or replace trigger zabbix.trau_trigger_depends after update on zabbix.trigger_depends for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.trigger_depends set '
||' triggerdepid='||Nvl(To_Char(:new.triggerdepid),'null')||','
||' triggerid_down='||Nvl(To_Char(:new.triggerid_down),'null')||','
||' triggerid_up='||Nvl(To_Char(:new.triggerid_up),'null') || ' where  triggerdepid='||:old.triggerdepid );
end;
/
show errors
create or replace trigger zabbix.trad_trigger_depends after delete on zabbix.trigger_depends for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.trigger_depends '
 || ' where  triggerdepid='||:old.triggerdepid 
);
end;
/
show errors
create or replace function zabbix.ins_users (userid in number,alias in varchar2,name in varchar2,surname in varchar2,passwd in varchar2,url in varchar2,autologout in number,lang in varchar2,refresh in number,type in number) return varchar2
is
begin
return
'insert into users (userid,alias,name,surname,passwd,url,autologout,lang,refresh,type) values ('
||Nvl(To_Char(userid),'null')||','
||''''||Replace(alias,'''','''''')||''''||','
||''''||Replace(name,'''','''''')||''''||','
||''''||Replace(surname,'''','''''')||''''||','
||''''||Replace(passwd,'''','''''')||''''||','
||''''||Replace(url,'''','''''')||''''||','
||Nvl(To_Char(autologout),'null')||','
||''''||Replace(lang,'''','''''')||''''||','
||Nvl(To_Char(refresh),'null')||','
||Nvl(To_Char(type),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_users after insert on zabbix.users for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_users (:new.userid,:new.alias,:new.name,:new.surname,:new.passwd,:new.url,:new.autologout,:new.lang,:new.refresh,:new.type)
);
end;
/
show errors
create or replace trigger zabbix.trau_users after update on zabbix.users for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.users set '
||' userid='||Nvl(To_Char(:new.userid),'null')||','
||' alias='''||Replace(:new.alias,'''','''''')||''''||','
||' name='''||Replace(:new.name,'''','''''')||''''||','
||' surname='''||Replace(:new.surname,'''','''''')||''''||','
||' passwd='''||Replace(:new.passwd,'''','''''')||''''||','
||' url='''||Replace(:new.url,'''','''''')||''''||','
||' autologout='||Nvl(To_Char(:new.autologout),'null')||','
||' lang='''||Replace(:new.lang,'''','''''')||''''||','
||' refresh='||Nvl(To_Char(:new.refresh),'null')||','
||' type='||Nvl(To_Char(:new.type),'null') || ' where  userid='||:old.userid );
end;
/
show errors
create or replace trigger zabbix.trad_users after delete on zabbix.users for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.users '
 || ' where  userid='||:old.userid 
);
end;
/
show errors
create or replace function zabbix.ins_usrgrp (usrgrpid in number,name in varchar2) return varchar2
is
begin
return
'insert into usrgrp (usrgrpid,name) values ('
||Nvl(To_Char(usrgrpid),'null')||','
||''''||Replace(name,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_usrgrp after insert on zabbix.usrgrp for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_usrgrp (:new.usrgrpid,:new.name)
);
end;
/
show errors
create or replace trigger zabbix.trau_usrgrp after update on zabbix.usrgrp for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.usrgrp set '
||' usrgrpid='||Nvl(To_Char(:new.usrgrpid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||'''' || ' where  usrgrpid='||:old.usrgrpid );
end;
/
show errors
create or replace trigger zabbix.trad_usrgrp after delete on zabbix.usrgrp for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.usrgrp '
 || ' where  usrgrpid='||:old.usrgrpid 
);
end;
/
show errors
create or replace function zabbix.ins_users_groups (id in number,usrgrpid in number,userid in number) return varchar2
is
begin
return
'insert into users_groups (id,usrgrpid,userid) values ('
||Nvl(To_Char(id),'null')||','
||Nvl(To_Char(usrgrpid),'null')||','
||Nvl(To_Char(userid),'null')
||')';
end;
/
show errors
create or replace trigger zabbix.trai_users_groups after insert on zabbix.users_groups for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_users_groups (:new.id,:new.usrgrpid,:new.userid)
);
end;
/
show errors
create or replace trigger zabbix.trau_users_groups after update on zabbix.users_groups for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.users_groups set '
||' id='||Nvl(To_Char(:new.id),'null')||','
||' usrgrpid='||Nvl(To_Char(:new.usrgrpid),'null')||','
||' userid='||Nvl(To_Char(:new.userid),'null') || ' where  id='||:old.id );
end;
/
show errors
create or replace trigger zabbix.trad_users_groups after delete on zabbix.users_groups for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.users_groups '
 || ' where  id='||:old.id 
);
end;
/
show errors
create or replace function zabbix.ins_valuemaps (valuemapid in number,name in varchar2) return varchar2
is
begin
return
'insert into valuemaps (valuemapid,name) values ('
||Nvl(To_Char(valuemapid),'null')||','
||''''||Replace(name,'''','''''')||''''
||')';
end;
/
show errors
create or replace trigger zabbix.trai_valuemaps after insert on zabbix.valuemaps for each row
begin
Replicate2MySQL.RunSQL
(
zabbix.ins_valuemaps (:new.valuemapid,:new.name)
);
end;
/
show errors
create or replace trigger zabbix.trau_valuemaps after update on zabbix.valuemaps for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.valuemaps set '
||' valuemapid='||Nvl(To_Char(:new.valuemapid),'null')||','
||' name='''||Replace(:new.name,'''','''''')||'''' || ' where  valuemapid='||:old.valuemapid );
end;
/
show errors
create or replace trigger zabbix.trad_valuemaps after delete on zabbix.valuemaps for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.valuemaps '
 || ' where  valuemapid='||:old.valuemapid 
);
end;
/
show errors
