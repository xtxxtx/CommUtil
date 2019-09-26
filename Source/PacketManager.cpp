// PacketManager.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CommUtil/Log.h"
#include "CommUtil/PacketManager.h"

SINGLETON_IMPLEMENT(CPacketManager)

CPacketManager::CPacketManager()
{
}

CPacketManager::~CPacketManager()
{
}

void
CPacketManager::Close()
{
	PACKET* pPacket = NULL;
	m_mtxPacket.Lock();
	DEQ_PACKET::iterator it = m_deqPacket.begin();
	for (; it!=m_deqPacket.end(); it++) {
		pPacket = *it;
		if (pPacket == NULL) {
			continue;
		}
		if (pPacket->pBody != NULL) {
			free(pPacket->pBody);	
		}
		delete pPacket;
	}
	m_deqPacket.clear();
	m_mtxPacket.Unlock();
}

PACKET*
CPacketManager::GetIdle()
{
	PACKET* pPacket = NULL;
	m_mtxPacket.Lock();
	if (!m_deqPacket.empty()) {
		pPacket = m_deqPacket.front();
		m_deqPacket.pop_front();
		m_mtxPacket.Unlock();
		return pPacket;
	}
	m_mtxPacket.Unlock();
	
	pPacket = new PACKET;
	pPacket->cBegin		= FLAG_BEGIN;
	pPacket->iLength	= 0;
	pPacket->iHdrLen	= 0;
	pPacket->iOffset	= 0;
	pPacket->pParent	= NULL;
	pPacket->iSize		= BODY_SIZE;
	pPacket->pBody		= (unsigned char*)malloc(BODY_SIZE);
	if (pPacket->pBody == NULL) {
		delete pPacket;
		pPacket = NULL;
	}
	
	return pPacket;
}

void
CPacketManager::SetIdle(PACKET* pPacket)
{
	if (pPacket == NULL) {
		return;
	}
	
	if (pPacket->iSize > BODY_SIZE) {
		CLog::Instance().Write(LOG_INFO, "release BIG packet. size: %d > %d", pPacket->iSize, BODY_SIZE);	
		free(pPacket->pBody);
		delete pPacket;
		return;
	}
	
	m_mtxPacket.Lock();
	DEQ_PACKET::iterator it = m_deqPacket.begin();
	for (; it!=m_deqPacket.end(); it++) {
		if (*it == pPacket) {
			m_mtxPacket.Unlock();
			CLog::Instance().Write(LOG_WARN, "CPacketManager::SetIdle(%p) repeated.", pPacket);
			return;
		}
	}
	size_t iSize = m_deqPacket.size();
	if (iSize < 1024) {
		pPacket->cBegin		= FLAG_BEGIN;
		pPacket->iLength	= 0;
		pPacket->iHdrLen	= 0;
		pPacket->iOffset	= 0;
		pPacket->pParent	= NULL;
		m_deqPacket.push_back(pPacket);
		m_mtxPacket.Unlock();
		return;
	}
	m_mtxPacket.Unlock();
	
	CLog::Instance().Write(LOG_INFO, "CPacketManager::SetIdle() m_deqPacket.size() %ld", iSize);	
	free(pPacket->pBody);
	delete pPacket;
}

PACKET*
CPacketManager::Clone(PACKET* pPacket)
{
	PACKET* pTemp = GetIdle();
	if (pTemp == NULL) {
		return NULL;
	}
	
	pTemp->cBegin		= pPacket->cBegin;
	pTemp->iLength		= pPacket->iLength;
	pTemp->iFuncCode	= pPacket->iFuncCode;
	pTemp->iSessID		= pPacket->iSessID;
	pTemp->cMode		= pPacket->cMode;
	pTemp->iSerialNumber= pPacket->iSerialNumber;
	pTemp->pParent		= pPacket->pParent;
	pTemp->iHdrLen		= pPacket->iHdrLen;
	pTemp->iOffset		= pPacket->iOffset;
	if (pTemp->iSize < pPacket->iSize) {
		pTemp->iSize = pPacket->iSize;
		free(pTemp->pBody);
		pTemp->pBody = (unsigned char*)malloc(pTemp->iSize);
		if (pTemp->pBody == NULL) {
			delete pTemp;
			return NULL;
		}
	}
	memcpy(pTemp->pBody, pPacket->pBody, pTemp->iLength-PKT_HDR_LEN);
	
	return pTemp;
}

