// SocketServer.cpp


#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#include "CommUtil/Log.h"
#include "CommUtil/SocketClient.h"
#include "CommUtil/SocketServer.h"


//////////////////////////////////////////////////////////////////////////
static const int SAIN_SIZE = sizeof(struct sockaddr_in);

CSocketServer::CListen::CListen(CSocketServer* pServer)
	: m_pServer(pServer), m_iFd(-1)
{

}

CSocketServer::CListen::~CListen()
{

}

int
CSocketServer::CListen::Initialize(const char* pszAddr, uint16_t iPort)
{
	if (m_pServer==NULL || pszAddr==NULL) {
		return -1;
	}

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
			CLog::Instance().Write(LOG_ERROR, "bind(%s:%d) failed. errno: %d", pszAddr, iPort, errno);
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
			CLog::Instance().Write(LOG_ERROR, "accept() failed. errno: %d.", errno);
			break;
		}

		setsockopt(iFd, SOL_SOCKET, SO_RCVBUF, (const char*)&BUF_SIZ, sizeof(BUF_SIZ));
		setsockopt(iFd, SOL_SOCKET, SO_SNDBUF, (const char*)&BUF_SIZ, sizeof(BUF_SIZ));

		szAddr[0] = 0;
		inet_ntop(AF_INET, &(sa.sin_addr), szAddr, sizeof(szAddr));
		m_pServer->OnAccept(iFd, szAddr, ntohs(sa.sin_port));
	}

	CLog::Instance().Write(LOG_INFO, "CListen::Execute() exit.");
}

//////////////////////////////////////////////////////////////////////////
CSocketServer::CClient::CClient()
{
	m_iFd = -1;
	m_iSize = BUF_SIZ;
	m_pBuf = (char*)malloc(BUF_SIZ);

	m_pParent = NULL;
	m_cbReceive = NULL;
	m_cbClose = NULL;
}

CSocketServer::CClient::~CClient()
{
	if (m_pBuf) {
		free(m_pBuf);
		m_pBuf = NULL;
	}
}


int
CSocketServer::CClient::Initialize(int iFd, const char* pszAddr, uint16_t iPort)
{
	if (iFd<1 || pszAddr == NULL) {
		return -1;
	}
	
	m_iFd = iFd;
	strcpy(m_szAddr, pszAddr);
	m_iPort = iPort;

	int iFlag = fcntl(iFd, F_GETFL, 0);
	fcntl(iFd, F_SETFL, iFlag | O_NONBLOCK);

	return 0;
}

int
CSocketServer::CClient::OnRecv(void* pHander, char* pBuf, int iSize)
{
	if (m_iFd == -1) {
		return -1;
	}

	int iResult = 0;

TAG_BEGIN:
	iResult = recv(m_iFd, pBuf, iSize, 0);

	if (iResult > 0) {
		if (m_cbReceive) {
			if (m_cbReceive(m_pParent, m_pBuf, iResult) == -1) {
				return -1;
			}
		}
		if (iResult == iSize) {
			goto TAG_BEGIN;
		}
	}
	else {
		if (m_cbClose) {
			m_cbClose(pHander, this, iResult);
		}

		if (iResult==-1 && errno==EAGAIN) {
			goto TAG_BEGIN;
		}
		else {
			return -1;
		}
	}

	return 0;
}

void
CSocketServer::CClient::Close()
{
	if (m_iFd > 0) {
		Del();

		close(m_iFd);
		m_iFd = -1;
	}
}

//////////////////////////////////////////////////////////////////////////
CSocketServer::CSocketServer()
{
	m_pHandler	= NULL;
	m_cbConnect = NULL;
	m_pListen	= NULL;
}

CSocketServer::~CSocketServer()
{
	
}

int
CSocketServer::Initialize(const char* pszAddr, uint16_t iPort, long lNum, void* pHandler,
	CBConnect cbConnect, CBReceive cbReceive, CBClose cbClose)
{
	if (pszAddr == NULL) {
		return -1;
	}

	m_pHandler = pHandler;
	m_cbConnect = cbConnect;
	m_cbReceive = cbReceive;
	m_cbClose = cbClose;

	CClientManager::Instance();

	if (m_pListen != NULL) {
		m_pListen->Close();
	}
	else {
		m_pListen = new CListen(this);
		if (m_pListen == NULL) {
			return -1;
		}
	}

	if (m_pListen->Initialize(pszAddr, iPort) == -1) {
		return -1;
	}

	int iCount = (lNum > 0 && lNum < 32) ? lNum : 2;

	CLog::Instance().Write(LOG_INFO, "listen on %s:%d ...... use threads: %d", pszAddr, iPort, iCount);

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

	CClient* pClient = NULL;
	m_mtxClientAll.Lock();
	LST_CLIENT::iterator it = m_lstClientAll.begin();
	for (; it != m_lstClientAll.end(); it++) {
		pClient = *it;
		pClient->Close();
		delete(pClient);
	}
	m_lstClientAll.clear();
	m_mtxClientAll.Unlock();

	CClientManager::Release();

	CLog::Instance().Write(LOG_INFO, "CSocketServer::Close().");
}

