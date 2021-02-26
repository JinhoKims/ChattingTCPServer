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

void ChatServer::Init(const int nMaxSessionCount) // ���� �ʱ�ȭ
{
	for (int i = 0; i < nMaxSessionCount; ++i) // ���� ����ŭ ����Ʈ�� �Ҵ�
	{
		Session* pSession = new Session(i, (boost::asio::io_context&)m_acceptor.get_executor().context(), this); // i��° ���� �����Ҵ� 
		m_SessionList.push_back(pSession); // �Ҵ� �� ����Ʈ�� push
		m_SessionQueue.push_back(i); // ���� �ܿ���(ť) ����
	}
}

void ChatServer::Start()
{
	cout << "���� ����" << endl;
	PostAccept();
}

void ChatServer::CloseSession(const int nSessionID) // ���� ���� �Լ�
{
	cout << "Ŭ���̾�Ʈ ���� ����" << endl;
	
	m_SessionList[nSessionID]->Socket().close(); // ���� �ݱ�
	m_SessionQueue.push_back(nSessionID);

	if (m_bIsAccepting==false)
	{
		PostAccept(); // ���� ���� ��û ���� ������ ���� ��� ���� ó��
	}
}

void ChatServer::ProcessPacket(const int nSessionID, const char* pData) // ��Ŷ ó��
{
	PACKET_HEADER* pheader = (PACKET_HEADER*)pData;

	switch (pheader->nID) // �α��� ��
	{
	case REQ_IN:
	{
		PKT_REQ_IN* pPacket = (PKT_REQ_IN*)pData;
		m_SessionList[nSessionID]->SetName(pPacket->szName);

		cout << "Ŭ���̾�Ʈ �α��� ���� Name : " << m_SessionList[nSessionID]->GetName() << endl;

		PKT_RES_IN SendPkt;
		SendPkt.Init();
		SendPkt.bIsSuccess = true;

		m_SessionList[nSessionID]->PostSend(false, SendPkt.nSize, (char*)&SendPkt);
	}
	break;
	case REQ_CHAT: // ä�� ��
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

bool ChatServer::PostAccept() // Ŭ���̾�Ʈ ����ó�� �Լ�
{
	if (m_SessionQueue.empty()) // ť�� ����� ���
	{
		m_bIsAccepting = false;
		return false;
	}
	// ť�� �������� ���
	m_bIsAccepting = true;
	int nSessionID = m_SessionQueue.front(); // ť front �ε���
	m_SessionQueue.pop_front(); // ť ����
	m_acceptor.async_accept(m_SessionList[nSessionID]->Socket(), boost::bind(&ChatServer::handle_accept, this, m_SessionList[nSessionID], boost::asio::placeholders::error)); // front ���� ���� ����

	return true;
}

void ChatServer::handle_accept(Session* pSession, const boost::system::error_code& error) // Ŭ�� ���� �� �ڵ鸵 �Լ�
{
	if (!error)
	{
		cout << "Ŭ���̾�Ʈ ���� ����. SessionID: " << pSession->GetSessionID() << endl;

		pSession->Init(); // �� ���� �ʱ�ȭ
		pSession->PostReceive(); // ���� ������ ���� 
	}
	else
	{
		cout << "error No: " << error.value() << " error Message : " << error.message() << endl;
	}
}
