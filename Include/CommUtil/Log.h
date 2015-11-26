// Log.h
//
#ifndef	LOG_H
#define	LOG_H

#include <list>
#include "CommUtil/Mutex.h"


enum { LOG_CLOSE, LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG };


//////////////////////////////////////////////////////////////////////////
//CLog
class CLog
{
public:
	enum {BUF_LEN=8192};
	~CLog();

	static CLog*	Instance();
	static void		Release();

public:
	int				Initialize(const char* pszPath, const char* pszName, int iLevel=LOG_INFO);

	void			SetLevel(int iLevel);
	void			SetKeepDays(int iKeepDays);
	void			SetFileSize(long lFileSize);

	void			Write(int iLevel, const char* pszFormat, ...);

private:
	CLog();
	CLog(const CLog&);
	CLog& operator=(const CLog&);

	typedef struct tagNode
	{
		char	pBuf[BUF_LEN];
		int		iLen;
	} NODE;
	typedef std::list<NODE*> LST_NODE;

	NODE*				GetIdle();
	void				SetIdle(NODE* pNode);

	NODE*				GetWork();
	void				SetWork(NODE* pNode);

	void				Close();

private:
	static void*		ThreadExecute(void* pParam);
	void				Execute();

	static void*		ThreadCleanup(void* pParam);
	void				Cleanup();

private:
	static CLog*		m_pThis;
	pthread_t			m_hThread[2];

	char				m_szPath[512];
	char				m_szName[256];

	int					m_iLevel;
	int					m_iKeepDays;
	long				m_lFileSize;

	bool				m_bRun;

	CMutex				m_mtxWork;
	LST_NODE			m_lstWork;

	CMutex				m_mtxIdle;
	LST_NODE			m_lstIdle;	
};

#endif	// LOG_H

