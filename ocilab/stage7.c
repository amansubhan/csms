/**
 * This program does the following:
 * a) Spawn threads
 * b) Each thread creates a connection and does the workload in a loop.
 *    The loop runs for number of iterations specified in iteration_num.
 * c) As a part of workload each thread executes three SQL statements
 *    a Select with one row, a Select with multiple rows and an Update.
 *
 * stage1:
 *    No Optimizations
 *    Enable binds. Note that this not only improves the performance but
 *    also makes the program secure by avoiding sql injuctions.
 *
 * stage2:
 *    Demonstrating array DML and array fetch (Set fetch size to > 1).
 *
 * stage3:
 *    Use OCI Session Pooling.
 *
 * stage4:
 *    Enable OCI statement caching.
 *
 * stage5:
 *    Enabling OCI client result-set caching.   
 *    Make sure that you enable result-set caching on the server too. 
 *    Examples use query-level hints, table annotations, and 
 *    result caching on queries with views.
 * 
 * stage6:
 *    Demonstrating use of PLSQL stored procedure in, out binds
 *     and PLSQL Ref cursor with prefetching
 * 
 * stage7:
 *    Demonstrating OCI Secure File Lob Access with lob insert, lob write, 
 *     and lob read using prefetching.
 *    
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <oci.h>
#include "helper.h"

#define BUFFERSIZE 256
static text *username = (text *) "ocihol";
static text *apppassword = (text *) "welcome";
static char *connstr = (char *) "";
OCIEnv   *envhp = (OCIEnv *)NULL;
static OCIAuthInfo *authp = NULL;
static OCISPool *spoolhp = NULL;
static char *poolName = NULL;
static ub4   poolNameLen;

static void create_session_pool(OCIEnv *envhp, OCIError *errhp,
                         char **poolName, ub4 *poolNameLenp);

/* This select empno, ename from myemp table or empview view 
   - can take different SQL texts 
*/
static void multirow_fetch_from_emp(OCISvcCtx *svchp, OCIStmt * stmthp, OCIError *errhp, 
				    thdata *pthdata);

static char *MY_DML = (char *) "update myemp set sal = :sal \
                        where empno = :id";

void update_salary(OCISvcCtx *svchp, OCIError *errhp, thdata *pthdata)
{
  OCIBind *bndp1, *bndp2;
  OCIStmt *stmthp;
  int i;
  
  if (verbose_flag)
    printf ("updating salaries \n");
  
   checkerr(errhp,
             OCIStmtPrepare2 (svchp, &stmthp,        /* returned stmt handle */
                           errhp,                            /* error handle */
                           (const OraText *) MY_DML,  /* input statement text*/
                           strlen((char *) MY_DML),        /* length of text */
                            NULL, 0,         /* tagging parameters: optional */
                            OCI_NTV_SYNTAX, OCI_DEFAULT));

   /* do parameter bindings */
   checkerr(errhp, OCIBindByPos(stmthp, &bndp1, errhp, 1,
                            (void *) (pthdata->salaries),
                            (sword) sizeof(*(pthdata->salaries)),
                            SQLT_INT, (void *) NULL, (ub2 *) NULL,
                            (ub2 *) NULL, 0, (ub4 *) NULL, OCI_DEFAULT));

   checkerr(errhp, OCIBindByPos(stmthp, &bndp2, errhp, 2,
                              (void *) (pthdata->id),
                              (sword) sizeof(*(pthdata->id)), SQLT_INT,
                              (void *) NULL, (ub2 *) NULL, (ub2 *) NULL, 0,
                              (ub4 *) NULL, OCI_DEFAULT));

   /* execute the statement and commit */
   checkerr(errhp, OCIStmtExecute(svchp, stmthp, errhp, update_num, 0,
                                 (OCISnapshot *) NULL, (OCISnapshot *) NULL,
                                 OCI_COMMIT_ON_SUCCESS));

   /* release the statement handle */
   checkerr(errhp, OCIStmtRelease(stmthp, errhp,
                                          (OraText *) NULL, 0, OCI_DEFAULT));

   if (verbose_flag)
     printf("updated salaries successfully\n");
}

static char *MY_SELECT = "select region_id, region_name \
                           from regions where region_id = :regionID";

