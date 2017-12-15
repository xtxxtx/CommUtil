// SocketClient.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "CommUtil/OSTime.h"
#include "CommUtil/Log.h"
#include "CommUtil/SocketClient.h"


CSocketClient::CSocketClient()
{
	m_pHandle	= NULL;
	m_cbPush	= NULL;
	m_pPacket	= NULL;

	m_szAddr[0]	= 0;
	m_iPort		= 0;
}

CSocketClient::~CSocketClient()
{
	if (m_pPacket != NULL) {
		CPacketManager::Instance()->SetIdle(m_pPacket);
	}
}

int
CSocketClient::Initialize(int iFd, const char* pszAddr, uint16_t iPort)
{
	if (iFd<1 || pszAddr==NULL) {
		return -1;
	}

	m_pPacket = CPacketManager::Instance()->GetIdle();
	if (m_pPacket == NULL) {
		return -1;
	}
	m_pPacket->pParent = this;

	m_iFd = iFd;
	strcpy(m_szAddr, pszAddr);
	m_iPort = iPort;

	return 0;
}

void
CSocketClient::Close()
{
	CLog::Instance()->Write(LOG_INFO, "CSocketClient::Close(%d).", m_iFd);

	OnClose();

	if (m_pPacket != NULL) {
		CPacketManager::Instance()->SetIdle(m_pPacket);
		m_pPacket = NULL;
	}

	m_szAddr[0]	= 0;
	m_iPort		= 0;
}

int
CSocketClient::OnRecv()
{
	if (m_iFd == -1) {
		return -1;
	}

	if (m_pPacket == NULL) {
		m_pPacket = CPacketManager::Instance()->GetIdle();
		if (m_pPacket == NULL) {
			CLog::Instance()->Write(LOG_WARN, "CPacketManager::Instance()->GetIdle() is NULL.");
			return -1;
		}
		m_pPacket->pParent = this;
	}

	int iResult = 0;
	if (m_pPacket->iHdrLen < PKT_HDR_LEN) {
		iResult = recv(m_iFd, ((char*)m_pPacket)+m_pPacket->iHdrLen, PKT_HDR_LEN-m_pPacket->iHdrLen, 0);	
		if (iResult > 0) {
			if (m_pPacket->cBegin != FLAG_BEGIN) {
				CLog::Instance()->Write(LOG_WARN, "invalid head flag.");
				m_pPacket->iHdrLen = 0;
				return -1;
			}
			m_pPacket->iHdrLen += iResult;
			if (m_pPacket->iHdrLen < PKT_HDR_LEN) {
				return 0;
			}
		} else {
			if (iResult == 0) {

			}
		}
	}

	if (m_cbPush) {
		m_cbPush(m_pHandle, m_pPacket);
		m_pPacket = NULL;
	}

	return 0;
}

int
CSocketClient::Send(const char* pszBuf, int iLen, long lTimeout)
{
	if (m_iFd == -1) {
		return -1;
	}

	int iOff = 0;
	long lSleep = 50;
	for (int iResult=0; iOff<iLen; ) {
		iResult = send(m_iFd, pszBuf+iOff, iLen-iOff, MSG_DONTWAIT);
		if (iResult == -1) {
			if (errno==EAGAIN || errno==EWOULDBLOCK) {
				CLog::Instance()->Write(LOG_WARN, "send to (%s:%d) timeout: %d. left data: %d",
					m_szAddr, m_iPort, errno, iLen-iOff);
				if (lSleep > lTimeout) {
					lSleep = lTimeout;
				}
				NS_TIME::MSleep(lSleep);
				lTimeout -= lSleep;
				if (lTimeout > 0) {
					continue;
				} else {
					break;
				}
			} else {
				CLog::Instance()->Write(LOG_ERROR, "send to (%s:%d) failed. errno: %d", m_szAddr, m_iPort, errno);
				return -1;
			}
		} else {
			iOff += iResult;
		}
	}

	return iOff;
}

//////////////////////////////////////////////////////////////////////////
CClientManager*
CClientManager::m_pThis = NULL;

CClientManager*
CClientManager::Instance()
{
	static CClientManager* pThis = new CClientManager();
	if (m_pThis == NULL) {
		m_pThis = pThis;
	}
	return m_pThis;
}

void
CClientManager::Release()
{
	if (m_pThis != NULL) {
		m_pThis->Close();
		delete m_pThis;
		m_pThis = NULL;
	}
}

CSocketClient*
CClientManager::GetIdle()
{
	CSocketClient* pClient = NULL;
	m_mtxClient.Lock();
	if (m_deqClient.size() < 1) {
		pClient = new CSocketClient();
	} else {
		pClient = m_deqClient.front();
		m_deqClient.pop_front();
	}
	m_mtxClient.Unlock();

	return pClient;
}

void
CClientManager::SetIdle(CSocketClient* pClient)
{
	if (pClient == NULL) {
		return;
	}

	pClient->Close();
	m_mtxClient.Lock();
	DEQ_CLIENT::iterator it = m_deqClient.begin();
	for (; it!=m_deqClient.end(); it++) {
		if (*it == pClient) {
			m_mtxClient.Unlock();
			CLog::Instance()->Write(LOG_WARN, "CClientManager::SetIdle(%p) is exist!!!", pClient);
			return;
		}
	}

	DEQ_CLIENT::size_type iSize = m_deqClient.size();
	CLog::Instance()->Write(LOG_INFO, "CClientManager::SetIdle(%p) size: %ld.", pClient, iSize);
	if (iSize < 1024) {
		m_deqClient.push_back(pClient);
		m_mtxClient.Unlock();
	} else {
		m_mtxClient.Unlock();
		delete pClient;
	}
}

void
CClientManager::Close()
{
	m_mtxClient.Lock();
	CSocketClient* pClient = NULL;
	DEQ_CLIENT::iterator it = m_deqClient.begin();
	for (; it!=m_deqClient.end(); it++) {
		pClient = *it;
		pClient->Close();
		delete pClient;
	}
	m_deqClient.clear();
	m_mtxClient.Unlock();
}

