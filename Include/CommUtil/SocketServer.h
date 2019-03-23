// SocketServer.h

#ifndef SOCKETSERVER_H_
#define SOCKETSERVER_H_

#include <deque>

#include <CommUtil/Mutex.h>
#include <CommUtil/Thread.h>


class CSocketClient;

typedef int(*CBConnect)(void* pHandler, CSocketClient*);

//////////////////////////////////////////////////////////////////////////
class CSocketServer : public IThread
{
	typedef std::deque<CSocketClient*>	DEQ_CLIENT;

	class CListen : public IThread
	{
	public:
		CListen(CSocketServer* pServer);
		virtual ~CListen();

		int				Initialize(const char* pszAddr, uint16_t iPort);

		void			Close();

	protected:
		void			Execute();

	private:
		CListen();
		CListen(const CListen&);

	private:
		CSocketServer*	m_pServer;

		int				m_iFd;
	};

public:
	CSocketServer();
	~CSocketServer();

	int				Initialize(const char* pszAddr, uint16_t iPort, long lNum, void* pHandler, CBConnect cbConnect);

	void			Close();

	int				OnAccept(int iFd, const char* pszAddr, uint16_t iPort);

	int				Create(const char* pszAddr, uint16_t iPort);
	
protected:
	virtual void	Execute();

private:
	CSocketServer(const CSocketServer&);
	CSocketServer& operator=(const CSocketServer);
	
protected:
	void*			m_pHandler;
	CBConnect		m_cbConnect;

	CListen*		m_pListen;

	CMutex			m_mtxClient;
	DEQ_CLIENT		m_deqClient;
};

#endif  // SOCKETSERVER_H_
