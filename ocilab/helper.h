/*

   NAME
     helper.h - Helper library for various stages of the application.

   DESCRIPTION

     Tests OCI client code, the main driver goes from main()=>
     thread_function()=> do_workload(), where do_workload() is defined in 
     workload.c.

*/

#ifndef _SAMPLE_H_
#define _SAMPLE_H_

/* datastructure for each thread */
typedef struct str_thdata
{
  int thread_no;
  ub8 updatetime;                         /* statistics for performance data */
  ub8 querytime;
  ub8 fetchtime;
  unsigned seed;                  /* seed for pseudo-random number generator */
  int emp_id;                                       /* employee id for query */
  int region_id;                                      /* region id for query */
  int multirow_fetch_id;                  /* employee id for multi-row fetch */
  int *salaries;                /* pointer to salary array for table updates */
  int *id;                 /* pointer to empolyee ID array for table updates */
} thdata;

void checkerr0(void *handle, ub4 htype, sword status);

/* This only prints error message assuming there is an error. It does not 
   exit program (unlike checkerr0).
*/
void printerrmsg(void *handle, ub4 htype);

/* friendly error checking macros */
/* checkerr(): meant for OCI calls that take an input error handle  */
#define checkerr(errhp, status) checkerr0((errhp), OCI_HTYPE_ERROR, (status))

/* checkenv(): meant for OCI calls that do not take an input error handle  */
#define checkenv(envhp, status) checkerr0((envhp), OCI_HTYPE_ENV, (status))
/* Testing workload */
void do_workload(OCISvcCtx *svchp, OCIError * errhp, thdata * pthData, int iteration);

extern int max_queriable_id;
extern int verbose_flag;
extern int waittime;
extern int iteration;
extern int update_num;
extern int thread_num;
extern OCIEnv *envhp;

#define print_progress(ptr, i)\
    if(thread_num==((thdata *)ptr)->thread_no +1)         /* print progress */\
    {\
      printf("\r\t\t\t\t%d%%", (int)((1.0+i)/iteration*100.0));\
      fflush(stdout);\
    } 

void parse_options(int argc, char *argv[]);
void random_input(thdata * pthdata);

void thread_function(void * ptr);
void spawn_threads(void * envhp, OCIError * errhp, 
                   void (* thread_fun)(void *));

#endif
