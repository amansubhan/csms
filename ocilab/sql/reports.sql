set echo on
define host_command = '&&1'

 
-- Specify default values for the report name prefix and extension.
-- These would be ignored, if the variable 'report_name' is defined.
define report_name_prefix    = 'addmrpt_';
define report_name_extension = '.txt';

--  Set up the snapshot-related binds

variable bid        number;
variable eid        number;
begin
  :bid       :=  DBMS_WORKLOAD_REPOSITORY.CREATE_SNAPSHOT();
end;
/

HOST &&host_command 
undefine host_command

begin
  :eid       :=  DBMS_WORKLOAD_REPOSITORY.CREATE_SNAPSHOT();
end;
/

-- Get the current database/instance information - this will be used
-- later in the report along with bid, eid to lookup snapshots

set heading on echo off feedback off verify off underline on timing off;

column inst_num  heading "Inst Num"  new_value inst_num  format 99999;
column inst_name heading "Instance"  new_value inst_name format a12;
column db_name   heading "DB Name"   new_value db_name   format a12;
column dbid      heading "DB Id"     new_value dbid      format 9999999999 just c;

prompt
prompt Current Instance
prompt ~~~~~~~~~~~~~~~~

select d.dbid            dbid
     , d.name            db_name
     , i.instance_number inst_num
     , i.instance_name   inst_name
  from v$database d,
       v$instance i;


--  Set up the binds for dbid and instance_number

variable dbid       number;
variable inst_num   number;
begin
  :dbid      :=  &dbid;
  :inst_num  :=  &inst_num;
end;
/

set pagesize 0;
set heading off echo off feedback off verify off;

variable task_name  varchar2(40);

prompt
prompt
prompt Running the ADDM analysis on the specified pair of snapshots ...
prompt

begin
  declare
    id number;
    name varchar2(100);
    descr varchar2(500);
  BEGIN
     name := '';
     descr := 'ADDM run: snapshots [' || :bid || ', '
              || :eid || '], instance ' || :inst_num
              || ', database id ' || :dbid;

     dbms_advisor.create_task('ADDM',id,name,descr,null);

     :task_name := name;

     -- set time window
     dbms_advisor.set_task_parameter(name, 'START_SNAPSHOT', :bid);
     dbms_advisor.set_task_parameter(name, 'END_SNAPSHOT', :eid);

     -- set instance number
     dbms_advisor.set_task_parameter(name, 'INSTANCE', :inst_num);

     -- set dbid
     dbms_advisor.set_task_parameter(name, 'DB_ID', :dbid);

     -- execute task
     dbms_advisor.execute_task(name);

  end;
end;
/

prompt
prompt Generating the ADDM report for this analysis ...
prompt
prompt

set linesize 80
spool &&report_name._addm.txt;

set long 1000000 pagesize 0 longchunksize 1000
column get_clob format a80

select dbms_advisor.get_task_report(:task_name, 'TEXT', 'TYPICAL')
from   sys.dual;

spool off

-- generate AWR report
set veri off;
set feedback off;

variable rpt_options number;
-- option settings
define NO_OPTIONS   = 0;
define ENABLE_ADDM  = 8;

-- set the report_options. To see the ADDM sections,
-- set the rpt_options to the ENABLE_ADDM constant.
begin
  :rpt_options := &NO_OPTIONS;
end;
/

define report_type = 'html';

set termout off;
-- set report function name and line size
column fn_name new_value fn_name noprint;
select 'awr_report_text' fn_name from dual where lower('&report_type') = 'text';
select 'awr_report_html' fn_name from dual where lower('&report_type') <> 'text';

column lnsz new_value lnsz noprint;
select '80' lnsz from dual where lower('&report_type') = 'text';
select '1500' lnsz from dual where lower('&report_type') <> 'text';

set linesize &lnsz;
set termout on;

spool &&report_name._awr.html;
set termout off;
-- call the table function to generate the report
select output from table(dbms_workload_repository.&fn_name( :dbid,
                                                            :inst_num,
                                                            :bid, :eid,
                                                            :rpt_options ));

spool off;
set termout on;
prompt
prompt End of ADDM and AWR Reports

undefine rpt_options
undefine report_type
undefine lnsz
undefine NO_OPTIONS;
undefine ENABLE_ADDM;

set termout off;

clear columns sql;
ttitle off;
btitle off;
repfooter off;

-- Restore the default SQL*Plus settings
set pagesize 14;
set heading on echo off feedback 6 verify on underline on timing off;
set long 80 longchunksize 80
set termout on

-- Undefine all variables that need to be set before calling this script
undefine dbid
undefine inst_num
undefine num_days
undefine begin_snap
undefine end_snap

-- Undefine report_name created in awrinput.sql
undefine report_name

undefine report_name_prefix
undefine report_name_extension

-- Undefine substitution variables
undefine dbid
undefine inst_num
undefine begin_snap
undefine end_snap
undefine db_name
undefine inst_name

whenever sqlerror continue;


