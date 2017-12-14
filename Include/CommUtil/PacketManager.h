// PacketManager.h

#ifndef	PACKETMANAGER_H_
#define	PACKETMANAGER_H_


#include <deque>

#include "CommUtil/Singleton.h"
#include "Mutex.h"

#define	FLAG_BEGIN		(unsigned char)0xFF
#define	FLAG_END		(unsigned char)0x00

#pragma pack(1)

typedef struct tagPacket {
	unsigned char	cBegin;
	int				iLength;
} PACKET;
#pragma pack()

#define	PKT_HDR_LEN		19

typedef std::deque<PACKET *>	DEQ_PACKET;

/////////////////////////////////////////////////
class CPacketManager
{
	SINGLETON_DECLARE(CPacketManager);
	enum { BODY_SIZE = 8192 };
public:
	~CPacketManager();
	
	PACKET*			GetIdle();
	void			SetIdle(PACKET* pPacket);
	
	PACKET*			Clone(PACKET* pPacket);
	
private:
	CPacketManager();
	
	void			Close();
	
private:
	CMutex			m_mtxPacket;
	DEQ_PACKET		m_deqPacket;
};

#endif	// PACKETMANAGER_H_

