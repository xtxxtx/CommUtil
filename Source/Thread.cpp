// Thread.cpp
//

#include <stdlib.h>

#include "CommUtil/Thread.h"

IThread::IThread()
{
	m_bRun		= false;
	m_iSize		= 1;
	m_pThread	= NULL;
}

IThread::~IThread()
{
	
}

int
IThread::Run(int iSize)
{
	if (iSize<1 || iSize>4096)	{
		m_iSize = 1;
	} else {
		m_iSize = iSize;
	}
	
	m_bRun = true;
	
	if (m_pThread) {
		free(m_pThread);	
	}
	m_pThread = (pthread_t *)calloc(m_iSize, sizeof(pthread_t*));
	
	int iOffset = 0;
	for (int i=0; i<m_iSize; i++) {
		if (pthread_create(m_pThread+iOffset, NULL, Execute, this) == 0) {
			iOffset++;	
		}
	}
	
	return iOffset;
}

void*
IThread::Execute(void* pParam)
{
	if (pParam != NULL) {
		((IThread*)pParam)->Execute();	
	}
	return 0;
}

void
IThread::WaitExit()
{
	m_bRun = false;
	if (m_pThread != NULL) {
		for (int i=0; i<m_iSize; i++) {
			if (m_pThread[i] != 0) {
				pthread_join(m_pThread[i], NULL);	
			}
		}
		
		free(m_pThread);
		m_pThread = NULL;
	}
}

