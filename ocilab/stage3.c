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

static char *MY_SELECT2 = (char *) "select empno, ename from \
                    myemp where empno > :EMPNO order by empno";

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
  gettimeofday (&finish, NULL);
  pthdata->updatetime += (finish.tv_sec-start.tv_sec) * 1000000 +
                         (finish.tv_usec-start.tv_usec);
}

int main(int argc, char *argv[])
{
  OCIError *errhp = NULL;

  printf("stage3: Demonstrating OCI Session Pooling \n");

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
  ub4  min;                            /* minimum number of sessions in pool */
  ub4  max;                            /* maximum number of sessions in pool */
  ub4  increment;                     /* the number to increment the pool by */

  /* allocate session pool handle
   * note: for OCIHandleAlloc(), we check error on environment handle
   */
  checkenv(envhp, OCIHandleAlloc(envhp, (void **) &spoolhp,
           OCI_HTYPE_SPOOL, (size_t) 0, (void **) NULL));



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
                                OCI_SPC_HOMOGENEOUS));
}
