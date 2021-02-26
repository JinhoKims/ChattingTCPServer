#include "ChattingServer.h"
#include "Protocol.h"

ChatServer::ChatServer(boost::asio::io_context& io_service) :m_acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), PORT_NUMBER))
{
	m_bIsAccepting = false;
}

ChatServer::~ChatServer()
{
	for (size_t i = 0; i < m_SessionList.size(); ++i)
	{
		if (m_SessionList[i]->Socket().is_open())
		{
			m_SessionList[i]->Socket().close();
		}
		delete m_SessionList[i];
	}
}

void ChatServer::Init(const int nMaxSessionCount) // 세션 초기화
{
	for (int i = 0; i < nMaxSessionCount; ++i) // 세션 수만큼 리스트에 할당
	{
		Session* pSession = new Session(i, (boost::asio::io_context&)m_acceptor.get_executor().context(), this); // i번째 세션 동적할당 
		m_SessionList.push_back(pSession); // 할당 후 리스트에 push
		m_SessionQueue.push_back(i); // 세션 잔여량(큐) 증가
	}
}

void ChatServer::Start()
{
	cout << "서버 시작" << endl;
	PostAccept();
}

void ChatServer::CloseSession(const int nSessionID) // 세션 종료 함수
{
	cout << "클라이언트 접속 종료" << endl;
	
	m_SessionList[nSessionID]->Socket().close(); // 소켓 닫기
	m_SessionQueue.push_back(nSessionID);

	if (m_bIsAccepting==false)
	{
		PostAccept(); // 아직 접속 요청 중인 세션이 있을 경우 접속 처리
	}
}

void ChatServer::ProcessPacket(const int nSessionID, const char* pData) // 패킷 처리
{
	PACKET_HEADER* pheader = (PACKET_HEADER*)pData;

	switch (pheader->nID) // 로그인 시
	{
	case REQ_IN:
	{
		PKT_REQ_IN* pPacket = (PKT_REQ_IN*)pData;
		m_SessionList[nSessionID]->SetName(pPacket->szName);

		cout << "클라이언트 로그인 성공 Name : " << m_SessionList[nSessionID]->GetName() << endl;

		PKT_RES_IN SendPkt;
		SendPkt.Init();
		SendPkt.bIsSuccess = true;

		m_SessionList[nSessionID]->PostSend(false, SendPkt.nSize, (char*)&SendPkt);
	}
	break;
	case REQ_CHAT: // 채팅 시
	{
		PKT_REQ_CHAT* pPacket = (PKT_REQ_CHAT*)pData;

		PKT_NOTICE_CHAT SendPkt;
		SendPkt.Init();
		strncpy_s(SendPkt.szName, MAX_NAME_LEN, m_SessionList[nSessionID]->GetName(), MAX_NAME_LEN - 1);
		strncpy_s(SendPkt.szMessage, MAX_MESSAGE_LEN, pPacket->szMessage, MAX_MESSAGE_LEN - 1);

		size_t nTotalSessionCount = m_SessionList.size();

		for (size_t i = 0; i < nTotalSessionCount; ++i)
		{
			if (m_SessionList[i]->Socket().is_open())
			{
				m_SessionList[i]->PostSend(false, SendPkt.nSize, (char*)&SendPkt);
			}
		}
	}
	break;
	default:
		break;
	}
}

bool ChatServer::PostAccept() // 클라이언트 접속처리 함수
{
	if (m_SessionQueue.empty()) // 큐가 비었을 경우
	{
		m_bIsAccepting = false;
		return false;
	}
	// 큐가 남아있을 경우
	m_bIsAccepting = true;
	int nSessionID = m_SessionQueue.front(); // 큐 front 인덱싱
	m_SessionQueue.pop_front(); // 큐 감소
	m_acceptor.async_accept(m_SessionList[nSessionID]->Socket(), boost::bind(&ChatServer::handle_accept, this, m_SessionList[nSessionID], boost::asio::placeholders::error)); // front 부터 접속 시작

	return true;
}

void ChatServer::handle_accept(Session* pSession, const boost::system::error_code& error) // 클라 접속 후 핸들링 함수
{
	if (!error)
	{
		cout << "클라이언트 접속 성공. SessionID: " << pSession->GetSessionID() << endl;

		pSession->Init(); // 각 세션 초기화
		pSession->PostReceive(); // 세션 데이터 수신 
	}
	else
	{
		cout << "error No: " << error.value() << " error Message : " << error.message() << endl;
	}
}
