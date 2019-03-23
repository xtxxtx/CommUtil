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

	class CListen : public IThread
	{
		typedef int(*CBAccept)(void* pHandler, int iFd, const char* pszAddr, unsigned short iPort);

	public:
		CListen();
		virtual ~CListen();

		int				Initialize(void* pHandler, CBAccept cbAccept, const char* pszAddr, uint16_t iPort);

		void			Close();

	protected:
		void			Execute();

	private:
		void*			m_pHandler;
		CBAccept		m_cbAccept;

		int				m_iFd;
	};

public:
	CSocketServer();
	~CSocketServer();

	int				Initialize(const char* pszAddr, uint16_t iPort, long lNum);

	void			Close();

	void			SetHandler(void* pHandler, CBAccept cbAccept);

	int				Create(int iFd, const char* pszAddr, uint16_t iPort);
	int				Create(const char* pszAddr, uint16_t iPort);

	CSocketClient*	GetClient();

protected:
	virtual void	Execute();

private:
	CSocketServer(const CSocketServer&);
	CSocketServer& operator=(const CSocketServer);

protected:
	void*			m_pHandler;
	CBAccept		m_cbAccept;
	CListen*		m_pListen;

	CMutex			m_mtxClient;
	DEQ_CLIENT		m_deqClient;
};

#endif  // SOCKETSERVER_H_

