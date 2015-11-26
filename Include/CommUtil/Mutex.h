/*
 * Mutex.h
 * 
 * 
 * @auth robertxiao
 * @date 2009-09-29
 */

#ifndef	MUTEX_H_
#define	MUTEX_H_


#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>


////////////////////////////////////////////////////////////////////////////////
// CMutex
class CMutex
{
public:	
	CMutex()			{ pthread_mutex_init(&m_Mutex, NULL); }
	~CMutex()			{ pthread_mutex_destroy(&m_Mutex); }
	
	void	Lock()		{ pthread_mutex_lock(&m_Mutex); }
	void	Unlock()	{ pthread_mutex_unlock(&m_Mutex); }
	
private:
	pthread_mutex_t		m_Mutex;
};

////////////////////////////////////////////////////////////////////////////////
// CMutexAuto
class CLockAuto
{
public:	
	CLockAuto(CMutex& mtx) : m_Mutex(mtx) { m_Mutex.Lock(); }
	~CLockAuto() {	m_Mutex.Unlock(); }

private:
	CMutex&		m_Mutex;
};

//////////////////////////////////////////////////////////////////////////
// CRWLock
class CRWLock
{
public:
	CRWLock()			{ pthread_rwlock_init(&m_rwlFile, NULL); }
	~CRWLock()			{ pthread_rwlock_destroy(&m_rwlFile); }

	int		RDLock()	{ return pthread_rwlock_rdlock(&m_rwlFile); }
	int		WRLock()	{ return pthread_rwlock_wrlock(&m_rwlFile); }
	int		UNLock()	{ return pthread_rwlock_unlock(&m_rwlFile); }

private:
	pthread_rwlock_t	m_rwlFile;
};

//////////////////////////////////////////////////////////////////////////
// CSemaphore
class CSemaphore
{
public:
	CSemaphore(int nShare=0, int nValue=0)	{ sem_init(&m_sem, nShare, nValue); }
	~CSemaphore()		{ sem_destroy(&m_sem); }

	int		Wait()		{ return sem_wait(&m_sem); }
	int		TimedWait(long msec)	
	{
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);    
		ts.tv_sec	+= msec/1000;

		long long nsec = ts.tv_nsec + (msec%1000)*1000000;
		if (nsec >=1000000000)
		{
			nsec		%= 1000000000;
			ts.tv_sec	+= 1;
		}		
		ts.tv_nsec = nsec;

		int iResult = 0;
		while ((iResult=sem_timedwait(&m_sem, &ts))==-1 && errno==EINTR)
			;
		return iResult;
	}
	int		Post()	{ return sem_post(&m_sem); }
	int		GetValue(int& nValue) { return sem_getvalue(&m_sem, &nValue); }

private:
	sem_t	m_sem;
};

////////////////////////////////////////////////////////////////////////////////
// CCondition
class CCondition
{
public:
	CCondition()
	{
		pthread_mutex_init(&m_mtx, NULL);
		pthread_cond_init(&m_cnd, NULL);
	}
	~CCondition()
	{
		pthread_cond_destroy(&m_cnd);
		pthread_mutex_destroy(&m_mtx);
	}

	int Wait() 
	{
		pthread_mutex_lock(&m_mtx);
		int nResult = pthread_cond_wait(&m_cnd, &m_mtx); 
		pthread_mutex_unlock(&m_mtx);
		return nResult;
	}

	int		TimedWait(long msec)
	{
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);

		ts.tv_sec	+= (msec / 1000);
		ts.tv_nsec	+= ((msec % 1000) * 1000000);
		pthread_mutex_lock(&m_mtx);
		int nResult = pthread_cond_timedwait(&m_cnd, &m_mtx, &ts);
		pthread_mutex_unlock(&m_mtx);
		return nResult;
	}

	int		Signal() 
	{
		pthread_mutex_lock(&m_mtx);
		int nResult = pthread_cond_signal(&m_cnd); 
		pthread_mutex_unlock(&m_mtx);
	}

	int		Broadcast() 
	{
		pthread_mutex_lock(&m_mtx);
		int nResult = pthread_cond_broadcast(&m_cnd); 
		pthread_mutex_unlock(&m_mtx);
		return nResult;
	}

private:
	pthread_mutex_t	m_mtx;
	pthread_cond_t	m_cnd;
};

#endif	// MUTEX_H_

