
SET ECHO ON
SET FEEDBACK 1
SET NUMWIDTH 10
SET LINESIZE 80
SET TRIMSPOOL ON
SET TAB OFF
SET PAGESIZE 100

connect system/Oracle1

-- Drop user 'ocihol'.
drop user ocihol cascade;

-- Create user 'ocihol' and grant permissions.
create user ocihol identified by welcome;
grant connect, resource, create procedure, create view to ocihol identified by welcome;

