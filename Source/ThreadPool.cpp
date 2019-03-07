// ThreadPool.cpp
//
#include "CommUtil/ThreadPool.h"


CThreadPool*
CThreadPool::m_pThis = NULL;

CThreadPool*
CThreadPool::Instance()
{
	static CThreadPool* pThis = new CThreadPool();
	if (m_pThis == NULL) {
		m_pThis = pThis;
	}
	return m_pThis;
}

void
CThreadPool::Release()
{
	if (m_pThis != NULL) {
		m_pThis->Close();
		delete m_pThis;
		m_pThis = NULL;
	}
}

CThreadPool::CThreadPool()
{

}

CThreadPool::~CThreadPool()
{

}


