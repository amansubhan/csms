/*

   NAME
     helper.c - helper functions

   DESCRIPTION

     This file contains helper functions like parse_options() to parse command
     line options. 

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <oci.h>
#include "helper.h"

/* input parameters */
int thread_num = 20;                             /* Number of Threads */
int waittime = 0;/* thinking time between database sessions in seconds*/
int iteration = 120;    /* number of units of work done by each thread */
int verbose_flag = 0;                     /* Flag set by `--verbose'. */
int update_num = 80;   /* number of updates in a single unit of work */

/* random_input: Initialize Input data that simulate web input */
void random_input(thdata * pthdata) 
{
  #define MIN_EMP_ID 100
  #define MAX_EMP_ID 113

  #define MIN_REGION_ID 1
  #define MAX_REGION_ID 4

  #define MIN_SALARY 100
  #define MAX_SALARY 999999

  /* minimal and maximal employee IDs for ith thread */
  #define T_MIN_EMP_ID(i) (MIN_EMP_ID+ (MAX_EMP_ID+1-MIN_EMP_ID)*(i)/thread_num)
  #define T_MAX_EMP_ID(i) ((T_MIN_EMP_ID(i+1))-1)

  #define TMIN_EMP_ID(ptr) T_MIN_EMP_ID((ptr)->thread_no)
  #define TMAX_EMP_ID(ptr) T_MAX_EMP_ID((ptr)->thread_no)

  #define RAND_INTERVAL(ptr, mn, mx) \
          (mn + ((mx-mn)*(((double)rand_r(&(ptr->seed)))/(double)RAND_MAX)))

  #define RAND_EMP_ID(ptr) RAND_INTERVAL(ptr, MIN_EMP_ID, MAX_EMP_ID)
  #define RAND_REGION_ID(ptr) RAND_INTERVAL(ptr, MIN_REGION_ID, MAX_REGION_ID)

  #define RAND_T_EMPID(ptr) RAND_INTERVAL(ptr, TMIN_EMP_ID(ptr), \
                                          TMAX_EMP_ID(ptr))

  #define RAND_SALARY(ptr) RAND_INTERVAL(ptr, MIN_SALARY, MAX_SALARY)

  int i = 0;
  pthdata->region_id = RAND_REGION_ID(pthdata);
  pthdata->emp_id = RAND_EMP_ID(pthdata);
  pthdata->multirow_fetch_id = RAND_EMP_ID(pthdata);

  for(i=0; i<update_num; i++)
  {
    *(pthdata->id+i) = RAND_T_EMPID(pthdata);
    *(pthdata->salaries+i) = RAND_SALARY(pthdata);
  }
}

/* prints the error message */
void printerrmsg(void *handle, ub4 htype)
{
  /* a buffer to hold the error message */
  text errbuf[2048];
  sb4 errcode = 0;

  (void) OCIErrorGet(handle, 1, (text *) NULL, &errcode,
                         errbuf, (ub4) sizeof(errbuf), htype);
  (void) printf("Error - %.*s\n", sizeof(errbuf), errbuf);
}

/*
 * checkerr0(): This function prints a detailed error report if an OCI call 
 *              fails.
 *              Used to "wrap" invocation of OCI calls.
 * Parameters:
 *   handle (IN) - can be either an environment handle or an error handle.
 *                 for OCI calls that take in an OCIError Handle:
 *                 pass in an OCIError Handle
 * 
 *                 for OCI calls that don't take an OCIError Handle,
 *                 pass in an OCIEnv Handle
 *
 *   htype   (IN)- type of handle: OCI_HTYPE_ENV or OCI_HTYPE_ERROR
 *                 
 *   status (IN) - the status code returned from the OCI call
 *
 * Notes:
 *                 Note that this "exits" on the first
 *                 OCI_ERROR/OCI_INVALID_HANDLE.
 *                 CUSTOMIZE ACCORDING TO YOUR ERROR HANDLING REQUIREMENTS 
 */
void checkerr0(void *handle, ub4 htype, sword status)
{

  switch (status)
  {
  case OCI_SUCCESS:
    break;
  case OCI_SUCCESS_WITH_INFO:
    (void) printf("Error - OCI_SUCCESS_WITH_INFO\n");
    break;
  case OCI_NEED_DATA:
    (void) printf("Error - OCI_NEED_DATA\n");
    break;
  case OCI_NO_DATA:
    (void) printf("Error - OCI_NODATA\n");
    break;
  case OCI_ERROR:
    (void) printf("Error - OCI_ERROR\n");
    if (handle)
      printerrmsg(handle,htype);
    else
    {
      (void) printf("NULL handle\n");            
      (void) printf("Unable to extract detailed diagnostic information\n");
    }
    exit(1);
    break;
  case OCI_INVALID_HANDLE:
    (void) printf("Error - OCI_INVALID_HANDLE\n");
    exit(1);
    break;
  case OCI_STILL_EXECUTING:
    (void) printf("Error - OCI_STILL_EXECUTE\n");
    break;
  case OCI_CONTINUE:
    (void) printf("Error - OCI_CONTINUE\n");
    break;
  default:
    break;
  }
}


