#ifndef	SOCKETCLIENT_H_
#define	SOCKETCLIENT_H_

#include <deque>

#include "CommUtil/Mutex.h"
#include "CommUtil/SocketHandle.h"
#include "CommUtil/PacketManager.h"


typedef	void (*CBPush)(void*, PACKET*);

//////////////////////////////////////////////////////////////////////////
class CSocketClient : public IHandle
{
	enum { BUF_SIZ=8192 };
	
public:
	CSocketClient();
	~CSocketClient();

	int				Initialize(int iFd, const char* pszAddr, uint16_t iPort);

	void			SetHandle(void* pHandle, CBPush cbPush) { m_pHandle=pHandle; m_cbPush=cbPush; }

	int				Send(const char* pszBuf, int iLen, long lTimeout);

	const char*		GetAddr()	{ return m_szAddr; }
	uint16_t		GetPort()	{ return m_iPort; }

	void			Close();

public:
	virtual int		OnRecv();

private:
	void			Reset();

private:
	char			m_szAddr[256];
	uint16_t		m_iPort;

	PACKET*			m_pPacket;

	void*			m_pHandle;
	CBPush			m_cbPush;
};

//////////////////////////////////////////////////////////////////////////
class CClientManager
{
	typedef std::deque<CSocketClient*>	DEQ_CLIENT;
public:
	~CClientManager()	{}

	static CClientManager*	Instance();
	static void			Release();

	CSocketClient*		GetIdle();
	void				SetIdle(CSocketClient* pClient);

private:
	CClientManager()	{}
	CClientManager(const CClientManager&);
	CClientManager& operator=(const CClientManager&);

	void				Close();
private:
	static CClientManager*	m_pThis;

	CMutex				m_mtxClient;
	DEQ_CLIENT			m_deqClient;
};

#endif	// SOCKETCLIENT_H_