/* To get client result caching, annotate the table regions for result_cache. 
   See stage5.sql 
*/
void query_salary(OCISvcCtx *svchp, OCIError *errhp, thdata *pthdata)
{
  OCIBind *bndp1;
  OCIDefine *defhp1, *defhp2, *defhp3;
  OCIStmt *stmthp;            
  ub4 region_id;
  char region_name[100];
  ub4 num_rows = 1;
  
  if (verbose_flag)
    printf ("demonstrating single row select\n"); 
  
   checkerr(errhp,
           OCIStmtPrepare2 (svchp, &stmthp,          /* returned stmt handle */
                        errhp,                               /* error handle */
                        (const OraText *) MY_SELECT,   /*input statement text*/
                        strlen((char *) MY_SELECT),        /* length of text */
                        NULL, 0,             /* tagging parameters: optional */
                        OCI_NTV_SYNTAX, OCI_DEFAULT));

  /* bind input parameters */
  checkerr(errhp, OCIBindByName(stmthp, &bndp1, errhp, (text *) ":regionID",
                                -1, (void *) &(pthdata->region_id),
                                sizeof(pthdata->region_id),
                                SQLT_INT, (void *) NULL,
                                (ub2 *) NULL, (ub2 *) NULL, 0, (ub4 *) NULL,
                                OCI_DEFAULT));

  /* execute the statement */
  checkerr(errhp, OCIStmtExecute(svchp, stmthp, errhp, 0, 0,
                                 (OCISnapshot *) NULL, (OCISnapshot *) NULL,
                                 OCI_DEFAULT));
    
  /* Define output buffers */
  checkerr(errhp, OCIDefineByPos (stmthp, &defhp1, errhp, 1, 
                                  (void *) &region_id, (sb4) sizeof(region_id),
                                  SQLT_INT, (void *) NULL, (ub2 *) NULL,
                                  (ub2 *) NULL, OCI_DEFAULT));

  checkerr(errhp, OCIDefineByPos (stmthp, &defhp2, errhp, 2,
                                  (void *)region_name,(sb4)sizeof(region_name),
                                  SQLT_STR, (void *) NULL, (ub2 *) NULL,
                                  (ub2 *) NULL, OCI_DEFAULT));

  /* Fetch one row */
  checkerr(errhp, OCIStmtFetch(stmthp, errhp, num_rows, OCI_FETCH_NEXT,
                        OCI_DEFAULT));
  if (verbose_flag) 
    printf("fetched results: region_id=%d, region_name=%s, \n",
              region_id, region_name);

  /* release the statement handle */
  checkerr(errhp, OCIStmtRelease(stmthp, errhp,
                                 (OraText *) NULL, 0, OCI_DEFAULT)); 
   
}

static char *MY_SELECT2 = (char *) "select /*+ result_cache */ \
                    empno, ename from \
                    empview where empno > :EMPNO order by empno";

void multirow_fetch(OCISvcCtx *svchp, OCIError *errhp, thdata *pthdata)
{
  OCIStmt *stmthp;
  OCIBind *bndp;

  if (verbose_flag)
    printf ("demonstrating array fetching\n");

  /* get a prepared statement handle */
  checkerr(errhp, OCIStmtPrepare2 (svchp, &stmthp, errhp,
				    (const OraText *) MY_SELECT2,
                                   strlen((char *) MY_SELECT2),
                                   (OraText*) NULL, 0,
                                   OCI_NTV_SYNTAX, OCI_DEFAULT));

  /* bind input parameters */
  checkerr(errhp, OCIBindByName(stmthp, &bndp, errhp, (text *) ":EMPNO",
                                -1, (void *) &(pthdata->multirow_fetch_id),
                                sizeof(int), SQLT_INT, (void *) NULL,
                                (ub2 *) NULL, (ub2 *) NULL, 0, (ub4 *) NULL,
                                OCI_DEFAULT));

  /* Execute the query */
  checkerr(errhp, OCIStmtExecute (svchp, stmthp, errhp, 0, 0,
                                  (OCISnapshot *) NULL, (OCISnapshot *) NULL,
                                  OCI_DEFAULT));             
  

  /* separating fetching rows into this method to help understand stage6 ref cursors */
  multirow_fetch_from_emp(svchp, stmthp, errhp, pthdata);

  if (verbose_flag)
    printf ("array fetch successful\n");

  /* release the statement handle */
  checkerr(errhp, OCIStmtRelease(stmthp, errhp,
                                 (OraText *) NULL, 0, OCI_DEFAULT));  

}

