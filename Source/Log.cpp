// Log.cpp

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include "CommUtil/OSTime.h"
#include "CommUtil/Log.h"

CLog*
CLog::m_pThis	= NULL;

CLog*
CLog::Instance()
{
	static CLog* pLog = new CLog();
	if (m_pThis == NULL) {
		m_pThis = pLog;
	}

	return m_pThis;
}

void
CLog::Release()
{
	if (m_pThis == NULL) {
		return;
	}

	m_pThis->Close();
	delete m_pThis;
	m_pThis	= NULL;
}

CLog::CLog()
{
	m_szPath[0]		= 0;
	m_szName[0]		= 0;

	m_iLevel		= LOG_INFO;
	m_iKeepDays		= 30;
	m_lFileSize		= 4*1024*1024;

	m_bRun			= false;
	memset(m_hThread, 0, sizeof(m_hThread));
}

CLog::~CLog()
{

}

int
CLog::Initialize(const char* pszPath, const char* pszName, int iLevel)
{
	if (pszPath==NULL || pszPath[0]==0 || pszName==NULL) {
		return -1;
	}

	if (pszPath[strlen(pszPath)-1] == '/') {
		snprintf(m_szPath, sizeof(m_szPath), "%s", pszPath);
	} else {
		snprintf(m_szPath, sizeof(m_szPath), "%s/", pszPath);
	}

	strncpy(m_szName, pszName, sizeof(m_szName));

	SetLevel(iLevel);

	m_bRun	= true;

	int result = 0;
	
	result = pthread_create(m_hThread+0, NULL, ThreadExecute, this);
	result = pthread_create(m_hThread+1, NULL, ThreadCleanup, this);

	return 0;
}

void
CLog::Close()
{
	m_bRun	= false;

	for (int i=0; i<2; i++) {
		if (m_hThread[i] != 0) {
			pthread_join(m_hThread[i], NULL);
			m_hThread[i] = 0;
		}
	}

	m_mtxIdle.Lock();
	CNode* pNode = NULL;
	while (!m_lstIdle.empty()) {
		pNode = m_lstIdle.front();
		m_lstIdle.pop_front();
		delete pNode;
	}
	m_mtxIdle.Unlock();	
}

void
CLog::SetLevel(int iLevel)
{
	if (iLevel > LOG_DEBUG) {
		m_iLevel	= LOG_DEBUG;
	} else if (iLevel < LOG_ERROR) {
		m_iLevel	= LOG_CLOSE;
	} else {
		m_iLevel	= iLevel;
	}
}

void
CLog::SetKeepDays(int iKeepDays)
{
	m_iKeepDays = iKeepDays>0 ? iKeepDays:1;
}

void
CLog::SetFileSize(long lFileSize)
{
	m_lFileSize = lFileSize>4096 ? lFileSize:(4*1024*1024);
}

CLog::CNode*
CLog::GetIdle(int iSize)
{
	if (iSize > BUF_LEN) {
		return new CNode(iSize);
	}

	CNode* pNode = NULL;
	m_mtxIdle.Lock();
	if (m_lstIdle.empty()) {
		pNode = new CNode(BUF_LEN);
	} else {
		pNode = m_lstIdle.front();
		m_lstIdle.pop_front();
	}
	m_mtxIdle.Unlock();

	if (pNode != NULL) {
		memset(pNode->m_pBuf, 0, BUF_LEN);
		pNode->m_iLen = 0;
	}

	return pNode;
}

void
CLog::SetIdle(CNode* pNode)
{
	if (pNode == NULL) {
		return;
	}

	m_mtxIdle.Lock();
	if (m_lstIdle.size() > 64) {
		delete pNode;
	} else {
		m_lstIdle.push_back(pNode);
	}
	m_mtxIdle.Unlock();
}

CLog::CNode*
CLog::GetWork()
{
	CNode* pNode = NULL;

	m_mtxWork.Lock();
	if (!m_lstWork.empty()) {
		pNode = m_lstWork.front();
		m_lstWork.pop_front();
	}
	m_mtxWork.Unlock();

	return pNode;
}

void
CLog::SetWork(CNode* pNode)
{
	if (pNode == NULL) {
		return;
	}

	m_mtxWork.Lock();
	m_lstWork.push_back(pNode);
	m_mtxWork.Unlock();
}

static const char LOG_TYPE[][8] = {"ERROR", "WARN", "INFO", "DEBUG" };
void
CLog::Write(int iLevel, const char* pszFormat, ...)
{
	if (iLevel<LOG_ERROR || iLevel>LOG_DEBUG || !m_bRun) {
		return;
	}

	CNode* pNode = GetIdle(BUF_LEN);
	if (pNode == NULL) {
		return;
	}

	struct timeval tv = {0};
	gettimeofday(&tv, NULL);

	struct tm tmCurr = {0};
	localtime_r(&(tv.tv_sec), &tmCurr);

	int iLen = sprintf(pNode->m_pBuf, "%02d-%02d %02d:%02d:%02d.%03d %s ",
		tmCurr.tm_mon+1, tmCurr.tm_mday, tmCurr.tm_hour, tmCurr.tm_min, tmCurr.tm_sec, tv.tv_usec/1000, LOG_TYPE[iLevel-1]);

	static const int MAX_LEN = BUF_LEN-2;

	va_list args;
	va_start(args, pszFormat);
	iLen += vsprintf(pNode->m_pBuf+iLen, pszFormat, args);
	va_end(args);

	if (iLen > MAX_LEN) {
		iLen = MAX_LEN;
	}

	pNode->m_pBuf[iLen++]	= '\n';
	pNode->m_pBuf[iLen++]	= '\0'; 
	pNode->m_iLen			= iLen;

	SetWork(pNode);
}

