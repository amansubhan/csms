
CONNECT ocihol/welcome;

-- Create myemp table
DROP TABLE myemp;
CREATE TABLE  myemp (
    empno     number(4) primary key,
    ename     varchar2(10),
    job       varchar2(9),
    mgr       number(4),
    hiredate  date,
    sal       number(7,2),
    comm      number(7,2),
    deptno    number(2));

-- Insert rows into myemp table
INSERT INTO MYEMP VALUES (100, 'SMITH', 'CLERK',    7902, TO_DATE('17-DEC-1980', 'DD-MON-YYYY'), 800, NULL, 20);
INSERT INTO MYEMP VALUES (101, 'ALLEN', 'SALESMAN', 7698, TO_DATE('20-FEB-1981', 'DD-MON-YYYY'), 1600, 300, 30);
INSERT INTO MYEMP VALUES (102, 'WARD',  'SALESMAN', 7698, TO_DATE('22-FEB-1981', 'DD-MON-YYYY'), 1250, 500, 30);
INSERT INTO MYEMP VALUES (103, 'JONES', 'MANAGER',  7839, TO_DATE('2-APR-1981',  'DD-MON-YYYY'), 2975, NULL, 20);
INSERT INTO MYEMP VALUES (104, 'MARTIN', 'SALESMAN', 7698,TO_DATE('28-SEP-1981', 'DD-MON-YYYY'), 1250, 1400, 30);
INSERT INTO MYEMP VALUES (105, 'BLAKE', 'MANAGER', 7839,TO_DATE('1-MAY-1981', 'DD-MON-YYYY'), 2850, NULL, 30);
INSERT INTO MYEMP VALUES (106, 'CLARK', 'MANAGER', 7839,TO_DATE('9-JUN-1981', 'DD-MON-YYYY'), 2450, NULL, 10);
INSERT INTO MYEMP VALUES (107, 'SCOTT', 'ANALYST', 7566,TO_DATE('09-DEC-1982', 'DD-MON-YYYY'), 3000, NULL, 20);
INSERT INTO MYEMP VALUES (108, 'KING', 'PRESIDENT', NULL,TO_DATE('17-NOV-1981', 'DD-MON-YYYY'), 5000, NULL, 10);
INSERT INTO MYEMP VALUES (109, 'TURNER', 'SALESMAN', 7698,TO_DATE('8-SEP-1981', 'DD-MON-YYYY'), 1500, 0, 30);
INSERT INTO MYEMP VALUES (110, 'ADAMS', 'CLERK', 7788,TO_DATE('12-JAN-1983', 'DD-MON-YYYY'), 1100, NULL, 20);
INSERT INTO MYEMP VALUES (111, 'JAMES', 'CLERK', 7698,TO_DATE('3-DEC-1981', 'DD-MON-YYYY'), 950, NULL, 30);
INSERT INTO MYEMP VALUES (112, 'FORD', 'ANALYST', 7566,TO_DATE('3-DEC-1981', 'DD-MON-YYYY'), 3000, NULL, 20);
INSERT INTO MYEMP VALUES (113, 'MILLER', 'CLERK', 7782,TO_DATE('23-JAN-1982', 'DD-MON-YYYY'), 1300, NULL, 10);

COMMIT; 

-- Create regions table 
DROP TABLE regions; 

CREATE TABLE regions
    ( region_id      NUMBER 
       CONSTRAINT  region_id_nn NOT NULL 
    , region_name    VARCHAR2(25) 
    );


-- Populate regions table
INSERT INTO regions VALUES (1, 'Europe' );

INSERT INTO regions VALUES (2, 'Americas');

INSERT INTO regions VALUES (3, 'Asia') ;

INSERT INTO regions VALUES (4, 'Middle East and Africa');

COMMIT;


-- STAGE 5
-- annotate table for result caching
-- Do below part of stage5 in separate script so can see 
-- performance difference in AWR reports.
-- ALTER TABLE regions result_cache (mode force);

-- create a view to show result caching on views
CREATE OR REPLACE VIEW empview as select * from myemp;

-- STAGE 6 
-- the PLSQL update to salary column requires higher precision 
-- alter table so salaries can be raised 
ALTER TABLE myemp modify sal number (10,2);

-- Create anonymous plsql procedure to show using PLSQL within OCI 
create or replace procedure 
RAISE_SALARY( empNumber IN NUMBER, salIncrease IN NUMBER, newSalary OUT NUMBER)
IS
BEGIN
    update myemp set sal = sal + salIncrease 
      where empno = empNumber returning sal into newSalary;
END;
/

-- This package created to demo ref cursors 
CREATE OR REPLACE PACKAGE emp_demo_pkg AS
  TYPE retCursor  IS REF CURSOR;
  PROCEDURE open_cur (employeeList OUT retCursor, empID IN number);
END emp_demo_pkg;
/
CREATE OR REPLACE PACKAGE BODY emp_demo_pkg AS
  PROCEDURE open_cur (employeeList OUT retCursor, empID IN number) IS
  BEGIN
      OPEN employeeList FOR SELECT empno, ename from 
                    myemp where empno > empID order by empno;
  END open_cur;
END emp_demo_pkg;
/

-- STAGE 7 
-- creating a table to show LOB usage
DROP TABLE lob_table;

create table lob_table (lobid number, lobcol clob)
     LOB(lobcol) STORE AS SECUREFILE (TABLESPACE SYSAUX
         COMPRESS low 
         CACHE
    );

QUIT;