/* This select empno, ename from myemp table or empview view 
   - can take different SQL texts 
*/
void multirow_fetch_from_emp(OCISvcCtx *svchp, OCIStmt * stmthp, OCIError *errhp, 
			     thdata *pthdata)
{
#define ARRAY_SIZE 3
  OCIDefine *defhp1, *defhp2;       
  ub4 empid[ARRAY_SIZE];
  sb2 empid_ind[ARRAY_SIZE];
  char lname[ARRAY_SIZE][30];
  sb2  lname_ind[ARRAY_SIZE];
  ub2  lname_len[ARRAY_SIZE];
  boolean done=FALSE;
  ub4 rows = 0;
  ub4 i =0;
  sb4 status;

  /* Define output buffers */
  checkerr(errhp, OCIDefineByPos (stmthp, &defhp1, errhp, 1,
                                  (void *) empid, (sb4) sizeof(empid[0]),
                                  SQLT_INT, (void *) empid_ind,
                                  (ub2 *) NULL, (ub2 *) NULL, OCI_DEFAULT));

  checkerr(errhp, OCIDefineByPos (stmthp, &defhp2, errhp, 2,
                                  (void *) lname[0], (sb4) sizeof(lname[0]),
                                  SQLT_STR, (void *) lname_ind,
                                  (ub2 *) lname_len, (ub2 *) NULL,
                                  OCI_DEFAULT));

  /* Fetch ARRAY_SIZE rows at a time */
  while (!done)
  {
    status = OCIStmtFetch(stmthp, errhp, ARRAY_SIZE,
                          OCI_FETCH_NEXT, OCI_DEFAULT);

    if ((status == OCI_SUCCESS) || (status == OCI_NO_DATA))
    {
      if (status == OCI_SUCCESS)
        rows = ARRAY_SIZE;               /* all rows asked for were obtained */
      else if (status == OCI_NO_DATA)
      {
        /* might have gotten fewer rows */
        checkerr(errhp, OCIAttrGet(stmthp, OCI_HTYPE_STMT,
                             &rows, (ub4 *) NULL,
                             OCI_ATTR_ROWS_FETCHED, errhp));
        done = TRUE;
      }

      for (i=0; i < rows; i++)
      {
        if (verbose_flag)
          printf ("empno=%d, ename=%s\n", empid[i], lname[i]);
      }
    }
    else
    {
      checkerr (errhp, status);
      done =TRUE;
    }
  }
}


/* Using PLSQL within OCI. 
   This procedure raises a given employee's salary by a given amount. The 
   increased salary which results is returned in the stored procedure's 
   variable, new_salary, and the program displays this value.

   Note that the PL/SQL procedure argument, new_salary, although a PL/SQL OUT 
   variable, must be bound, not defined. 
*/

/* Define PL/SQL statement to be used in program. */
text *MY_PLSQL = (text *) "BEGIN\
                  RAISE_SALARY(:emp_id,:sal_increase, :new_salary);\
                     END;";