void
CSocketServer::Execute()
{
	const int EPOLL_MAX_FDSIZE = 0x4000;
	int iEp = epoll_create(EPOLL_MAX_FDSIZE);
	if (iEp == -1) {
		return;
	}

	static const int BUF_SIZ = 32768;
	char szBuf[BUF_SIZ] = { 0 };

	const int EE_SIZE = 256;
	struct epoll_event ees[EE_SIZE];
	int iResult			= 0;
	int nfds			= 0;
#if 0
	IHandle* pHandle	= NULL;

	for (; m_bRun; ) {
		m_mtxClient.Lock();
		if (m_lstClient.size() > 0) {
			pHandle = m_lstClient.front();
			m_lstClient.pop_front();

			m_mtxClient.Unlock();

			if (pHandle != NULL && pHandle->Add(iEp) == -1) {
				CLog::Instance().Write(LOG_WARN, "pHandle->Add(%d) failed.", iEp);
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
			CLog::Instance().Write(LOG_ERROR, "epoll_wait() failed. errno: %d.", errno);
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
#else
	CClient* pClient = NULL;

	for (; m_bRun;) {
		m_mtxClientNew.Lock();
		if (m_lstClientNew.size() > 0) {
			pClient = m_lstClientNew.front();
			m_lstClientNew.pop_front();

			m_mtxClientNew.Unlock();

			if (pClient != NULL && pClient->Add(iEp) == -1) {
				CLog::Instance().Write(LOG_WARN, "pHandle->Add(%d) failed.", iEp);
				pClient->Close();
			}
		}
		else {
			m_mtxClientNew.Unlock();
		}

		nfds = epoll_wait(iEp, ees, EE_SIZE, 100);
		if (nfds == 0) {
			continue;
		}
		if (nfds == -1) {
			if (errno == EINTR) {
				continue;
			}
			CLog::Instance().Write(LOG_ERROR, "epoll_wait() failed. errno: %d.", errno);
			break;
		}

		for (int i = 0; i<nfds; i++) {
			pClient = (CClient *)(ees[i].data.ptr);
			if (pClient == NULL) {
				continue;
			}
			if (ees[i].events & EPOLLERR || ees[i].events & EPOLLHUP) {
				pClient->Close();
				continue;
			}

			if (ees[i].events & EPOLLIN) {
				iResult = pClient->OnRecv(m_pHandler, szBuf, BUF_SIZ);

				if (iResult == 0) {
					continue;
				}

				if (iResult < 0) {
					pClient->Close();

					m_mtxClientAll.Lock();
					m_lstClientAll.remove(pClient);
					m_mtxClientAll.Unlock();

					delete pClient;
				}
			}

			if (ees[i].events & EPOLLOUT) {
			//	iResult = pClient->OnSend();
			//	if (iResult == 0) {
			//		continue;
			//	}

			//	pClient->OnClose();
			}
		}
	}
#endif
	close(iEp);
}

int
CSocketServer::OnAccept(int iFd, const char* pszAddr, uint16_t iPort)
{
	if (!m_bRun || pszAddr == NULL || m_cbConnect == NULL) {
		close(iFd);
		return -1;
	}
#if 0
	CSocketClient* pClient = CClientManager::Instance()->GetIdle();
	
	if (pClient == NULL) {
		close(iFd);
		return -1;
	}

	CLog::Instance().Write(LOG_INFO, "[%5d] client(%s:%d) connected.", iFd, pszAddr, iPort);
	
	pClient->Initialize(iFd, pszAddr, iPort);

	if (m_cbConnect(m_pHandler, pClient) == -1) {
		CClientManager::Instance()->SetIdle(pClient);
	} else {
		m_mtxClient.Lock();
		m_lstClient.push_back(pClient);
		m_mtxClient.Unlock();
	}
#else
	CClient* pClient = new CClient();
	if (pClient == NULL) {
		close(iFd);
		return -1;
	}

	void* pParent = m_cbConnect(m_pHandler, pClient);
	if (pParent == NULL) {
		delete pClient;
	}
	else {
		pClient->Set(iFd);
		pClient->Set(pParent);

		m_mtxClientNew.Lock();
		m_lstClientNew.push_back(pClient);
		m_mtxClientNew.Unlock();

		m_mtxClientAll.Lock();
		m_lstClientAll.push_back(pClient);
		m_mtxClientAll.Unlock();
	}
#endif
	return 0;
}

#include <algorithm>
void
CSocketServer::Close(void* pHandler, void* pClient)
{
	if (pClient == NULL) {
		return;
	}

	m_mtxClientAll.Lock();
	if (std::find(m_lstClientAll.begin(), m_lstClientAll.end(), (CClient*)pClient) == m_lstClientAll.end()) {
		m_mtxClientAll.Unlock();
		return;
	}
	m_mtxClientAll.Unlock();

	((CClient*)pClient)->Close();
}

