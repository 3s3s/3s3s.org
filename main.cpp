#include "simple_server.h"
#include "StartupInfo.h"
#include "log.h"

using namespace simple_server;
using namespace startup;
using namespace curl;

#define CLIENT_T	ClientForProxy
#include "./proxy/ClientForProxy.h"


#ifndef _WIN32
CStartupInfo<CLIENT_T> settings(80, 443, WWW_ROOT, WWW_ROOT "errorpages", "index.ssp");
#define _unlink	unlink
#else
CStartupInfo<CLIENT_T> settings(80, 1111, WWW_ROOT, WWW_ROOT "errorpages", "index.ssp");
#endif

set<string> g_vWhiteListedIP;

unordered_map<string, shared_ptr<CProxyInfo> > simple_server::g_mapProxyInfo;

int main()
{
	orm::CTable::CreateTable("UsersSites", "(url TEXT, title TEXT, description TEXT, UserIP TEXT, time INTEGER, flag INTEGER)");

	//orm::CTable::ExecuteSQL("DROP TABLE LINKS");
	orm::CTable::CreateTable("LINKS", "(long TEXT, short TEXT PRIMARY KEY)");

	orm::CTable::ExecuteSQL("ALTER TABLE LINKS ADD COLUMN time INTEGER");
	orm::CTable::ExecuteSQL("ALTER TABLE UsersSites ADD COLUMN lang TEXT");

	orm::CTable::ExecuteSQL("ALTER TABLE LINKS ADD COLUMN creatorIP TEXT");
	orm::CTable::ExecuteSQL("ALTER TABLE LINKS ADD COLUMN status TEXT");

	CServer<CStartupInfo<CLIENT_T> > server(settings);
	
	while(server.m_bIsInit)
	{
		server.Continue();
	}
	return 0;
}

#ifdef _DEBUG_LOG
#include <stdio.h>

void debug_log(char const* fmt, ...)
{
	static FILE *glbLogFile = fopen("FullLog.txt", "r+b");
	
	static time_t tmLastDeleteLogTime = time(0);
	static time_t tmLastOpenLogTime = time(0);

	time_t tmCurrent = time(0);
	if (tmCurrent-tmLastDeleteLogTime > 3600)
	{
		tmLastDeleteLogTime = tmCurrent;

		fclose(glbLogFile);
		_unlink("FullLog.txt");
		glbLogFile = fopen("FullLog.txt", "w+b");
	}

	if (tmCurrent-tmLastOpenLogTime > 5)
	{
		tmLastOpenLogTime = tmCurrent;

		fclose(glbLogFile);
		glbLogFile = fopen("FullLog.txt", "r+b");
	}

	if (!glbLogFile)
	{
		glbLogFile = fopen("FullLog.txt", "w+b");
	}
	if (glbLogFile)
		fseek(glbLogFile, 0, SEEK_END);

    va_list ap;
    va_start(ap, fmt);
    const string ret = orm::utils::dupvprintf(fmt, ap);
    va_end(ap);

	time_t tmCurrTime;
	tm *ptmCurrTime;

	time(&tmCurrTime);
	ptmCurrTime = NULL;
#ifdef WIN32
	ptmCurrTime = localtime(&tmCurrTime);
#else
	struct tm *ptm, tmCurrentTime;
	ptm = localtime_r(&tmCurrTime, &tmCurrentTime);
    ptmCurrTime = &tmCurrentTime;
#endif
	
	if (!glbLogFile)
	{
		if (ptmCurrTime)
			printf("%i:%i:%i !!!error at oppening log file!!!", ptmCurrTime->tm_hour, ptmCurrTime->tm_min, ptmCurrTime->tm_sec);
		else
			printf("!!!error at oppening log file!!!\n");
		return;
	}
	if (ptmCurrTime)
	{
		char sz[1000];
		sprintf(sz, "%i:%i:%i ", ptmCurrTime->tm_hour, ptmCurrTime->tm_min, ptmCurrTime->tm_sec);
		fwrite(sz, 1, strlen(sz), glbLogFile);

	}

	fwrite(ret.c_str(), 1, ret.length(), glbLogFile);
	size_t nWrote = fwrite("\n", 1, 1, glbLogFile);
	if (nWrote != 1)
	{
		fclose(glbLogFile);
		glbLogFile = fopen("FullLog.txt", "w+b");
	}
};
#endif
