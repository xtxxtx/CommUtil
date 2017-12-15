// Thread.h

#ifndef	THREAD_H_
#define	THREAD_H_

#ifdef	_MSC_VER
#else
#include <pthread.h>
#endif

class IThread
{
public:
	IThread();
	~IThread();
	
	int				Run(int iSize=1);
	
	void*			Get()		{ return m_pParam; }
	void			Set(void* pParam) { m_pParam = pParam; }
	
	void			WaitExit();
	
protected:
	virtual void	Execute() = 0;
	
private:
	static void*	Execute(void* pParam);
	
protected:
	bool			m_bRun;
	
private:
	int				m_iSize;
	pthread_t*		m_pThread;
	
	void*			m_pParam;
};

#endif	// THREAD_H_

