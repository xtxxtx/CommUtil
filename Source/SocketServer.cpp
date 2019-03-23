// SocketServer.cpp


#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#include "CommUtil/Log.h"
#include "CommUtil/SocketListen.h"
#include "CommUtil/SocketClient.h"
#include "CommUtil/SocketServer.h"


static int
OnAccept(void* pHandler, int iFd, const char* pszAddr, unsigned short iPort)
{
	return ((CSocketServer*)pHandler)->Create(iFd, pszAddr, iPort);
}

//////////////////////////////////////////////////////////////////////////
static const int SAIN_SIZE = sizeof(struct sockaddr_in);

CSocketServer::CListen::CListen()
	: m_iFd(-1)
{

}

CSocketServer::CListen::~CListen()
{

}

int
CSocketServer::CListen::Initialize(void* pHandler, CBAccept cbAccept, const char* pszAddr, uint16_t iPort)
{
	if (m_cbAccept == NULL || pszAddr == NULL) {
		return -1;
	}

	m_pHandler = pHandler;
	m_cbAccept = cbAccept;

	if (m_iFd > 0) {
		close(m_iFd);
	}
	m_iFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_iFd < 1) {
		return -1;
	}

	do {
		int iFlag = 1;
		if (setsockopt(m_iFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&iFlag, sizeof(iFlag)) == -1) {
			break;
		}

		struct sockaddr_in sa;
		sa.sin_family = AF_INET;
		sa.sin_port = htons(iPort);
		inet_pton(AF_INET, pszAddr, &(sa.sin_addr));
		sa.sin_zero[0] = 0;

		if (bind(m_iFd, (const sockaddr*)&sa, sizeof(sa)) == -1) {
			CLog::Instance()->Write(LOG_ERROR, "bind(%s:%d) failed. errno: %d", pszAddr, iPort, errno);
			break;
		}

		if (listen(m_iFd, SOMAXCONN) == -1) {
			break;
		}

		return Run(2);
	} while (false);

	close(m_iFd);
	m_iFd = -1;

	return -1;
}

void
CSocketServer::CListen::Close()
{
	if (m_iFd > 0) {
		close(m_iFd);
		m_iFd = -1;
	}

	WaitExit();
}

void
CSocketServer::CListen::Execute()
{
	if (m_cbAccept == NULL) {
		CLog::Instance()->Write(LOG_ERROR, "m_cbAccept is NULL.");
		return;
	}

	const int BUF_SIZ = 64 * 1024;
	int iFd = -1;
	char szAddr[256] = { 0 };
	sockaddr_in sa;
	socklen_t len = SAIN_SIZE;

	//	struct pollfd pfds = {m_iFd, POLLIN, 0};

	for (; m_bRun;) {
		//		if (poll(&pfds, 1, 100) < 1) {
		//			continue;
		//		}

		len = SAIN_SIZE;
		iFd = accept(m_iFd, (sockaddr *)&sa, &len);
		if (iFd == -1 && errno != EAGAIN) {
			CLog::Instance()->Write(LOG_ERROR, "accept() failed. errno: %d.", errno);
			break;
		}

		setsockopt(iFd, SOL_SOCKET, SO_RCVBUF, (const char*)&BUF_SIZ, sizeof(BUF_SIZ));
		setsockopt(iFd, SOL_SOCKET, SO_SNDBUF, (const char*)&BUF_SIZ, sizeof(BUF_SIZ));

		szAddr[0] = 0;
		inet_ntop(AF_INET, &(sa.sin_addr), szAddr, sizeof(szAddr));
		m_cbAccept(m_pHandler, iFd, szAddr, ntohs(sa.sin_port));
	}

	CLog::Instance()->Write(LOG_INFO, "CListen::Execute() exit.");
}

//////////////////////////////////////////////////////////////////////////
CSocketServer::CSocketServer()
{
	m_pHandler	= NULL;
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

	if (m_pListen != NULL) {
		m_pListen->Close();
	}
	else {
		m_pListen = new CListen();
		if (m_pListen == NULL) {
			return -1;
		}
	}

	if (m_pListen->Initialize((void*)this, OnAccept, pszAddr, iPort) == -1) {
		return -1;
	}

	CLog::Instance()->Write(LOG_INFO, "listen on %s:%d ...", pszAddr, iPort);

	int iCount = (lNum > 0 && lNum < 16) ? lNum : 2;

	return Run(iCount);
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
CSocketServer::SetHandler(void* pHandler, CBAccept cbAccept)
{
	m_pHandler	= pHandler;
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
		if (m_deqClient.size() > 0) {
			pHandle = m_deqClient.front();
			m_deqClient.pop_front();

			m_mtxClient.Unlock();

			if (pHandle != NULL && pHandle->Add(iEp) == -1) {
				CLog::Instance()->Write(LOG_WARN, "pHandle->Add(%d) failed.", iEp);
				pHandle->OnClose();
			}
		}
		else {
			m_mtxClient.Unlock();
		}

		nfds = epoll_wait(iEp, ees, EE_SIZE, 100);
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

	if (m_cbAccept(m_pHandler, pClient) == -1) {
		CClientManager::Instance()->SetIdle(pClient);
	} else {
		m_mtxClient.Lock();
		m_deqClient.push_back(pClient);
		m_mtxClient.Unlock();
	}

	return 0;
}
