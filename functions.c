#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int failedcount = 0;
FILE *fh;

char* dt();
char* concat();
char* prtstr();
int getfailedcount();
void setfailedcount();
void suspend();
void resume();
char* suspended();
void logger();

char main (int argc, char *argv[]) {
	//sleep(1);
	/*if (strcmp(argv[1], "suspend") == 0) {
		suspend();
	} else if (strcmp(argv[1], "resume") == 0) {
		resume();
	} else if (strcmp(argv[1], "suspended") == 0) {
		printf("%s \n", suspended());
	}*/
	logger(1,"success","+9233226655");
	logger(2,"errors","+9233226655");
	logger(3,"failed","+9233226655");
	logger(9,"server","+9233226655");
	logger(6,"Test","+9233226655");
	return 0;
}

int getfailedcount() {
	return failedcount;
}

void setfailedcount(int n) {
	if (n = 0) {
		failedcount = 0;
	} else {
		failedcount = failedcount+1;
	}
}

void suspend() {
	fh = fopen("/tmp/suspend", "w");
	fprintf(fh, "Y");
	fclose(fh);
}

void resume() {
	fh = fopen("/tmp/suspend", "w");
	fprintf(fh, "N");
	fclose(fh);
}

char* suspended() {
	char str[1];
	fh = fopen("/tmp/suspend", "r");
	fscanf(fh, "%s", str);
	fclose(fh);
	size_t l = strlen(str);
	char *result = malloc(l+1);
	free(result);
	memcpy(result, str, l);
	return result;
}

void logger(int level, char *msg, char *num) {
	if (level == 1) {		// On success
		fh = fopen("/tmp/smslog", "a");
		fputs(concat(concat(concat(concat(concat(dt()," : "),num)," : "),msg),"\n"), fh);
		fclose;
	} else if (level == 2 ) {	// On failure
		fh = fopen("/tmp/errorlog", "a");
		fputs(concat(concat(concat(concat(concat(dt()," : "),num)," : "),msg),"\n"), fh);
		fclose;
	} else if (level == 3 ) {	// Failed log
		fh = fopen("/tmp/failedlog", "a");
		fputs(concat(concat(concat(concat(concat(dt()," : "),num)," : "),msg),"\n"), fh);
		fclose;
	} else if (level == 9 ) {	// Server log
		fh = fopen("/tmp/serverlog", "a");
		fputs(concat(concat(concat(concat(concat(dt()," : "),num)," : "),msg),"\n"), fh);
		fclose;
	} else {	// Unknown errors
		fh = fopen("/tmp/errorlog", "a");
		fputs(concat(concat(concat(concat(concat(dt()," : "),num)," : "),msg),"\n"), fh);
		fclose;
	}
}

char* prtstr(char *str) {
	size_t len = strlen(str);
	char *result = malloc(len+1);
	free(result);
	memcpy(result, str, len);
	return result;
}

char* concat(char *s1, char *s2)
{
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char *result = malloc(len1+len2+1);
	free(result);
    memcpy(result, s1, len1);
    memcpy(result+len1, s2, len2+1);
    return result;
}

char* dt () {
   #define LEN 150
   char buf[LEN];
   time_t curtime;
   struct tm *loc_time;
   curtime = time(NULL);
   loc_time = localtime(&curtime);
   strftime(buf, LEN, "%d-%b-%Y %I:%M:%S", loc_time);
   size_t buflen = strlen(buf);
   char *result = malloc(buflen+1);//+1 for the zero-terminator
   free(result);
   memcpy(result, buf, buflen);
   return result;
}