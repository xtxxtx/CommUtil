// SocketServer.h

#ifndef SOCKETSERVER_H_
#define SOCKETSERVER_H_

#include <deque>

#include <CommUtil/Mutex.h>
#include <CommUtil/Thread.h>


class CSocketListen;
class CSocketClient;


//////////////////////////////////////////////////////////////////////////
class CSocketServer : public IThread
{
	typedef int (*CBAccept)(void*, CSocketClient*);
	typedef std::deque<CSocketClient*>	DEQ_CLIENT;

public:
	CSocketServer();
	~CSocketServer();

	int				Initialize(const char* pszAddr, uint16_t iPort, long lNum);

	void			Close();

	void			SetHandle(void* pHandle, CBAccept cbAccept);

	int				Create(int iFd, const char* pszAddr, uint16_t iPort);
	int				Create(const char* pszAddr, uint16_t iPort);

	CSocketClient*	GetClient();

protected:
	virtual void	Execute();

private:
	CSocketServer(const CSocketServer&);
	CSocketServer& operator=(const CSocketServer);

protected:
	void*			m_pHandle;
	CBAccept		m_cbAccept;
	CSocketListen*	m_pListen;

	CMutex			m_mtxClient;
	DEQ_CLIENT		m_deqClient;
};

#endif  // SOCKETSERVER_H_

