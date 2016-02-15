#include <ocilib.h>
/**
 * Functions Declarations:
 */
void err_handler(OCI_Error *err);

/**
 * Main program
 * @param
 * @param
 * @return
 */
int main(int argc, char *argv[])
{
    OCI_Connection* cn;
    OCI_Statement* st;
    OCI_Resultset* rs;

    if (!OCI_Initialize(err_handler, NULL, OCI_ENV_DEFAULT))
        return EXIT_FAILURE;
    cn = OCI_ConnectionCreate("ytmdb2.ytm.net:1521/oracle91", "scott", "tiger", OCI_SESSION_DEFAULT);

    printf("Server major    version : %i\n",   OCI_GetServerMajorVersion(cn));
    printf("Server minor    version : %i\n",   OCI_GetServerMinorVersion(cn));
    printf("Server revision version : %i\n\n", OCI_GetServerRevisionVersion(cn));
    printf("Connection      version : %i\n\n", OCI_GetVersionConnection(cn)); 

    char sql[100] = "select empno, ename, job from emp";
    st = OCI_StatementCreate(cn);
    OCI_ExecuteStmt(st, sql);
    rs = OCI_GetResultset(st);
    while (OCI_FetchNext(rs))    {
        printf("%i - %s - %s \n", OCI_GetInt(rs, 1), OCI_GetString(rs,2), OCI_GetString(rs,3));
    }
    OCI_Cleanup();
    return EXIT_SUCCESS;   
}

/** 
 * Error handling function
 * @param
 */
void err_handler(OCI_Error *err)
{
    printf(
                "code  : ORA-%05i\n"
                "msg   : %s\n"
                "sql   : %s\n",
                OCI_ErrorGetOCICode(err), 
                OCI_ErrorGetString(err),
                OCI_GetSql(OCI_ErrorGetStatement(err))
           );
}