void
CLog::Write(const char* pszText, int iLength, int iLevel)
{
	if (m_iLevel<LOG_ERROR || m_iLevel<iLevel || !m_bRun || pszText==NULL || iLength<1) {
		return;
	}

	CNode* pNode = GetIdle(iLength+32);
	if (pNode == NULL) {
		return;
	}

	struct timeval tv = {0};
	gettimeofday(&tv, NULL);
	struct tm dt	= {0};
	localtime_r(&(tv.tv_sec), &dt);

	pNode->m_iLen = sprintf(pNode->m_pBuf, "%02d-%02d %02d:%02d:%02d.%03ld %s %s\n",
		dt.tm_mon+1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec, tv.tv_usec/1000, LOG_TYPE[iLevel-1], pszText);

	SetWork(pNode);
}

void*
CLog::ThreadExecute(void* pParam)
{
	if (pParam != NULL) {
		((CLog *)pParam)->Execute();
	}

	return 0;
}


void
CLog::Execute()
{
	FILE* pFile		= NULL;
	CNode* pNode	= NULL;

	long lSize		= 0;
	int iIndex		= 0;

	time_t tTime	= 0;
	struct tm tmCurr	= {0};
	struct tm tmPrev	= {0};

	for (; m_bRun; ) {
		pNode = GetWork();
		if (pNode == NULL) {
			NS_TIME::MSleep(50);
			continue;
		}

		if (pNode->m_pBuf[0]==0 || pNode->m_iLen<1) {
			SetIdle(pNode);
			continue;
		}

		time(&tTime);
		localtime_r(&tTime, &tmCurr);

		if (tmCurr.tm_mday!=tmPrev.tm_mday || tmCurr.tm_mon!=tmPrev.tm_mon) {
			tmPrev = tmCurr;
			iIndex = 0;
			if (pFile != NULL) {
				fclose(pFile);
				pFile = NULL;
			}
		}

		if (pFile == NULL) {
			char szPath[512]	= {0};
			struct stat st		= {0};
			do {
				sprintf(szPath, "%s%s_%04d-%02d-%02d.%03d.log",
					m_szPath, m_szName, tmCurr.tm_year+1900, tmCurr.tm_mon+1, tmCurr.tm_mday, iIndex++);
				if (stat(szPath, &st) == 0) {
					lSize = st.st_size;
				} else {
					lSize = 0;
				}
			} while (lSize >= m_lFileSize);
	
			pFile = fopen(szPath, "a");
			if (pFile == NULL) {
				SetIdle(pNode);
				NS_TIME::MSleep(100);
				continue;
			}
		}

		if (fwrite(pNode->m_pBuf, 1, pNode->m_iLen, pFile) > 0) {
			lSize += pNode->m_iLen;
			fflush(pFile);
		} else {
			fclose(pFile);
			pFile = NULL;
		}

		SetIdle(pNode);

		if (lSize >= m_lFileSize) {
			fclose(pFile);
			pFile = NULL;
		}
	}	

	if (pFile != NULL) {
		fclose(pFile);
	}
}

void*
CLog::ThreadCleanup(void* pParam)
{
	if (pParam != NULL) {
		((CLog *)pParam)->Cleanup();
	}

	return 0;
}

void
CLog::Cleanup()
{
	const time_t SECOFDAY = 24*60*60;
	const int OFFSET	= strlen(m_szName)+1;

	DIR* dir			= NULL;
	struct dirent* de	= NULL;
	
	time_t tDiff		= m_iKeepDays * SECOFDAY;
	time_t tTime		= 0;
	struct tm tmTemp	= {0};
	int iYear=0, iMon=0, iDay=0;

	for (int i=0; m_bRun; i++) {
		if (i < 1200) {
			NS_TIME::MSleep(100);
			continue;
		}

		dir = opendir(m_szPath);
		if (dir == NULL) {
			continue;
		}

		tDiff = m_iKeepDays * SECOFDAY;

		time(&tTime);

		while ((de=readdir(dir)) != NULL) {
			if (de->d_reclen<OFFSET) {
				continue;
			}

			if (sscanf(de->d_name+OFFSET, "%d-%d-%d", &iYear, &iMon, &iDay) != 3) {
				continue;
			}

			tmTemp.tm_year	= iYear-1900;
			tmTemp.tm_mon	= iMon-1;
			tmTemp.tm_mday	= iDay;

			if (tTime - mktime(&tmTemp) > tDiff) {
				char szFile[512]	= {0};
				sprintf(szFile, "%s%s", m_szPath, de->d_name);
				if (remove(szFile) == 0) {
					Write(LOG_INFO, "remove(%s) successful.\n", szFile);
				} else {
					Write(LOG_WARN, "remove(%s) failed. errno: %d\n", szFile, errno);
				}
			}
		}

		closedir(dir);
	}
}

