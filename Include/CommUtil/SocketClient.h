#ifndef	SOCKETCLIENT_H_
#define	SOCKETCLIENT_H_

#include <deque>

#include "Mutex.h"
#include "SocketHandle.h"
#include "PacketManager.h"

typedef	void (*CBPush)(void*, PACKET*);

////////////////////////////////////////////////////////
class CSocketClient : public IHandle
{
	enum { BUF_SIZ=8192 };
	
public:
	CSocketClient();
	~CSocketClient();
};

#endif	// SOCKETCLIENT_H_

