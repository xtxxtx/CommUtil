// Log.h
//
#ifndef	LOG_H_
#define	LOG_H_

#include <deque>
#include "CommUtil/Mutex.h"


enum { LOG_CLOSE, LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG };


//////////////////////////////////////////////////////////////////////////
//CLog
class CLog
{
public:
	enum {BUF_SIZ = 4096};
	~CLog();

	static CLog&	Instance();

public:
	int				Initialize(const char* pszPath, const char* pszName, int iLevel=LOG_INFO);

	void			SetLevel(int iLevel);
	void			SetKeepDays(int iKeepDays);
	void			SetFileSize(long lFileSize);

	void			Write(int iLevel, const char* pszFormat, ...);
	void			Write(const char* pszText, int iLength, int iLevel);

private:
	CLog();
	CLog(const CLog&);
	CLog& operator=(const CLog&);

	class CNode
	{
	public:
		CNode(int iSize);
		~CNode();
		
		void	Cleanup();
		
		char*	m_pBuf;
		int		m_iLen;
		int		m_iSize;
	private:
		CNode();
		CNode(const CNode&);
		CNode& operator=(const CNode&);
	} ;
	typedef std::deque<CNode *> DEQ_NODE;

	CNode*				GetIdle();
	void				SetIdle(CNode* pNode);

	CNode*				GetWork();
	void				SetWork(CNode* pNode);

	void				Close();

private:
	static void*		ThreadExecute(void* pParam);
	void				Execute();

	static void*		ThreadCleanup(void* pParam);
	void				Cleanup();

private:
	static CLog			m_This;
	pthread_t			m_hThread[2];

	char				m_szPath[512];
	char				m_szName[256];

	int					m_iLevel;
	int					m_iKeepDays;
	long				m_lFileSize;

	bool				m_bRun;

	CMutex				m_mtxWork;
	DEQ_NODE			m_deqWork;

	CMutex				m_mtxIdle;
	DEQ_NODE			m_deqIdle;	
};

#endif	// LOG_H_