void plsql_exec(OCISvcCtx *svchp, OCIError *errhp, thdata *pthdata)
{
  sword     raise, new_sal;

  OCIStmt  *stmthp;
  OCIBind  *bnd1p = NULL;                      /* the first bind handle */
  OCIBind  *bnd2p = NULL;                     /* the second bind handle */
  OCIBind  *bnd3p = NULL;                      /* the third bind handle */

  /* prepare the statement request, passing the PL/SQL text
     block as the statement to be prepared 
   */
  if (verbose_flag)
    printf ("demonstrating using PLSQL inside OCI program \n"); 
  
   checkerr(errhp,
           OCIStmtPrepare2 (svchp, &stmthp,          /* returned stmt handle */
                        errhp,                               /* error handle */
                        (const OraText *) MY_PLSQL,    /*input statement text*/
                        strlen((char *) MY_PLSQL),         /* length of text */
                        NULL, 0,             /* tagging parameters: optional */
                        OCI_NTV_SYNTAX, OCI_DEFAULT));

  /* Bind the employee number */ 
  checkerr( errhp, OCIBindByName(stmthp, &bnd1p, errhp, (text *) ":emp_id",
             -1, (ub1 *) & (pthdata->emp_id) ,
            (sword) sizeof(pthdata->emp_id), SQLT_INT, (void *) 0,
             (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT));

  /* set a fixed raise */ 
  raise = 1000;

  /* Bind the salary increase */ 
  checkerr( errhp, OCIBindByName(stmthp, &bnd2p, errhp, 
              (text *) ":sal_increase",     -1, (ub1 *) &raise,
             (sword) sizeof(raise), SQLT_INT, (void *) 0,
             (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT));

  /* remember that PL/SQL OUT variable are bound, not defined */
  checkerr( errhp, OCIBindByName(stmthp, &bnd3p, errhp, (text *) ":new_salary",
             -1, (ub1 *) &new_sal,
             (sword) sizeof(new_sal), SQLT_INT, (void *) 0,
             (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT));

  /* execute PL/SQL procedure */
  checkerr(errhp, OCIStmtExecute(svchp, stmthp, errhp, 1, 0,
                                 (OCISnapshot *) NULL, (OCISnapshot *) NULL,
                                 OCI_DEFAULT));
    
  if (verbose_flag) 
    printf("results: emp_id=%d, salary_increase=%d new_salary=%d \n",
              pthdata->emp_id,raise,new_sal);

  /* release the statement handle */
  checkerr(errhp, OCIStmtRelease(stmthp, errhp,
                                 (OraText *) NULL, 0, OCI_DEFAULT)); 
}

/* Demo of OCI PLSQL ref cursor */
void plsql_refcursor(OCISvcCtx *svchp, OCIError *errhp, thdata *pthdata)
{
  text *plscall = 
          (text *)"begin emp_demo_pkg.open_cur(:myrefcursor, :emp_id); end;";
  OCIStmt  *stmthp;
  OCIStmt  *stmthp2;
  OCIBind  *bnd1p = NULL;                      /* the first bind handle */
  OCIBind  *bnd2p = NULL;                     /* the second bind handle */

  /* prepare the statement request, passing the PL/SQL text
     block as the statement to be prepared 
   */
  if (verbose_flag)
    printf ("demonstrating using PLSQL Ref cursor inside OCI program \n"); 

   checkerr(errhp,
           OCIStmtPrepare2 (svchp, &stmthp,          /* returned stmt handle */
                        errhp,                               /* error handle */
                        (const OraText *) plscall,    /*input statement text*/
                        strlen((char *) plscall),         /* length of text */
                        NULL, 0,             /* tagging parameters: optional */
                        OCI_NTV_SYNTAX, OCI_DEFAULT));


  /* allocate statement handle to receive the execute PLSQL ref cursor */
  checkerr(errhp, OCIHandleAlloc( (dvoid *) envhp, (dvoid **) &stmthp2,OCI_HTYPE_STMT, 
				  (size_t) 0, (dvoid **) 0));

  /* Bind the ref cursor */
  checkerr(errhp, OCIBindByName(stmthp, (OCIBind **)&bnd1p, errhp, 
				(CONST text *)":myrefcursor", (sb4)-1, (dvoid *)&stmthp2,
				(sb4)0,(ub2)SQLT_RSET,(dvoid *)0,(ub2 *)0,(ub2 *)0,
				(ub4)0,(ub4 *)0, (ub4) OCI_DEFAULT));

  /* Bind the employee number */ 
  checkerr( errhp, OCIBindByName(stmthp, &bnd2p, errhp, (text *) ":emp_id",
				 -1, (ub1 *) & (pthdata->emp_id),
				 (sword) sizeof(pthdata->emp_id), SQLT_INT, (void *) 0,
				 (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT));

  /* execute the PLSQL procedure */
  checkerr(errhp, OCIStmtExecute(svchp, stmthp, errhp,(ub4)1, (ub4)0,
				 (CONST OCISnapshot *)NULL,
				 (OCISnapshot *)NULL,(ub4)OCI_DEFAULT));

  /* the out-bind is now a executed OCI statement handle so can now fetch from it */
  multirow_fetch_from_emp(svchp, stmthp2, errhp, pthdata);

  /* free the statement handle */
  checkerr(errhp, OCIHandleFree((dvoid *)stmthp2,(ub4)OCI_HTYPE_STMT));

  if (verbose_flag)
    printf ("ref cursor fetch successful\n");

}

/* Use DML returning to get back locator */
text * MY_LOBQUERY = "INSERT INTO lob_table VALUES ( :mylobid, EMPTY_CLOB()) \
                          returning lobcol into :myclob";

#define CDEMOLB_TEXT_FILE "ocidemolb.dat"
#define BUFSIZE          1024

/* Show Lob Write within OCI */
void lob_write(OCISvcCtx *svchp, OCIError *errhp, thdata *pthdata, int iteration)
{
  OCILobLocator    *lobp;
  OCIStmt  *stmthp;
  OCIBind  *bnd1p = NULL;                      /* the first bind handle */
  OCIBind  *bnd2p = NULL;                    /* the second bind handle */
  FILE *fp;
  ub1  buf[BUFSIZE+1];
  ub4  offset;
  ub4  amtp;
  ub4 lenp;
  sb4 err;
  int mylobid; 

  mylobid = (pthdata->thread_no * 1000) + (iteration+ 1);

  /* allocate lob descriptor */
  checkerr(errhp, OCIDescriptorAlloc((dvoid *) envhp, (dvoid **) &lobp,
				     (ub4) OCI_DTYPE_LOB,
				     (size_t) 0, (dvoid **) 0));

  /* prepare the statement request, passing the PL/SQL text
     block as the statement to be prepared 
   */
  if (verbose_flag)
    printf ("demonstrating LOB Write inside OCI program \n"); 
  
  checkerr(errhp,
           OCIStmtPrepare2 (svchp, &stmthp,          /* returned stmt handle */
                        errhp,                             /* error handle */
                     (const OraText *) MY_LOBQUERY,    /*input statement text*/
                     strlen((char *) MY_LOBQUERY),         /* length of text */
                        NULL, 0,             /* tagging parameters: optional */
                        OCI_NTV_SYNTAX, OCI_DEFAULT));

  checkerr( errhp, OCIBindByName(stmthp, &bnd1p, errhp, (text *) ":mylobid",
                            -1, (void *) &mylobid,
                              sizeof(mylobid), SQLT_INT, (void *) 0,
               	  (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT));

  checkerr( errhp, OCIBindByName(stmthp, &bnd2p, errhp, (text *) ":myclob",
                            -1, (void *) &lobp,
                              0, SQLT_CLOB, (void *) 0,
               	  (ub2 *) 0, (ub2) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT));

  /* execute LOB query */
  checkerr(errhp, OCIStmtExecute(svchp, stmthp, errhp, 1, 0,
                                 (OCISnapshot *) NULL, (OCISnapshot *) NULL,
                                 OCI_DEFAULT));

  /* release the statement handle */
  checkerr(errhp, OCIStmtRelease(stmthp, errhp,
                                 (OraText *) NULL, 0, OCI_DEFAULT)); 
  

  /* open file for reading */ 
  if ((fp = fopen (CDEMOLB_TEXT_FILE, "r"))==NULL)
  {
    printf ("Cannot open file for reading \n");
    exit (1);
  }

  /* write into this empty lob from beginning i.e. offset 1 */
  offset = 1;                              

  while (!feof(fp))
  {
    /* Read the data from file */
    memset ((void *)buf, '\0', BUFSIZE);
    fread((void *)buf, BUFSIZE, 1, fp);
    buf[BUFSIZE]='\0';
    /* printf("%s",buf); */

    /*Write it into the locator */
    amtp = BUFSIZE;                 /* IN/OUT : IN - amount if data to write */
    err = OCILobWrite (svchp, errhp, lobp, &amtp, offset,
                   (dvoid *) buf, (ub4) BUFSIZE, OCI_ONE_PIECE,
                   (dvoid *)0, (sb4 (*)()) 0,
                   (ub2) 0, (ub1) SQLCS_IMPLICIT);

    if (err == OCI_SUCCESS)
      offset += amtp;
    else
    {
      fclose(fp);
      printf ("Write_to_loc error : OCILobWrite \n");
      printerrmsg(errhp,OCI_HTYPE_ERROR);
    }
  }

  if (verbose_flag)
    printf("Write Successful \n");

  fclose (fp);

  /* get length of lob */ 
  checkerr(errhp, OCILobGetLength(svchp, errhp, lobp, &lenp));

  if (verbose_flag)
      printf ("Written %d bytes into Locator Successfully.\n\n", lenp);

  /* commit transaction */ 
  checkerr(errhp, OCITransCommit (svchp, errhp,0));

  (void) OCIDescriptorFree((dvoid *) lobp, (ub4) OCI_DTYPE_LOB);
}

text * MY_LOBQUERY2 = "select lobcol from lob_table where lobid = :1"; 

/* Show Lob Read with prefetching within OCI */
void lob_read(OCISvcCtx *svchp, OCIError *errhp, thdata *pthdata, int iteration)
{
  OCILobLocator    *lobp;
  OCIStmt  *stmthp;
  OCIBind  *bndp = NULL;                      /* the first bind handle */
  OCIDefine *dfnhp = NULL;
  FILE *fp;
  ub1  buf[BUFSIZE+1];
  ub4  offset;
  ub4  amtp;
  ub4 lenp;
  sb4 err;
  int mylobid; 
  boolean done = FALSE;
  sword  status;

  /* create unique key so each thread (i.e. session) reads 
     a limited set of rows from table.
  */
  mylobid = (pthdata->thread_no * 1000) + (iteration + 1);

  /* prepare the statement request, passing the PL/SQL text
     block as the statement to be prepared 
   */
  if (verbose_flag)
    printf ("demonstrating LOB Read inside OCI program \n"); 
  
  checkerr(errhp,
           OCIStmtPrepare2 (svchp, &stmthp,          /* returned stmt handle */
                        errhp,                             /* error handle */
                     (const OraText *) MY_LOBQUERY2,    /*input statement text*/
                     strlen((char *) MY_LOBQUERY2),         /* length of text */
                        NULL, 0,             /* tagging parameters: optional */
                        OCI_NTV_SYNTAX, OCI_DEFAULT));


  checkerr (errhp, OCIBindByPos(stmthp, &bndp, errhp, (ub4) 1,
                 (dvoid *) &mylobid, (sb4) sizeof(mylobid), SQLT_INT,
                 (dvoid *) 0, (ub2 *) 0,
				(ub2 *) 0, (ub4) 0, (ub4 *) 0, (ub4) OCI_DEFAULT));

  /* allocate lob descriptor */
  checkerr(errhp, OCIDescriptorAlloc((dvoid *) envhp, (dvoid **) &lobp,
				     (ub4) OCI_DTYPE_LOB,
				     (size_t) 0, (dvoid **) 0));

  checkerr(errhp, OCIDefineByPos (stmthp, &dfnhp, errhp, 1,
                 (dvoid *)&lobp, 0 , SQLT_CLOB,
		  (dvoid *)0, (ub2 *)0, (ub2 *)0, OCI_DEFAULT));


  /* execute LOB query */
  checkerr(errhp, OCIStmtExecute(svchp, stmthp, errhp, 0, 0,
                                 (OCISnapshot *) NULL, (OCISnapshot *) NULL,
                                 OCI_DEFAULT));


  /* read all lobs */ 
  while ((status = OCIStmtFetch(stmthp, errhp, 1, 
				OCI_FETCH_NEXT, OCI_DEFAULT)) != OCI_NO_DATA)
  {
    if (status != OCI_SUCCESS)
       checkerr (errhp, status);

    /* get length of lob */ 
    checkerr(errhp, OCILobGetLength(svchp, errhp, lobp, &lenp));

    if (verbose_flag)
      printf ("Reading %d bytes from Locator \n", lenp);

    offset = 1;

    /* Read the LOB - there will be less roundtrips since lob-prefetch data
     *   is fetched with the locator
     */
    do
    {
      memset ((dvoid *)buf, '\0', BUFSIZE);
      err = OCILobRead(svchp, errhp, lobp, &amtp, offset,
			 (dvoid *) buf, (ub4)BUFSIZE , (dvoid *) 0,
			 (OCICallbackLobRead) 0, (ub2) 0, (ub1) SQLCS_IMPLICIT);

      if (err == OCI_SUCCESS || err == OCI_NEED_DATA)
      {
        buf[BUFSIZE] = '\0';
         
	/* printf ("%s", buf);  */ 
	offset +=amtp;

      }
      else
      {
        printerrmsg(errhp,OCI_HTYPE_ERROR);
	break;
      }
    }
    while (offset <= lenp);

    /* offset has gone beyond lob length so decrement it */ 
    if (verbose_flag)
      printf ("Read %d bytes from Locator \n", --offset);
  }

  /* release the statement handle */
  checkerr(errhp, OCIStmtRelease(stmthp, errhp,
                                 (OraText *) NULL, 0, OCI_DEFAULT));   

}

/* do_workload() executes update_salary, query_salary and multirow_fetch, 
   and collect statistics on accumulated waiting time on above operations */
void do_workload(OCISvcCtx *svchp, OCIError *errhp, thdata *pthdata, int iteration)
{
  struct timeval start, finish ;

  /* Simulate web input for salary updates */
  random_input(pthdata);
    
  gettimeofday (&start, NULL);
  
  query_salary(svchp, errhp, pthdata);
  gettimeofday (&finish, NULL);
  pthdata->querytime += (finish.tv_sec-start.tv_sec) * 1000000 +
                        (finish.tv_usec-start.tv_usec);

  multirow_fetch(svchp, errhp, pthdata);
  gettimeofday (&start, NULL);
  pthdata->fetchtime -= (finish.tv_sec-start.tv_sec) * 1000000 +
                        (finish.tv_usec-start.tv_usec);

  update_salary(svchp, errhp, pthdata);

  plsql_exec(svchp, errhp, pthdata);

  plsql_refcursor(svchp, errhp, pthdata);

  lob_write(svchp, errhp, pthdata,iteration); 

  lob_read(svchp, errhp, pthdata,iteration); 

  gettimeofday (&finish, NULL);
  pthdata->updatetime += (finish.tv_sec-start.tv_sec) * 1000000 +
                         (finish.tv_usec-start.tv_usec);
}

int main(int argc, char *argv[])
{
  OCIError *errhp = NULL;

  printf("stage5: Demonstrating client resultset cache\n");

  /* parse command line options */
  parse_options(argc, argv);
  
  checkenv(envhp, OCIEnvCreate(&envhp,                /* returned env handle */
                               OCI_THREADED,         /* initialization modes */
                               NULL, NULL, NULL, NULL, /* callbacks, context */
                               (size_t) 0,    /* extra memory size: optional */
                               (void **) NULL));    /* returned extra memory */

  /* allocate error handle
   * note: for OCIHandleAlloc(), we always check error on environment handle
   */
  checkenv(envhp, OCIHandleAlloc(envhp,                /* environment handle */
                                 (void **) &errhp,    /* returned err handle */
                                 OCI_HTYPE_ERROR,/*type of handle to allocate*/
                                 (size_t) 0,  /* extra memory size: optional */
                                 (void **) NULL));  /* returned extra memory */

  create_session_pool(envhp, errhp, &poolName, &poolNameLen);

  /* allocate auth handle
   * note: for OCIHandleAlloc(), we check error on environment handle
   */
  checkenv(envhp, OCIHandleAlloc(envhp,
                          (void **) &authp, OCI_HTYPE_AUTHINFO,
                          (size_t) 0, (void **) NULL));

  /* setup username and password */
  checkerr(errhp, OCIAttrSet(authp, OCI_HTYPE_AUTHINFO,
                             (void *) username, strlen((char *)username),
                             OCI_ATTR_USERNAME, errhp));

  checkerr(errhp, OCIAttrSet(authp, OCI_HTYPE_AUTHINFO,
                            apppassword, strlen((char *) apppassword),
                            OCI_ATTR_PASSWORD, errhp));

  spawn_threads(envhp, errhp, &thread_function);
  
  /* Destroy the session pool */
  OCISessionPoolDestroy(spoolhp, errhp, OCI_DEFAULT);

  /* clean up */
  if (authp)
    OCIHandleFree(authp, OCI_HTYPE_AUTHINFO);
  if (spoolhp)
    OCIHandleFree(spoolhp, OCI_HTYPE_SPOOL); 
  if (errhp)
    OCIHandleFree(errhp, OCI_HTYPE_ERROR);
  if (envhp)
    OCIHandleFree(envhp, OCI_HTYPE_ENV);
  
  return 0;
}

/* thread_function can be run in multi-threads, each thread will have its own 
   error handle */
void thread_function(void *ptr)
{
  OCIError    *errhp = NULL;
  int          i=0;
  OCISession  *usrhp_svc = (OCISession *) 0;
  ub4          default_lobprefetch_size = 0;

  /* allocate error handle
   * note: for OCIHandleAlloc(), we always check error on environment handle
   */
  checkenv(envhp, OCIHandleAlloc(envhp,                /* environment handle */
                                 (void **) &errhp,    /* returned err handle */
                                 OCI_HTYPE_ERROR,/*type of handle to allocate*/
                                 (size_t) 0,  /* extra memory size: optional */
                                 (void **) NULL));  /* returned extra memory */
  
  for(i=0; i<iteration; i++)
  {
    OCISvcCtx *svchp = NULL; /* OCI Service Context is a database connection */

    /* get the database connection */
    checkerr(errhp, OCISessionGet(envhp, errhp,
                                &svchp,      /* returned database connection */
                                authp,  /* initialized authentication handle */
                                                        /* connect pool name */
                                (OraText *) poolName, poolNameLen,
                                     /* session tagging parameters: optional */
                                NULL, 0, NULL, NULL, NULL,
                                OCI_SESSGET_SPOOL));

    /* set lob prefetch size (to 2K bytes) */ 

    /* Get the user handle from the service context handle */
    checkerr(errhp, OCIAttrGet(svchp, OCI_HTYPE_SVCCTX, &usrhp_svc,
               0,OCI_ATTR_SESSION,errhp));

    default_lobprefetch_size = 2000;  

    checkerr(errhp, OCIAttrSet((OCISession *)usrhp_svc,
			       (ub4) OCI_HTYPE_SESSION,
			       (dvoid *)&default_lobprefetch_size,
			       (ub4) sizeof(ub4),
			       (ub4) OCI_ATTR_DEFAULT_LOBPREFETCH_SIZE,
			       (OCIError *)errhp));

    do_workload(svchp, errhp, ptr,i);
    
    /* destroy the connection */
    checkerr(errhp, OCISessionRelease(svchp, errhp, NULL, 0, OCI_DEFAULT));

    print_progress(ptr, i);                                /* print progress */
    
    if (waittime > 0)
      sleep(waittime);            /* Thinking time between database sessions */
  }

  if (errhp)
    OCIHandleFree(errhp, OCI_HTYPE_ERROR);
}

static void create_session_pool(OCIEnv *envhp, OCIError * errhp,
                         char **poolName, ub4 *poolNameLenp)
{
  ub4       stmt_cachesize = 20;
  ub4  min;                            /* minimum number of sessions in pool */
  ub4  max;                            /* maximum number of sessions in pool */
  ub4  increment;                     /* the number to increment the pool by */

  /* allocate session pool handle
   * note: for OCIHandleAlloc(), we check error on environment handle
   */
  checkenv(envhp, OCIHandleAlloc(envhp, (void **) &spoolhp,
           OCI_HTYPE_SPOOL, (size_t) 0, (void **) NULL));

  /* set the statement cache size for all sessions in the pool */
  checkerr(errhp, OCIAttrSet(spoolhp, OCI_HTYPE_SPOOL,
                  &stmt_cachesize, 0, OCI_ATTR_SPOOL_STMTCACHESIZE, errhp));

  min = thread_num;                    /* minimum number of sessions in pool */
  max = thread_num + 1;                /* maximum number of sessions in pool */
  increment = 1;                      /* the number to increment the pool by */

  /* create a homogeneous session pool */
  checkerr(errhp,
           OCISessionPoolCreate(envhp, errhp,
                                spoolhp,              /* session pool handle */
                                                /* returned poolname, length */
                                (OraText **) poolName, poolNameLenp,
                                                           /* connect string */
                                (const OraText *) connstr, strlen(connstr),
                                min, max, increment,/* pool size constraints */
                                                                 /* username */
                                (OraText *) username,strlen((char *) username),
                                                                 /* password */
                                (OraText *) apppassword,
                                strlen((char *) apppassword),
                                                                    /* modes */
                                OCI_SPC_HOMOGENEOUS|OCI_SPC_STMTCACHE));
}
