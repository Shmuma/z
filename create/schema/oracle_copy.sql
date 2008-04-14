
set define off

create or replace and compile java source named system."TReplicate2MySQL" as
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
  procedure RunSQL(csqlCmd in varchar2) as language java name 'TReplicate2MySQL.RunSQL(java.lang.String)';
end;
/
grant execute on replicate2mysql to zabbix;
/
create or replace trigger zabbix.trai_slideshows after insert on zabbix.slideshows for each row
begin
Replicate2MySQL.RunSQL
(
'insert into slideshows (slideshowid,name,delay) values ('
||Nvl(To_Char(:new.slideshowid),'null')||','
||''''||:new.name||''''||','
||Nvl(To_Char(:new.delay),'null')
||')'
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
||' name='''||:new.name||''''||','
||' delay='||Nvl(To_Char(:new.delay),'null') || ' where  slideshowid='''||:old.slideshowid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_slideshows after delete on zabbix.slideshows for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.slideshows '
 || ' where  slideshowid='''||:old.slideshowid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_slides after insert on zabbix.slides for each row
begin
Replicate2MySQL.RunSQL
(
'insert into slides (slideid,slideshowid,screenid,step,delay) values ('
||Nvl(To_Char(:new.slideid),'null')||','
||Nvl(To_Char(:new.slideshowid),'null')||','
||Nvl(To_Char(:new.screenid),'null')||','
||Nvl(To_Char(:new.step),'null')||','
||Nvl(To_Char(:new.delay),'null')
||')'
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
||' delay='||Nvl(To_Char(:new.delay),'null') || ' where  slideid='''||:old.slideid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_slides after delete on zabbix.slides for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.slides '
 || ' where  slideid='''||:old.slideid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_drules after insert on zabbix.drules for each row
begin
Replicate2MySQL.RunSQL
(
'insert into drules (druleid,name,iprange,delay,nextcheck,status) values ('
||Nvl(To_Char(:new.druleid),'null')||','
||''''||:new.name||''''||','
||''''||:new.iprange||''''||','
||Nvl(To_Char(:new.delay),'null')||','
||Nvl(To_Char(:new.nextcheck),'null')||','
||Nvl(To_Char(:new.status),'null')
||')'
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
||' name='''||:new.name||''''||','
||' iprange='''||:new.iprange||''''||','
||' delay='||Nvl(To_Char(:new.delay),'null')||','
||' nextcheck='||Nvl(To_Char(:new.nextcheck),'null')||','
||' status='||Nvl(To_Char(:new.status),'null') || ' where  druleid='''||:old.druleid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_drules after delete on zabbix.drules for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.drules '
 || ' where  druleid='''||:old.druleid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_dchecks after insert on zabbix.dchecks for each row
begin
Replicate2MySQL.RunSQL
(
'insert into dchecks (dcheckid,druleid,type,key_,snmp_community,ports) values ('
||Nvl(To_Char(:new.dcheckid),'null')||','
||Nvl(To_Char(:new.druleid),'null')||','
||Nvl(To_Char(:new.type),'null')||','
||''''||:new.key_||''''||','
||''''||:new.snmp_community||''''||','
||''''||:new.ports||''''
||')'
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
||' key_='''||:new.key_||''''||','
||' snmp_community='''||:new.snmp_community||''''||','
||' ports='''||:new.ports||'''' || ' where  dcheckid='''||:old.dcheckid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_dchecks after delete on zabbix.dchecks for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.dchecks '
 || ' where  dcheckid='''||:old.dcheckid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_dhosts after insert on zabbix.dhosts for each row
begin
Replicate2MySQL.RunSQL
(
'insert into dhosts (dhostid,druleid,ip,status,lastup,lastdown) values ('
||Nvl(To_Char(:new.dhostid),'null')||','
||Nvl(To_Char(:new.druleid),'null')||','
||''''||:new.ip||''''||','
||Nvl(To_Char(:new.status),'null')||','
||Nvl(To_Char(:new.lastup),'null')||','
||Nvl(To_Char(:new.lastdown),'null')
||')'
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
||' ip='''||:new.ip||''''||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' lastup='||Nvl(To_Char(:new.lastup),'null')||','
||' lastdown='||Nvl(To_Char(:new.lastdown),'null') || ' where  dhostid='''||:old.dhostid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_dhosts after delete on zabbix.dhosts for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.dhosts '
 || ' where  dhostid='''||:old.dhostid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_dservices after insert on zabbix.dservices for each row
begin
Replicate2MySQL.RunSQL
(
'insert into dservices (dserviceid,dhostid,type,key_,value,port,status,lastup,lastdown) values ('
||Nvl(To_Char(:new.dserviceid),'null')||','
||Nvl(To_Char(:new.dhostid),'null')||','
||Nvl(To_Char(:new.type),'null')||','
||''''||:new.key_||''''||','
||''''||:new.value||''''||','
||Nvl(To_Char(:new.port),'null')||','
||Nvl(To_Char(:new.status),'null')||','
||Nvl(To_Char(:new.lastup),'null')||','
||Nvl(To_Char(:new.lastdown),'null')
||')'
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
||' key_='''||:new.key_||''''||','
||' value='''||:new.value||''''||','
||' port='||Nvl(To_Char(:new.port),'null')||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' lastup='||Nvl(To_Char(:new.lastup),'null')||','
||' lastdown='||Nvl(To_Char(:new.lastdown),'null') || ' where  dserviceid='''||:old.dserviceid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_dservices after delete on zabbix.dservices for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.dservices '
 || ' where  dserviceid='''||:old.dserviceid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_ids after insert on zabbix.ids for each row
begin
Replicate2MySQL.RunSQL
(
'insert into ids (nodeid,table_name,field_name,nextid) values ('
||Nvl(To_Char(:new.nodeid),'null')||','
||''''||:new.table_name||''''||','
||''''||:new.field_name||''''||','
||Nvl(To_Char(:new.nextid),'null')
||')'
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
||' table_name='''||:new.table_name||''''||','
||' field_name='''||:new.field_name||''''||','
||' nextid='||Nvl(To_Char(:new.nextid),'null') || ' where  nodeid='''||:old.nodeid||'''' || ' and  table_name='''||:old.table_name||'''' || ' and  field_name='''||:old.field_name||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_ids after delete on zabbix.ids for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.ids '
 || ' where  nodeid='''||:old.nodeid||'''' || ' and  table_name='''||:old.table_name||'''' || ' and  field_name='''||:old.field_name||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_httptest after insert on zabbix.httptest for each row
begin
Replicate2MySQL.RunSQL
(
'insert into httptest (httptestid,name,applicationid,lastcheck,nextcheck,curstate,curstep,lastfailedstep,delay,status,macros,agent,time,error) values ('
||Nvl(To_Char(:new.httptestid),'null')||','
||''''||:new.name||''''||','
||Nvl(To_Char(:new.applicationid),'null')||','
||Nvl(To_Char(:new.lastcheck),'null')||','
||Nvl(To_Char(:new.nextcheck),'null')||','
||Nvl(To_Char(:new.curstate),'null')||','
||Nvl(To_Char(:new.curstep),'null')||','
||Nvl(To_Char(:new.lastfailedstep),'null')||','
||Nvl(To_Char(:new.delay),'null')||','
||Nvl(To_Char(:new.status),'null')||','
||''''||:new.macros||''''||','
||''''||:new.agent||''''||','
||Nvl(To_Char(:new.time),'null')||','
||''''||:new.error||''''
||')'
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
||' name='''||:new.name||''''||','
||' applicationid='||Nvl(To_Char(:new.applicationid),'null')||','
||' lastcheck='||Nvl(To_Char(:new.lastcheck),'null')||','
||' nextcheck='||Nvl(To_Char(:new.nextcheck),'null')||','
||' curstate='||Nvl(To_Char(:new.curstate),'null')||','
||' curstep='||Nvl(To_Char(:new.curstep),'null')||','
||' lastfailedstep='||Nvl(To_Char(:new.lastfailedstep),'null')||','
||' delay='||Nvl(To_Char(:new.delay),'null')||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' macros='''||:new.macros||''''||','
||' agent='''||:new.agent||''''||','
||' time='||Nvl(To_Char(:new.time),'null')||','
||' error='''||:new.error||'''' || ' where  httptestid='''||:old.httptestid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_httptest after delete on zabbix.httptest for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.httptest '
 || ' where  httptestid='''||:old.httptestid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_httpstep after insert on zabbix.httpstep for each row
begin
Replicate2MySQL.RunSQL
(
'insert into httpstep (httpstepid,httptestid,name,no,url,timeout,posts,required,status_codes) values ('
||Nvl(To_Char(:new.httpstepid),'null')||','
||Nvl(To_Char(:new.httptestid),'null')||','
||''''||:new.name||''''||','
||Nvl(To_Char(:new.no),'null')||','
||''''||:new.url||''''||','
||Nvl(To_Char(:new.timeout),'null')||','
||''''||:new.posts||''''||','
||''''||:new.required||''''||','
||''''||:new.status_codes||''''
||')'
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
||' name='''||:new.name||''''||','
||' no='||Nvl(To_Char(:new.no),'null')||','
||' url='''||:new.url||''''||','
||' timeout='||Nvl(To_Char(:new.timeout),'null')||','
||' posts='''||:new.posts||''''||','
||' required='''||:new.required||''''||','
||' status_codes='''||:new.status_codes||'''' || ' where  httpstepid='''||:old.httpstepid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_httpstep after delete on zabbix.httpstep for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.httpstep '
 || ' where  httpstepid='''||:old.httpstepid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_httpstepitem after insert on zabbix.httpstepitem for each row
begin
Replicate2MySQL.RunSQL
(
'insert into httpstepitem (httpstepitemid,httpstepid,itemid,type) values ('
||Nvl(To_Char(:new.httpstepitemid),'null')||','
||Nvl(To_Char(:new.httpstepid),'null')||','
||Nvl(To_Char(:new.itemid),'null')||','
||Nvl(To_Char(:new.type),'null')
||')'
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
||' type='||Nvl(To_Char(:new.type),'null') || ' where  httpstepitemid='''||:old.httpstepitemid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_httpstepitem after delete on zabbix.httpstepitem for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.httpstepitem '
 || ' where  httpstepitemid='''||:old.httpstepitemid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_httptestitem after insert on zabbix.httptestitem for each row
begin
Replicate2MySQL.RunSQL
(
'insert into httptestitem (httptestitemid,httptestid,itemid,type) values ('
||Nvl(To_Char(:new.httptestitemid),'null')||','
||Nvl(To_Char(:new.httptestid),'null')||','
||Nvl(To_Char(:new.itemid),'null')||','
||Nvl(To_Char(:new.type),'null')
||')'
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
||' type='||Nvl(To_Char(:new.type),'null') || ' where  httptestitemid='''||:old.httptestitemid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_httptestitem after delete on zabbix.httptestitem for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.httptestitem '
 || ' where  httptestitemid='''||:old.httptestitemid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_nodes after insert on zabbix.nodes for each row
begin
Replicate2MySQL.RunSQL
(
'insert into nodes (nodeid,name,timezone,ip,port,slave_history,slave_trends,event_lastid,history_lastid,history_str_lastid,history_uint_lastid,nodetype,masterid) values ('
||Nvl(To_Char(:new.nodeid),'null')||','
||''''||:new.name||''''||','
||Nvl(To_Char(:new.timezone),'null')||','
||''''||:new.ip||''''||','
||Nvl(To_Char(:new.port),'null')||','
||Nvl(To_Char(:new.slave_history),'null')||','
||Nvl(To_Char(:new.slave_trends),'null')||','
||Nvl(To_Char(:new.event_lastid),'null')||','
||Nvl(To_Char(:new.history_lastid),'null')||','
||Nvl(To_Char(:new.history_str_lastid),'null')||','
||Nvl(To_Char(:new.history_uint_lastid),'null')||','
||Nvl(To_Char(:new.nodetype),'null')||','
||Nvl(To_Char(:new.masterid),'null')
||')'
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
||' name='''||:new.name||''''||','
||' timezone='||Nvl(To_Char(:new.timezone),'null')||','
||' ip='''||:new.ip||''''||','
||' port='||Nvl(To_Char(:new.port),'null')||','
||' slave_history='||Nvl(To_Char(:new.slave_history),'null')||','
||' slave_trends='||Nvl(To_Char(:new.slave_trends),'null')||','
||' event_lastid='||Nvl(To_Char(:new.event_lastid),'null')||','
||' history_lastid='||Nvl(To_Char(:new.history_lastid),'null')||','
||' history_str_lastid='||Nvl(To_Char(:new.history_str_lastid),'null')||','
||' history_uint_lastid='||Nvl(To_Char(:new.history_uint_lastid),'null')||','
||' nodetype='||Nvl(To_Char(:new.nodetype),'null')||','
||' masterid='||Nvl(To_Char(:new.masterid),'null') || ' where  nodeid='''||:old.nodeid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_nodes after delete on zabbix.nodes for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.nodes '
 || ' where  nodeid='''||:old.nodeid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_node_cksum after insert on zabbix.node_cksum for each row
begin
Replicate2MySQL.RunSQL
(
'insert into node_cksum (cksumid,nodeid,tablename,fieldname,recordid,cksumtype,cksum) values ('
||Nvl(To_Char(:new.cksumid),'null')||','
||Nvl(To_Char(:new.nodeid),'null')||','
||''''||:new.tablename||''''||','
||''''||:new.fieldname||''''||','
||Nvl(To_Char(:new.recordid),'null')||','
||Nvl(To_Char(:new.cksumtype),'null')||','
||''''||:new.cksum||''''
||')'
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
||' tablename='''||:new.tablename||''''||','
||' fieldname='''||:new.fieldname||''''||','
||' recordid='||Nvl(To_Char(:new.recordid),'null')||','
||' cksumtype='||Nvl(To_Char(:new.cksumtype),'null')||','
||' cksum='''||:new.cksum||'''' || ' where  cksumid='''||:old.cksumid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_node_cksum after delete on zabbix.node_cksum for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.node_cksum '
 || ' where  cksumid='''||:old.cksumid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_node_configlog after insert on zabbix.node_configlog for each row
begin
Replicate2MySQL.RunSQL
(
'insert into node_configlog (conflogid,nodeid,tablename,recordid,operation,sync_master,sync_slave) values ('
||Nvl(To_Char(:new.conflogid),'null')||','
||Nvl(To_Char(:new.nodeid),'null')||','
||''''||:new.tablename||''''||','
||Nvl(To_Char(:new.recordid),'null')||','
||Nvl(To_Char(:new.operation),'null')||','
||Nvl(To_Char(:new.sync_master),'null')||','
||Nvl(To_Char(:new.sync_slave),'null')
||')'
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
||' tablename='''||:new.tablename||''''||','
||' recordid='||Nvl(To_Char(:new.recordid),'null')||','
||' operation='||Nvl(To_Char(:new.operation),'null')||','
||' sync_master='||Nvl(To_Char(:new.sync_master),'null')||','
||' sync_slave='||Nvl(To_Char(:new.sync_slave),'null') || ' where  nodeid='''||:old.nodeid||'''' || ' and  conflogid='''||:old.conflogid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_node_configlog after delete on zabbix.node_configlog for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.node_configlog '
 || ' where  nodeid='''||:old.nodeid||'''' || ' and  conflogid='''||:old.conflogid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_acknowledges after insert on zabbix.acknowledges for each row
begin
Replicate2MySQL.RunSQL
(
'insert into acknowledges (acknowledgeid,userid,eventid,clock,message) values ('
||Nvl(To_Char(:new.acknowledgeid),'null')||','
||Nvl(To_Char(:new.userid),'null')||','
||Nvl(To_Char(:new.eventid),'null')||','
||Nvl(To_Char(:new.clock),'null')||','
||''''||:new.message||''''
||')'
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
||' message='''||:new.message||'''' || ' where  acknowledgeid='''||:old.acknowledgeid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_acknowledges after delete on zabbix.acknowledges for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.acknowledges '
 || ' where  acknowledgeid='''||:old.acknowledgeid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_actions after insert on zabbix.actions for each row
begin
Replicate2MySQL.RunSQL
(
'insert into actions (actionid,name,eventsource,evaltype,status) values ('
||Nvl(To_Char(:new.actionid),'null')||','
||''''||:new.name||''''||','
||Nvl(To_Char(:new.eventsource),'null')||','
||Nvl(To_Char(:new.evaltype),'null')||','
||Nvl(To_Char(:new.status),'null')
||')'
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
||' name='''||:new.name||''''||','
||' eventsource='||Nvl(To_Char(:new.eventsource),'null')||','
||' evaltype='||Nvl(To_Char(:new.evaltype),'null')||','
||' status='||Nvl(To_Char(:new.status),'null') || ' where  actionid='''||:old.actionid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_actions after delete on zabbix.actions for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.actions '
 || ' where  actionid='''||:old.actionid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_operations after insert on zabbix.operations for each row
begin
Replicate2MySQL.RunSQL
(
'insert into operations (operationid,actionid,operationtype,object,objectid,shortdata,longdata) values ('
||Nvl(To_Char(:new.operationid),'null')||','
||Nvl(To_Char(:new.actionid),'null')||','
||Nvl(To_Char(:new.operationtype),'null')||','
||Nvl(To_Char(:new.object),'null')||','
||Nvl(To_Char(:new.objectid),'null')||','
||''''||:new.shortdata||''''||','
||''''||:new.longdata||''''
||')'
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
||' shortdata='''||:new.shortdata||''''||','
||' longdata='''||:new.longdata||'''' || ' where  operationid='''||:old.operationid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_operations after delete on zabbix.operations for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.operations '
 || ' where  operationid='''||:old.operationid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_applications after insert on zabbix.applications for each row
begin
Replicate2MySQL.RunSQL
(
'insert into applications (applicationid,hostid,name,templateid) values ('
||Nvl(To_Char(:new.applicationid),'null')||','
||Nvl(To_Char(:new.hostid),'null')||','
||''''||:new.name||''''||','
||Nvl(To_Char(:new.templateid),'null')
||')'
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
||' name='''||:new.name||''''||','
||' templateid='||Nvl(To_Char(:new.templateid),'null') || ' where  applicationid='''||:old.applicationid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_applications after delete on zabbix.applications for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.applications '
 || ' where  applicationid='''||:old.applicationid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_auditlog after insert on zabbix.auditlog for each row
begin
Replicate2MySQL.RunSQL
(
'insert into auditlog (auditid,userid,clock,action,resourcetype,details) values ('
||Nvl(To_Char(:new.auditid),'null')||','
||Nvl(To_Char(:new.userid),'null')||','
||Nvl(To_Char(:new.clock),'null')||','
||Nvl(To_Char(:new.action),'null')||','
||Nvl(To_Char(:new.resourcetype),'null')||','
||''''||:new.details||''''
||')'
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
||' details='''||:new.details||'''' || ' where  auditid='''||:old.auditid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_auditlog after delete on zabbix.auditlog for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.auditlog '
 || ' where  auditid='''||:old.auditid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_conditions after insert on zabbix.conditions for each row
begin
Replicate2MySQL.RunSQL
(
'insert into conditions (conditionid,actionid,conditiontype,operator,value) values ('
||Nvl(To_Char(:new.conditionid),'null')||','
||Nvl(To_Char(:new.actionid),'null')||','
||Nvl(To_Char(:new.conditiontype),'null')||','
||Nvl(To_Char(:new.operator),'null')||','
||''''||:new.value||''''
||')'
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
||' value='''||:new.value||'''' || ' where  conditionid='''||:old.conditionid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_conditions after delete on zabbix.conditions for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.conditions '
 || ' where  conditionid='''||:old.conditionid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_config after insert on zabbix.config for each row
begin
Replicate2MySQL.RunSQL
(
'insert into config (configid,alert_history,event_history,refresh_unsupported,work_period,alert_usrgrpid) values ('
||Nvl(To_Char(:new.configid),'null')||','
||Nvl(To_Char(:new.alert_history),'null')||','
||Nvl(To_Char(:new.event_history),'null')||','
||Nvl(To_Char(:new.refresh_unsupported),'null')||','
||''''||:new.work_period||''''||','
||Nvl(To_Char(:new.alert_usrgrpid),'null')
||')'
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
||' work_period='''||:new.work_period||''''||','
||' alert_usrgrpid='||Nvl(To_Char(:new.alert_usrgrpid),'null') || ' where  configid='''||:old.configid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_config after delete on zabbix.config for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.config '
 || ' where  configid='''||:old.configid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_functions after insert on zabbix.functions for each row
begin
Replicate2MySQL.RunSQL
(
'insert into functions (functionid,itemid,triggerid,lastvalue,function,parameter) values ('
||Nvl(To_Char(:new.functionid),'null')||','
||Nvl(To_Char(:new.itemid),'null')||','
||Nvl(To_Char(:new.triggerid),'null')||','
||''''||:new.lastvalue||''''||','
||''''||:new.function||''''||','
||''''||:new.parameter||''''
||')'
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
||' lastvalue='''||:new.lastvalue||''''||','
||' function='''||:new.function||''''||','
||' parameter='''||:new.parameter||'''' || ' where  functionid='''||:old.functionid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_functions after delete on zabbix.functions for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.functions '
 || ' where  functionid='''||:old.functionid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_graphs after insert on zabbix.graphs for each row
begin
Replicate2MySQL.RunSQL
(
'insert into graphs (graphid,name,width,height,yaxistype,yaxismin,yaxismax,templateid,show_work_period,show_triggers,graphtype) values ('
||Nvl(To_Char(:new.graphid),'null')||','
||''''||:new.name||''''||','
||Nvl(To_Char(:new.width),'null')||','
||Nvl(To_Char(:new.height),'null')||','
||Nvl(To_Char(:new.yaxistype),'null')||','
||Nvl(To_Char(:new.yaxismin),'null')||','
||Nvl(To_Char(:new.yaxismax),'null')||','
||Nvl(To_Char(:new.templateid),'null')||','
||Nvl(To_Char(:new.show_work_period),'null')||','
||Nvl(To_Char(:new.show_triggers),'null')||','
||Nvl(To_Char(:new.graphtype),'null')
||')'
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
||' name='''||:new.name||''''||','
||' width='||Nvl(To_Char(:new.width),'null')||','
||' height='||Nvl(To_Char(:new.height),'null')||','
||' yaxistype='||Nvl(To_Char(:new.yaxistype),'null')||','
||' yaxismin='||Nvl(To_Char(:new.yaxismin),'null')||','
||' yaxismax='||Nvl(To_Char(:new.yaxismax),'null')||','
||' templateid='||Nvl(To_Char(:new.templateid),'null')||','
||' show_work_period='||Nvl(To_Char(:new.show_work_period),'null')||','
||' show_triggers='||Nvl(To_Char(:new.show_triggers),'null')||','
||' graphtype='||Nvl(To_Char(:new.graphtype),'null') || ' where  graphid='''||:old.graphid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_graphs after delete on zabbix.graphs for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.graphs '
 || ' where  graphid='''||:old.graphid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_graphs_items after insert on zabbix.graphs_items for each row
begin
Replicate2MySQL.RunSQL
(
'insert into graphs_items (gitemid,graphid,itemid,drawtype,sortorder,color,yaxisside,calc_fnc,type,periods_cnt) values ('
||Nvl(To_Char(:new.gitemid),'null')||','
||Nvl(To_Char(:new.graphid),'null')||','
||Nvl(To_Char(:new.itemid),'null')||','
||Nvl(To_Char(:new.drawtype),'null')||','
||Nvl(To_Char(:new.sortorder),'null')||','
||''''||:new.color||''''||','
||Nvl(To_Char(:new.yaxisside),'null')||','
||Nvl(To_Char(:new.calc_fnc),'null')||','
||Nvl(To_Char(:new.type),'null')||','
||Nvl(To_Char(:new.periods_cnt),'null')
||')'
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
||' color='''||:new.color||''''||','
||' yaxisside='||Nvl(To_Char(:new.yaxisside),'null')||','
||' calc_fnc='||Nvl(To_Char(:new.calc_fnc),'null')||','
||' type='||Nvl(To_Char(:new.type),'null')||','
||' periods_cnt='||Nvl(To_Char(:new.periods_cnt),'null') || ' where  gitemid='''||:old.gitemid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_graphs_items after delete on zabbix.graphs_items for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.graphs_items '
 || ' where  gitemid='''||:old.gitemid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_groups after insert on zabbix.groups for each row
begin
Replicate2MySQL.RunSQL
(
'insert into groups (groupid,name) values ('
||Nvl(To_Char(:new.groupid),'null')||','
||''''||:new.name||''''
||')'
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
||' name='''||:new.name||'''' || ' where  groupid='''||:old.groupid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_groups after delete on zabbix.groups for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.groups '
 || ' where  groupid='''||:old.groupid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_help_items after insert on zabbix.help_items for each row
begin
Replicate2MySQL.RunSQL
(
'insert into help_items (itemtype,key_,description) values ('
||Nvl(To_Char(:new.itemtype),'null')||','
||''''||:new.key_||''''||','
||''''||:new.description||''''
||')'
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
||' key_='''||:new.key_||''''||','
||' description='''||:new.description||'''' || ' where  itemtype='''||:old.itemtype||'''' || ' and  key_='''||:old.key_||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_help_items after delete on zabbix.help_items for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.help_items '
 || ' where  itemtype='''||:old.itemtype||'''' || ' and  key_='''||:old.key_||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_hosts after insert on zabbix.hosts for each row
begin
Replicate2MySQL.RunSQL
(
'insert into hosts (hostid,host,dns,useip,ip,port,status,disable_until,error,available,errors_from,siteid) values ('
||Nvl(To_Char(:new.hostid),'null')||','
||''''||:new.host||''''||','
||''''||:new.dns||''''||','
||Nvl(To_Char(:new.useip),'null')||','
||''''||:new.ip||''''||','
||Nvl(To_Char(:new.port),'null')||','
||Nvl(To_Char(:new.status),'null')||','
||Nvl(To_Char(:new.disable_until),'null')||','
||''''||:new.error||''''||','
||Nvl(To_Char(:new.available),'null')||','
||Nvl(To_Char(:new.errors_from),'null')||','
||Nvl(To_Char(:new.siteid),'null')
||')'
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
||' host='''||:new.host||''''||','
||' dns='''||:new.dns||''''||','
||' useip='||Nvl(To_Char(:new.useip),'null')||','
||' ip='''||:new.ip||''''||','
||' port='||Nvl(To_Char(:new.port),'null')||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' disable_until='||Nvl(To_Char(:new.disable_until),'null')||','
||' error='''||:new.error||''''||','
||' available='||Nvl(To_Char(:new.available),'null')||','
||' errors_from='||Nvl(To_Char(:new.errors_from),'null')||','
||' siteid='||Nvl(To_Char(:new.siteid),'null') || ' where  hostid='''||:old.hostid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_hosts after delete on zabbix.hosts for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.hosts '
 || ' where  hostid='''||:old.hostid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_sites after insert on zabbix.sites for each row
begin
Replicate2MySQL.RunSQL
(
'insert into sites (siteid,name,description) values ('
||Nvl(To_Char(:new.siteid),'null')||','
||''''||:new.name||''''||','
||''''||:new.description||''''
||')'
);
end;
/
show errors
create or replace trigger zabbix.trau_sites after update on zabbix.sites for each row
begin
Replicate2MySQL.RunSQL
(
'update zabbix.sites set '
||' siteid='||Nvl(To_Char(:new.siteid),'null')||','
||' name='''||:new.name||''''||','
||' description='''||:new.description||'''' || ' where  siteid='''||:old.siteid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_sites after delete on zabbix.sites for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.sites '
 || ' where  siteid='''||:old.siteid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_hosts_groups after insert on zabbix.hosts_groups for each row
begin
Replicate2MySQL.RunSQL
(
'insert into hosts_groups (hostgroupid,hostid,groupid) values ('
||Nvl(To_Char(:new.hostgroupid),'null')||','
||Nvl(To_Char(:new.hostid),'null')||','
||Nvl(To_Char(:new.groupid),'null')
||')'
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
||' groupid='||Nvl(To_Char(:new.groupid),'null') || ' where  hostgroupid='''||:old.hostgroupid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_hosts_groups after delete on zabbix.hosts_groups for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.hosts_groups '
 || ' where  hostgroupid='''||:old.hostgroupid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_hosts_profiles after insert on zabbix.hosts_profiles for each row
begin
Replicate2MySQL.RunSQL
(
'insert into hosts_profiles (hostid,devicetype,name,os,serialno,tag,macaddress,hardware,software,contact,location,notes) values ('
||Nvl(To_Char(:new.hostid),'null')||','
||''''||:new.devicetype||''''||','
||''''||:new.name||''''||','
||''''||:new.os||''''||','
||''''||:new.serialno||''''||','
||''''||:new.tag||''''||','
||''''||:new.macaddress||''''||','
||''''||:new.hardware||''''||','
||''''||:new.software||''''||','
||''''||:new.contact||''''||','
||''''||:new.location||''''||','
||''''||:new.notes||''''
||')'
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
||' devicetype='''||:new.devicetype||''''||','
||' name='''||:new.name||''''||','
||' os='''||:new.os||''''||','
||' serialno='''||:new.serialno||''''||','
||' tag='''||:new.tag||''''||','
||' macaddress='''||:new.macaddress||''''||','
||' hardware='''||:new.hardware||''''||','
||' software='''||:new.software||''''||','
||' contact='''||:new.contact||''''||','
||' location='''||:new.location||''''||','
||' notes='''||:new.notes||'''' || ' where  hostid='''||:old.hostid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_hosts_profiles after delete on zabbix.hosts_profiles for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.hosts_profiles '
 || ' where  hostid='''||:old.hostid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_hosts_templates after insert on zabbix.hosts_templates for each row
begin
Replicate2MySQL.RunSQL
(
'insert into hosts_templates (hosttemplateid,hostid,templateid) values ('
||Nvl(To_Char(:new.hosttemplateid),'null')||','
||Nvl(To_Char(:new.hostid),'null')||','
||Nvl(To_Char(:new.templateid),'null')
||')'
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
||' templateid='||Nvl(To_Char(:new.templateid),'null') || ' where  hosttemplateid='''||:old.hosttemplateid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_hosts_templates after delete on zabbix.hosts_templates for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.hosts_templates '
 || ' where  hosttemplateid='''||:old.hosttemplateid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_housekeeper after insert on zabbix.housekeeper for each row
begin
Replicate2MySQL.RunSQL
(
'insert into housekeeper (housekeeperid,tablename,field,value) values ('
||Nvl(To_Char(:new.housekeeperid),'null')||','
||''''||:new.tablename||''''||','
||''''||:new.field||''''||','
||Nvl(To_Char(:new.value),'null')
||')'
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
||' tablename='''||:new.tablename||''''||','
||' field='''||:new.field||''''||','
||' value='||Nvl(To_Char(:new.value),'null') || ' where  housekeeperid='''||:old.housekeeperid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_housekeeper after delete on zabbix.housekeeper for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.housekeeper '
 || ' where  housekeeperid='''||:old.housekeeperid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_items after insert on zabbix.items for each row
begin
Replicate2MySQL.RunSQL
(
'insert into items (itemid,type,snmp_community,snmp_oid,snmp_port,hostid,description,key_,delay,history,trends,nextcheck,lastvalue,lastclock,prevvalue,status,value_type,trapper_hosts,units,multiplier,delta,prevorgvalue,snmpv3_securityname,snmpv3_securitylevel,snmpv3_authpassphrase,snmpv3_privpassphrase,formula,error,lastlogsize,logtimefmt,templateid,valuemapid,delay_flex,params,siteid) values ('
||Nvl(To_Char(:new.itemid),'null')||','
||Nvl(To_Char(:new.type),'null')||','
||''''||:new.snmp_community||''''||','
||''''||:new.snmp_oid||''''||','
||Nvl(To_Char(:new.snmp_port),'null')||','
||Nvl(To_Char(:new.hostid),'null')||','
||''''||:new.description||''''||','
||''''||:new.key_||''''||','
||Nvl(To_Char(:new.delay),'null')||','
||Nvl(To_Char(:new.history),'null')||','
||Nvl(To_Char(:new.trends),'null')||','
||Nvl(To_Char(:new.nextcheck),'null')||','
||''''||:new.lastvalue||''''||','
||Nvl(To_Char(:new.lastclock),'null')||','
||''''||:new.prevvalue||''''||','
||Nvl(To_Char(:new.status),'null')||','
||Nvl(To_Char(:new.value_type),'null')||','
||''''||:new.trapper_hosts||''''||','
||''''||:new.units||''''||','
||Nvl(To_Char(:new.multiplier),'null')||','
||Nvl(To_Char(:new.delta),'null')||','
||''''||:new.prevorgvalue||''''||','
||''''||:new.snmpv3_securityname||''''||','
||Nvl(To_Char(:new.snmpv3_securitylevel),'null')||','
||''''||:new.snmpv3_authpassphrase||''''||','
||''''||:new.snmpv3_privpassphrase||''''||','
||''''||:new.formula||''''||','
||''''||:new.error||''''||','
||Nvl(To_Char(:new.lastlogsize),'null')||','
||''''||:new.logtimefmt||''''||','
||Nvl(To_Char(:new.templateid),'null')||','
||Nvl(To_Char(:new.valuemapid),'null')||','
||''''||:new.delay_flex||''''||','
||''''||:new.params||''''||','
||Nvl(To_Char(:new.siteid),'null')
||')'
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
||' snmp_community='''||:new.snmp_community||''''||','
||' snmp_oid='''||:new.snmp_oid||''''||','
||' snmp_port='||Nvl(To_Char(:new.snmp_port),'null')||','
||' hostid='||Nvl(To_Char(:new.hostid),'null')||','
||' description='''||:new.description||''''||','
||' key_='''||:new.key_||''''||','
||' delay='||Nvl(To_Char(:new.delay),'null')||','
||' history='||Nvl(To_Char(:new.history),'null')||','
||' trends='||Nvl(To_Char(:new.trends),'null')||','
||' nextcheck='||Nvl(To_Char(:new.nextcheck),'null')||','
||' lastvalue='''||:new.lastvalue||''''||','
||' lastclock='||Nvl(To_Char(:new.lastclock),'null')||','
||' prevvalue='''||:new.prevvalue||''''||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' value_type='||Nvl(To_Char(:new.value_type),'null')||','
||' trapper_hosts='''||:new.trapper_hosts||''''||','
||' units='''||:new.units||''''||','
||' multiplier='||Nvl(To_Char(:new.multiplier),'null')||','
||' delta='||Nvl(To_Char(:new.delta),'null')||','
||' prevorgvalue='''||:new.prevorgvalue||''''||','
||' snmpv3_securityname='''||:new.snmpv3_securityname||''''||','
||' snmpv3_securitylevel='||Nvl(To_Char(:new.snmpv3_securitylevel),'null')||','
||' snmpv3_authpassphrase='''||:new.snmpv3_authpassphrase||''''||','
||' snmpv3_privpassphrase='''||:new.snmpv3_privpassphrase||''''||','
||' formula='''||:new.formula||''''||','
||' error='''||:new.error||''''||','
||' lastlogsize='||Nvl(To_Char(:new.lastlogsize),'null')||','
||' logtimefmt='''||:new.logtimefmt||''''||','
||' templateid='||Nvl(To_Char(:new.templateid),'null')||','
||' valuemapid='||Nvl(To_Char(:new.valuemapid),'null')||','
||' delay_flex='''||:new.delay_flex||''''||','
||' params='''||:new.params||''''||','
||' siteid='||Nvl(To_Char(:new.siteid),'null') || ' where  itemid='''||:old.itemid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_items after delete on zabbix.items for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.items '
 || ' where  itemid='''||:old.itemid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_items_applications after insert on zabbix.items_applications for each row
begin
Replicate2MySQL.RunSQL
(
'insert into items_applications (itemappid,applicationid,itemid) values ('
||Nvl(To_Char(:new.itemappid),'null')||','
||Nvl(To_Char(:new.applicationid),'null')||','
||Nvl(To_Char(:new.itemid),'null')
||')'
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
||' itemid='||Nvl(To_Char(:new.itemid),'null') || ' where  itemappid='''||:old.itemappid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_items_applications after delete on zabbix.items_applications for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.items_applications '
 || ' where  itemappid='''||:old.itemappid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_mappings after insert on zabbix.mappings for each row
begin
Replicate2MySQL.RunSQL
(
'insert into mappings (mappingid,valuemapid,value,newvalue) values ('
||Nvl(To_Char(:new.mappingid),'null')||','
||Nvl(To_Char(:new.valuemapid),'null')||','
||''''||:new.value||''''||','
||''''||:new.newvalue||''''
||')'
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
||' value='''||:new.value||''''||','
||' newvalue='''||:new.newvalue||'''' || ' where  mappingid='''||:old.mappingid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_mappings after delete on zabbix.mappings for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.mappings '
 || ' where  mappingid='''||:old.mappingid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_media after insert on zabbix.media for each row
begin
Replicate2MySQL.RunSQL
(
'insert into media (mediaid,userid,mediatypeid,sendto,active,severity,period) values ('
||Nvl(To_Char(:new.mediaid),'null')||','
||Nvl(To_Char(:new.userid),'null')||','
||Nvl(To_Char(:new.mediatypeid),'null')||','
||''''||:new.sendto||''''||','
||Nvl(To_Char(:new.active),'null')||','
||Nvl(To_Char(:new.severity),'null')||','
||''''||:new.period||''''
||')'
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
||' sendto='''||:new.sendto||''''||','
||' active='||Nvl(To_Char(:new.active),'null')||','
||' severity='||Nvl(To_Char(:new.severity),'null')||','
||' period='''||:new.period||'''' || ' where  mediaid='''||:old.mediaid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_media after delete on zabbix.media for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.media '
 || ' where  mediaid='''||:old.mediaid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_media_type after insert on zabbix.media_type for each row
begin
Replicate2MySQL.RunSQL
(
'insert into media_type (mediatypeid,type,description,smtp_server,smtp_helo,smtp_email,exec_path,gsm_modem,username,passwd) values ('
||Nvl(To_Char(:new.mediatypeid),'null')||','
||Nvl(To_Char(:new.type),'null')||','
||''''||:new.description||''''||','
||''''||:new.smtp_server||''''||','
||''''||:new.smtp_helo||''''||','
||''''||:new.smtp_email||''''||','
||''''||:new.exec_path||''''||','
||''''||:new.gsm_modem||''''||','
||''''||:new.username||''''||','
||''''||:new.passwd||''''
||')'
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
||' description='''||:new.description||''''||','
||' smtp_server='''||:new.smtp_server||''''||','
||' smtp_helo='''||:new.smtp_helo||''''||','
||' smtp_email='''||:new.smtp_email||''''||','
||' exec_path='''||:new.exec_path||''''||','
||' gsm_modem='''||:new.gsm_modem||''''||','
||' username='''||:new.username||''''||','
||' passwd='''||:new.passwd||'''' || ' where  mediatypeid='''||:old.mediatypeid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_media_type after delete on zabbix.media_type for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.media_type '
 || ' where  mediatypeid='''||:old.mediatypeid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_profiles after insert on zabbix.profiles for each row
begin
Replicate2MySQL.RunSQL
(
'insert into profiles (profileid,userid,idx,value,valuetype) values ('
||Nvl(To_Char(:new.profileid),'null')||','
||Nvl(To_Char(:new.userid),'null')||','
||''''||:new.idx||''''||','
||''''||:new.value||''''||','
||Nvl(To_Char(:new.valuetype),'null')
||')'
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
||' idx='''||:new.idx||''''||','
||' value='''||:new.value||''''||','
||' valuetype='||Nvl(To_Char(:new.valuetype),'null') || ' where  profileid='''||:old.profileid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_profiles after delete on zabbix.profiles for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.profiles '
 || ' where  profileid='''||:old.profileid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_rights after insert on zabbix.rights for each row
begin
Replicate2MySQL.RunSQL
(
'insert into rights (rightid,groupid,type,permission,id) values ('
||Nvl(To_Char(:new.rightid),'null')||','
||Nvl(To_Char(:new.groupid),'null')||','
||Nvl(To_Char(:new.type),'null')||','
||Nvl(To_Char(:new.permission),'null')||','
||Nvl(To_Char(:new.id),'null')
||')'
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
||' id='||Nvl(To_Char(:new.id),'null') || ' where  rightid='''||:old.rightid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_rights after delete on zabbix.rights for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.rights '
 || ' where  rightid='''||:old.rightid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_screens after insert on zabbix.screens for each row
begin
Replicate2MySQL.RunSQL
(
'insert into screens (screenid,name,hsize,vsize) values ('
||Nvl(To_Char(:new.screenid),'null')||','
||''''||:new.name||''''||','
||Nvl(To_Char(:new.hsize),'null')||','
||Nvl(To_Char(:new.vsize),'null')
||')'
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
||' name='''||:new.name||''''||','
||' hsize='||Nvl(To_Char(:new.hsize),'null')||','
||' vsize='||Nvl(To_Char(:new.vsize),'null') || ' where  screenid='''||:old.screenid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_screens after delete on zabbix.screens for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.screens '
 || ' where  screenid='''||:old.screenid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_screens_items after insert on zabbix.screens_items for each row
begin
Replicate2MySQL.RunSQL
(
'insert into screens_items (screenitemid,screenid,resourcetype,resourceid,width,height,x,y,colspan,rowspan,elements,valign,halign,style,url) values ('
||Nvl(To_Char(:new.screenitemid),'null')||','
||Nvl(To_Char(:new.screenid),'null')||','
||Nvl(To_Char(:new.resourcetype),'null')||','
||Nvl(To_Char(:new.resourceid),'null')||','
||Nvl(To_Char(:new.width),'null')||','
||Nvl(To_Char(:new.height),'null')||','
||Nvl(To_Char(:new.x),'null')||','
||Nvl(To_Char(:new.y),'null')||','
||Nvl(To_Char(:new.colspan),'null')||','
||Nvl(To_Char(:new.rowspan),'null')||','
||Nvl(To_Char(:new.elements),'null')||','
||Nvl(To_Char(:new.valign),'null')||','
||Nvl(To_Char(:new.halign),'null')||','
||Nvl(To_Char(:new.style),'null')||','
||''''||:new.url||''''
||')'
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
||' url='''||:new.url||'''' || ' where  screenitemid='''||:old.screenitemid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_screens_items after delete on zabbix.screens_items for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.screens_items '
 || ' where  screenitemid='''||:old.screenitemid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_services after insert on zabbix.services for each row
begin
Replicate2MySQL.RunSQL
(
'insert into services (serviceid,name,status,algorithm,triggerid,showsla,goodsla,sortorder) values ('
||Nvl(To_Char(:new.serviceid),'null')||','
||''''||:new.name||''''||','
||Nvl(To_Char(:new.status),'null')||','
||Nvl(To_Char(:new.algorithm),'null')||','
||Nvl(To_Char(:new.triggerid),'null')||','
||Nvl(To_Char(:new.showsla),'null')||','
||Nvl(To_Char(:new.goodsla),'null')||','
||Nvl(To_Char(:new.sortorder),'null')
||')'
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
||' name='''||:new.name||''''||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' algorithm='||Nvl(To_Char(:new.algorithm),'null')||','
||' triggerid='||Nvl(To_Char(:new.triggerid),'null')||','
||' showsla='||Nvl(To_Char(:new.showsla),'null')||','
||' goodsla='||Nvl(To_Char(:new.goodsla),'null')||','
||' sortorder='||Nvl(To_Char(:new.sortorder),'null') || ' where  serviceid='''||:old.serviceid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_services after delete on zabbix.services for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.services '
 || ' where  serviceid='''||:old.serviceid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_service_alarms after insert on zabbix.service_alarms for each row
begin
Replicate2MySQL.RunSQL
(
'insert into service_alarms (servicealarmid,serviceid,clock,value) values ('
||Nvl(To_Char(:new.servicealarmid),'null')||','
||Nvl(To_Char(:new.serviceid),'null')||','
||Nvl(To_Char(:new.clock),'null')||','
||Nvl(To_Char(:new.value),'null')
||')'
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
||' value='||Nvl(To_Char(:new.value),'null') || ' where  servicealarmid='''||:old.servicealarmid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_service_alarms after delete on zabbix.service_alarms for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.service_alarms '
 || ' where  servicealarmid='''||:old.servicealarmid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_services_links after insert on zabbix.services_links for each row
begin
Replicate2MySQL.RunSQL
(
'insert into services_links (linkid,serviceupid,servicedownid,soft) values ('
||Nvl(To_Char(:new.linkid),'null')||','
||Nvl(To_Char(:new.serviceupid),'null')||','
||Nvl(To_Char(:new.servicedownid),'null')||','
||Nvl(To_Char(:new.soft),'null')
||')'
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
||' soft='||Nvl(To_Char(:new.soft),'null') || ' where  linkid='''||:old.linkid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_services_links after delete on zabbix.services_links for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.services_links '
 || ' where  linkid='''||:old.linkid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_sysmaps_links after insert on zabbix.sysmaps_links for each row
begin
Replicate2MySQL.RunSQL
(
'insert into sysmaps_links (linkid,sysmapid,selementid1,selementid2,triggerid,drawtype_off,color_off,drawtype_on,color_on) values ('
||Nvl(To_Char(:new.linkid),'null')||','
||Nvl(To_Char(:new.sysmapid),'null')||','
||Nvl(To_Char(:new.selementid1),'null')||','
||Nvl(To_Char(:new.selementid2),'null')||','
||Nvl(To_Char(:new.triggerid),'null')||','
||Nvl(To_Char(:new.drawtype_off),'null')||','
||''''||:new.color_off||''''||','
||Nvl(To_Char(:new.drawtype_on),'null')||','
||''''||:new.color_on||''''
||')'
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
||' color_off='''||:new.color_off||''''||','
||' drawtype_on='||Nvl(To_Char(:new.drawtype_on),'null')||','
||' color_on='''||:new.color_on||'''' || ' where  linkid='''||:old.linkid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_sysmaps_links after delete on zabbix.sysmaps_links for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.sysmaps_links '
 || ' where  linkid='''||:old.linkid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_sysmaps_elements after insert on zabbix.sysmaps_elements for each row
begin
Replicate2MySQL.RunSQL
(
'insert into sysmaps_elements (selementid,sysmapid,elementid,elementtype,iconid_off,iconid_on,iconid_unknown,label,label_location,x,y,url) values ('
||Nvl(To_Char(:new.selementid),'null')||','
||Nvl(To_Char(:new.sysmapid),'null')||','
||Nvl(To_Char(:new.elementid),'null')||','
||Nvl(To_Char(:new.elementtype),'null')||','
||Nvl(To_Char(:new.iconid_off),'null')||','
||Nvl(To_Char(:new.iconid_on),'null')||','
||Nvl(To_Char(:new.iconid_unknown),'null')||','
||''''||:new.label||''''||','
||Nvl(To_Char(:new.label_location),'null')||','
||Nvl(To_Char(:new.x),'null')||','
||Nvl(To_Char(:new.y),'null')||','
||''''||:new.url||''''
||')'
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
||' label='''||:new.label||''''||','
||' label_location='||Nvl(To_Char(:new.label_location),'null')||','
||' x='||Nvl(To_Char(:new.x),'null')||','
||' y='||Nvl(To_Char(:new.y),'null')||','
||' url='''||:new.url||'''' || ' where  selementid='''||:old.selementid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_sysmaps_elements after delete on zabbix.sysmaps_elements for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.sysmaps_elements '
 || ' where  selementid='''||:old.selementid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_sysmaps after insert on zabbix.sysmaps for each row
begin
Replicate2MySQL.RunSQL
(
'insert into sysmaps (sysmapid,name,width,height,backgroundid,label_type,label_location) values ('
||Nvl(To_Char(:new.sysmapid),'null')||','
||''''||:new.name||''''||','
||Nvl(To_Char(:new.width),'null')||','
||Nvl(To_Char(:new.height),'null')||','
||Nvl(To_Char(:new.backgroundid),'null')||','
||Nvl(To_Char(:new.label_type),'null')||','
||Nvl(To_Char(:new.label_location),'null')
||')'
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
||' name='''||:new.name||''''||','
||' width='||Nvl(To_Char(:new.width),'null')||','
||' height='||Nvl(To_Char(:new.height),'null')||','
||' backgroundid='||Nvl(To_Char(:new.backgroundid),'null')||','
||' label_type='||Nvl(To_Char(:new.label_type),'null')||','
||' label_location='||Nvl(To_Char(:new.label_location),'null') || ' where  sysmapid='''||:old.sysmapid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_sysmaps after delete on zabbix.sysmaps for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.sysmaps '
 || ' where  sysmapid='''||:old.sysmapid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_triggers after insert on zabbix.triggers for each row
begin
Replicate2MySQL.RunSQL
(
'insert into triggers (triggerid,expression,description,url,status,value,priority,lastchange,dep_level,comments,error,templateid) values ('
||Nvl(To_Char(:new.triggerid),'null')||','
||''''||:new.expression||''''||','
||''''||:new.description||''''||','
||''''||:new.url||''''||','
||Nvl(To_Char(:new.status),'null')||','
||Nvl(To_Char(:new.value),'null')||','
||Nvl(To_Char(:new.priority),'null')||','
||Nvl(To_Char(:new.lastchange),'null')||','
||Nvl(To_Char(:new.dep_level),'null')||','
||''''||:new.comments||''''||','
||''''||:new.error||''''||','
||Nvl(To_Char(:new.templateid),'null')
||')'
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
||' expression='''||:new.expression||''''||','
||' description='''||:new.description||''''||','
||' url='''||:new.url||''''||','
||' status='||Nvl(To_Char(:new.status),'null')||','
||' value='||Nvl(To_Char(:new.value),'null')||','
||' priority='||Nvl(To_Char(:new.priority),'null')||','
||' lastchange='||Nvl(To_Char(:new.lastchange),'null')||','
||' dep_level='||Nvl(To_Char(:new.dep_level),'null')||','
||' comments='''||:new.comments||''''||','
||' error='''||:new.error||''''||','
||' templateid='||Nvl(To_Char(:new.templateid),'null') || ' where  triggerid='''||:old.triggerid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_triggers after delete on zabbix.triggers for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.triggers '
 || ' where  triggerid='''||:old.triggerid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_trigger_depends after insert on zabbix.trigger_depends for each row
begin
Replicate2MySQL.RunSQL
(
'insert into trigger_depends (triggerdepid,triggerid_down,triggerid_up) values ('
||Nvl(To_Char(:new.triggerdepid),'null')||','
||Nvl(To_Char(:new.triggerid_down),'null')||','
||Nvl(To_Char(:new.triggerid_up),'null')
||')'
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
||' triggerid_up='||Nvl(To_Char(:new.triggerid_up),'null') || ' where  triggerdepid='''||:old.triggerdepid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_trigger_depends after delete on zabbix.trigger_depends for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.trigger_depends '
 || ' where  triggerdepid='''||:old.triggerdepid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_users after insert on zabbix.users for each row
begin
Replicate2MySQL.RunSQL
(
'insert into users (userid,alias,name,surname,passwd,url,autologout,lang,refresh,type) values ('
||Nvl(To_Char(:new.userid),'null')||','
||''''||:new.alias||''''||','
||''''||:new.name||''''||','
||''''||:new.surname||''''||','
||''''||:new.passwd||''''||','
||''''||:new.url||''''||','
||Nvl(To_Char(:new.autologout),'null')||','
||''''||:new.lang||''''||','
||Nvl(To_Char(:new.refresh),'null')||','
||Nvl(To_Char(:new.type),'null')
||')'
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
||' alias='''||:new.alias||''''||','
||' name='''||:new.name||''''||','
||' surname='''||:new.surname||''''||','
||' passwd='''||:new.passwd||''''||','
||' url='''||:new.url||''''||','
||' autologout='||Nvl(To_Char(:new.autologout),'null')||','
||' lang='''||:new.lang||''''||','
||' refresh='||Nvl(To_Char(:new.refresh),'null')||','
||' type='||Nvl(To_Char(:new.type),'null') || ' where  userid='''||:old.userid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_users after delete on zabbix.users for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.users '
 || ' where  userid='''||:old.userid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_usrgrp after insert on zabbix.usrgrp for each row
begin
Replicate2MySQL.RunSQL
(
'insert into usrgrp (usrgrpid,name) values ('
||Nvl(To_Char(:new.usrgrpid),'null')||','
||''''||:new.name||''''
||')'
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
||' name='''||:new.name||'''' || ' where  usrgrpid='''||:old.usrgrpid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_usrgrp after delete on zabbix.usrgrp for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.usrgrp '
 || ' where  usrgrpid='''||:old.usrgrpid||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_users_groups after insert on zabbix.users_groups for each row
begin
Replicate2MySQL.RunSQL
(
'insert into users_groups (id,usrgrpid,userid) values ('
||Nvl(To_Char(:new.id),'null')||','
||Nvl(To_Char(:new.usrgrpid),'null')||','
||Nvl(To_Char(:new.userid),'null')
||')'
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
||' userid='||Nvl(To_Char(:new.userid),'null') || ' where  id='''||:old.id||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_users_groups after delete on zabbix.users_groups for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.users_groups '
 || ' where  id='''||:old.id||'''' 
);
end;
/
show errors
create or replace trigger zabbix.trai_valuemaps after insert on zabbix.valuemaps for each row
begin
Replicate2MySQL.RunSQL
(
'insert into valuemaps (valuemapid,name) values ('
||Nvl(To_Char(:new.valuemapid),'null')||','
||''''||:new.name||''''
||')'
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
||' name='''||:new.name||'''' || ' where  valuemapid='''||:old.valuemapid||'''' );
end;
/
show errors
create or replace trigger zabbix.trad_valuemaps after delete on zabbix.valuemaps for each row
begin
Replicate2MySQL.RunSQL
(
'delete from zabbix.valuemaps '
 || ' where  valuemapid='''||:old.valuemapid||'''' 
);
end;
/
show errors
