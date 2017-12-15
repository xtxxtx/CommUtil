// SocketServer.cpp


#include <stdio.h>
#include <unistd.h>

#include "CommUtil/Log.h"
#include "CommUtil/SocketListen.h"
#include "CommUtil/SocketClient.h"
#include "CommUtil/SocketServer.h"



CSocketServer::CSocketServer()
{
	m_pHandle	= NULL;
	m_cbAccept	= NULL;
	m_pListen	= NULL;
}

CSocketServer::~CSocketServer()
{
	
}

int
CSocketServer::Initialize(const char* pszAddr, uint16_t iPort, long lNum)
{
	CClientManager::Instance();

	m_pListen = new CSocketListen(this);
	if (m_pListen == NULL) {
		return -1;
	}

	if (m_pListen->Initialize(pszAddr, iPort) == -1) {
		return -1;
	}

	CLog::Instance()->Write(LOG_INFO, "listen on %s:%d ...", pszAddr, iPort);

	return Run(2);
}

void
CSocketServer::Close()
{
	if (m_pListen != NULL) {
		m_pListen->Close();
		delete m_pListen;
		m_pListen = NULL;
	}

	WaitExit();

	CSocketClient* pClient = NULL;
	m_mtxClient.Lock();
	DEQ_CLIENT::iterator it = m_deqClient.begin();
	for (; it!=m_deqClient.end(); it++) {
		pClient = *it;
		pClient->Close();
		delete(pClient);
	}
	m_deqClient.clear();
	m_mtxClient.Unlock();

	CClientManager::Release();

	CLog::Instance()->Write(LOG_INFO, "CSocketServer::Close().");
}

void
CSocketServer::SetHandle(void* pHandle, CBAccept cbAccept)
{
	m_pHandle	= pHandle;
	m_cbAccept	= cbAccept;
}

void
CSocketServer::Execute()
{
	const int EPOLL_MAX_FDSIZE = 0x4000;
	int iEp = epoll_create(EPOLL_MAX_FDSIZE);
	if (iEp == -1) {
		return;
	}

	const int EE_SIZE = 256;
	struct epoll_event ees[EE_SIZE];
	int iResult			= 0;
	int nfds			= 0;
	IHandle* pHandle	= NULL;

	for (; m_bRun; ) {
		m_mtxClient.Lock();
		for (; m_deqClient.size() > 0; ) {
			pHandle = m_deqClient.front();
			m_deqClient.pop_front();

			if (pHandle == NULL) {
				continue;
			}

			if (pHandle->Add(iEp) == -1) {
				CLog::Instance()->Write(LOG_WARN, "pHandle->Add(%d) failed.", iEp);
			} else {
				break;
			}
		}
		m_mtxClient.Unlock();

		nfds = epoll_wait(iEp, ees, EE_SIZE, 50);
		if (nfds == 0) {
			continue;
		}
		if (nfds == -1) {
			if (errno == EINTR) {
				continue;
			}
			CLog::Instance()->Write(LOG_ERROR, "epoll_wait() failed. errno: %d.", errno);
			break;
		}

		for (int i=0; i<nfds; i++) {
			pHandle = (IHandle *)(ees[i].data.ptr);
			if (pHandle == NULL) {
				continue;
			}
			if (ees[i].events & EPOLLERR || ees[i].events & EPOLLHUP) {
				pHandle->OnClose();
				continue;
			}

			if (ees[i].events & EPOLLIN) {
				iResult = pHandle->OnRecv();
				if (iResult == 0) {
					continue;
				}

				if (iResult < 0) {
					pHandle->OnClose();
				}
			}

			if (ees[i].events & EPOLLOUT) {
				iResult = pHandle->OnSend();
				if (iResult == 0) {
					continue;
				}

				pHandle->OnClose();
			}
		}
	}

	close(iEp);
}

int
CSocketServer::Create(int iFd, const char* pszAddr, uint16_t iPort)
{
	if (!m_bRun || pszAddr==NULL || m_cbAccept==NULL) {
		close(iFd);
		return -1;
	}

	CSocketClient* pClient = CClientManager::Instance()->GetIdle();
	if (pClient == NULL) {
		close(iFd);
		return -1;
	}

	CLog::Instance()->Write(LOG_INFO, "[%5d] client(%s:%d) connected.", iFd, pszAddr, iPort);
	
	pClient->Initialize(iFd, pszAddr, iPort);

	if (m_cbAccept(m_pHandle, pClient) == -1) {
		CClientManager::Instance()->SetIdle(pClient);
	} else {
		m_mtxClient.Lock();
		m_deqClient.push_back(pClient);
		m_mtxClient.Unlock();
	}

	return 0;
}