void parse_options(int argc, char *argv[])
{
  OCIError *errhp = NULL;
  int c;
  /* parse command line options */
  /* getopt_long stores the option index here. */
  int option_index = 0;
  while (1)
  {
    static struct option long_options[] =
    {
      /* These options don't set a flag.
         We distinguish them by their indices. */
      {"help",        no_argument,       0, 'h'},
      {"verbose",     no_argument,       0, 'v'},
      {"iteration",   required_argument, 0, 'i'},
      {"threadnum",   required_argument, 0, 't'},
      {"updatenum",   required_argument, 0, 'u'},
      {"thinktime",    required_argument, 0, 'w'},
      {0, 0, 0, 0}
    };
  
    c = getopt_long (argc, argv, "hi:t:u:vw:",
             long_options, &option_index);
    /* Detect the end of the options. */
    if (c == -1)
      break;
  
    switch (c)
    {
    case 0:
      break;
  
    case 'h':
      printf("usage: %s [-t threadnum] [-w thinktime] [-i iteration] [-u updatenum] [-v] [-h]", argv[0]);
      printf("\n\nDescription: \n");
      printf("An OCI sample code to update, query and fetch\n\n");
      
      printf("-t, --thread \n");
      printf("\t The number of threads, the default is 20 threads\n\n");
      
      printf("-w, --thinktime \n");
      printf("\t think time in seconds between units of workload\n");
      printf("\t the default is 0 second\n\n");

      printf("-i, --iteration \n");
      printf("\t The number of units of workload in each thread\n");
      printf("\t the default iteration is 60\n\n");

      printf("-u, --updatenum \n");
      printf("\t number of updates in a unit of workload\n");
      printf("\t the default number of updates is 80 \n\n");

      printf("-v, --verbose \n");
      printf("\t print detailed information\n\n");

      printf("-h, --help \n");
      printf("\t print this help info\n\n");
      
      exit(0);
      break;
      
    case 'i':
      iteration = atoi(optarg);
      break;
      
    case 't':
      thread_num = atoi(optarg);
      break;
  
    case 'u':
      update_num = atoi(optarg);
      break;

    case 'v':
      verbose_flag = 1;
      break;
      
    case 'w':
      waittime = atoi(optarg);
      break;
      
    case '?':
      /* getopt_long already printed an error message. */
      break;
  
    default:
      abort ();
    }
  }
  
  printf("This application will spawn %d threads\n", thread_num);
  printf("Each thread will perform %d units of workload\n", iteration);
  printf("Each unit of workload will include %d updates\n", update_num);
  printf("Think time between two units of workload is %d seconds\n\n",
         waittime);
/* end of command line parsing */

}

void spawn_threads(void * envhp, OCIError * errhp, 
                   void (* thread_fun)(void *))
{
  sb4 error = OCI_SUCCESS;
  OCIThreadProcessInit();
  
  if( thread_num >= 1)
  { /* multithreaded workload */
    int i;
    ub8 updatetime = 0;
    ub8 querytime  = 0;
    ub8 fetchtime  = 0;
    OCIThreadId     **tidp   = calloc(thread_num, sizeof(void *));
    OCIThreadHandle **thdhpp = calloc(thread_num, sizeof(void *));
    thdata    * pData   = calloc(thread_num, sizeof(thdata));
    /* User input data for each thread */
    /* Each thread have two arrays: salaries[updatenum] and id[updatenum] 
       as input from web page */
    int       * pInput  = calloc(thread_num, 2*update_num*sizeof(int));
    
    for( i=0; i< thread_num; i++)
    {
      pData[i].thread_no  = i;
      pData[i].updatetime = 0;
      pData[i].querytime  = 0;
      pData[i].fetchtime  = 0;

      /* Initialize random number generator seed for each thread */
      pData[i].seed = time(NULL) + 100*i;
      
      /* calculate each thread's input */
      pData[i].salaries   = pInput + i*2*update_num;
      pData[i].id         = pData[i].salaries + update_num;
      
      /* create threads */
      if (error=OCIThreadIdInit (envhp, errhp, tidp+i))
      {
        checkerr(errhp, error);
        continue;
      }
      if (error=OCIThreadHndInit (envhp, errhp, thdhpp+i))
      {
        checkerr(errhp, error);
        continue;
      }
        
      OCIThreadCreate ((void *)envhp, errhp, thread_fun, pData+i, *(tidp+i), 
                       *(thdhpp+i));
    }
    for( i=0; i< thread_num; i++)
    {
      OCIThreadJoin((void *)envhp, errhp, *(thdhpp+i));
    }

    /* get performance statistics */
    for( i=0; i< thread_num; i++)
    {
      updatetime += pData[i].updatetime;
      querytime  += pData[i].querytime;
      fetchtime  += pData[i].fetchtime;

      /* close OCI threads */
      OCIThreadClose((void *)envhp, errhp, *(thdhpp+i));
      OCIThreadIdDestroy( (void *)envhp, errhp, tidp+i);
      OCIThreadHndDestroy((void *)envhp, errhp, thdhpp+i);     
    }
    
    printf("\nThe application takes %f seconds to update records\n", 
            updatetime/1000000.0);
    printf("The application takes %f seconds for single row queries\n", 
            querytime/1000000.0);
    printf("The application takes %f seconds for multi-row fetch\n", 
            fetchtime/1000000.0);
    printf("Each thread takes %f seconds average to update records\n", 
            updatetime/1000000.0/thread_num);
    printf("Each thread takes %f seconds average for single row queries\n", 
            querytime/1000000.0/thread_num);
    printf("Each thread takes %f seconds average for multi-row fetch\n", 
            fetchtime/1000000.0/thread_num);
    
    if( tidp )   free(tidp);
    if( thdhpp ) free(thdhpp);
    if( pData )  free(pData);
    if( pInput ) free (pInput);
  }
}
