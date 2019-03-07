// ThreadPool.h
//
#ifndef	THREADPOOL_H_
#define	THREADPOOL_H_


#include <pthread.h>
#include <deque>

#include "CommUtil/Mutex.h"

//////////////////////////////////////////////////////////////////////////
class IWork
{
public:
	IWork()		{}
	~IWork()	{}

	virtual void	Execute(void* pParam) = 0;

	int				Run(void* pParam);
};

typedef void (*CBExec)(void*);

//////////////////////////////////////////////////////////////////////////
class CThreadPool
{
public:
	static CThreadPool*	Instance();
	static void			Release();

	~CThreadPool();

	int					Initialize(int iCount);

	int					Add(CBExec cbExec, void* pParam);
	int					Add(IWork* pWork, void* pParam);

private:
	class CTask
	{
	public:
		CTask() : m_cbExec(NULL), m_pParam(NULL)	{}
		~CTask()	{}

		void			Set(CBExec cbExec, void* pParam) {
			m_cbExec	= cbExec;
			m_pParam	= pParam;
		}
	private:
		CBExec			m_cbExec;
		void*			m_pParam;
	};
	typedef std::deque<CTask *>		DEQ_TASK;

	class CObject
	{
	public:
		CObject(CThreadPool* pPool) : m_pPool(pPool)	{}
		~CObject()	{}

		int				Initialize(IWork* pWork, void* pParam);

		void			Execute();

	private:
		CThreadPool*	m_pPool;
		IWork*			m_pWork;
		void*			m_pParam;
	};
	typedef std::deque<CObject *>	DEQ_OBJECT;

	class CThread
	{
	public:
		CThread(CThreadPool* pPool) : m_pPool(pPool) {}
		~CThread();

		CThreadPool*	m_pPool;
		pthread_t		m_hThread;
		int				m_iStatus;
	};
	typedef std::deque<CThread *>	DEQ_THREAD;

private:
	CThreadPool();
	CThreadPool(const CThreadPool&);
	CThreadPool& operator=(const CThreadPool&);

	CObject*			GetObject();
	void				SetObject(CObject* pObject);

	void				Close();

private:
	static CThreadPool*	m_pThis;
	bool				m_bRun;

	CMutex				m_mtxObject;
	DEQ_OBJECT			m_deqObject;

	CMutex				m_mtxThread;
	DEQ_THREAD			m_deqThread;

	CMutex				m_mtxIdle;
	DEQ_TASK			m_deqIdle;

	CMutex				m_mtxWork;
	DEQ_TASK			m_deqWork;
};

#endif	// THREADPOOL_H_

