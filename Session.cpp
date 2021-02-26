#include "Session.h"

boost::asio::ip::tcp::socket& Session::Socket()
{
	return m_Socket;
}

Session::Session(int nSessionID, boost::asio::io_context& io_service, ChatServer* pServer) : m_Socket(io_service), m_nSessionID(nSessionID), m_pServer(pServer)
{ 
} // ���� ������(���ǹ�ȣ, io_service, ����)

Session::~Session()
{
	while (m_SendDataQueue.empty() == false)
	{
		delete[] m_SendDataQueue.front();
		m_SendDataQueue.pop_front();
	}
}

void Session::Init()
{
	m_nPacketBufferMark = 0;
}

void Session::PostReceive() // ������ ����
{
	m_Socket.async_read_some(boost::asio::buffer(m_ReceiveBuffer), boost::bind(&Session::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}	// m_ReceiveBuffer�� ������ ���� �� handle_receive �Լ� �ڵ鸵

void Session::PostSend(const bool bImmediately, const int nSize ,char* pData) // ������ ���� �Լ�
{
	char* pSendData = nullptr;
	
	if (bImmediately == false)
	{
		pSendData = new char[nSize];
		memcpy(pSendData, pData, nSize); // ������ ����

		m_SendDataQueue.push_back(pSendData); // ���� �����͸� ť�� ����
	}
	else
	{
		pSendData = pData;
	}
	
	/* handle_write�� ȣ��� ������ �����͸� �� ���� �� �ƴϱ� ������ �������� �ʰ� �����Ѵ�.
	   �����Ͱ� ��� �� ������ �� Ȯ���� �Ŀ��� �����͸� �����ؾ� �Ѵ�. */
	if (bImmediately == false && m_SendDataQueue.size() > 1)
	{
		return; // ������ �� �����Ͱ� ���� �� �������� ������ �������� �ʰ� ������ ������.
	}

	// ������ ������ �Ϸ�Ǹ� ���ε��� �Լ� (handle_write)
	boost::asio::async_write(m_Socket, boost::asio::buffer(pSendData, nSize), boost::bind(&Session::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void Session::handle_write(const boost::system::error_code&, size_t) // ������ ��� ���� ��
{
	delete[] m_SendDataQueue.front(); // ť�� ������ �����ʹ� ���� (front �Ҵ� �κ��� ����)
	m_SendDataQueue.pop_front(); // �Ҵ� ������ front�� pop

	// ť�� ������ �����Ͱ� �ִٸ� �����͸� ������.
	if (m_SendDataQueue.empty() == false)
	{
		char* pData = m_SendDataQueue.front();
		PACKET_HEADER* pHeader = (PACKET_HEADER*)pData;
		PostSend(true, pHeader->nSize, pData);
	}
}

void Session::handle_receive(const boost::system::error_code& error, size_t bytes_transferred) // ������ ���� �� ó��
{
	if (error) // ���� ������
	{
		if (error == boost::asio::error::eof)
		{
			std::cout << "Ŭ���̾�Ʈ�� ������ ���������ϴ�" << std::endl;
		}
		else
		{
			std::cout << "error No: " << error.value() << " error Message: " << error.message() << std::endl;
		}

		m_pServer->CloseSession(m_nSessionID); // ���� �ݱ�
	}

	else // ���� ��
	{
		memcpy(&m_PacketBuffer[m_nPacketBufferMark], m_ReceiveBuffer.data(), bytes_transferred); // ���� �����͸� ��Ŷ ���ۿ� ����

		int nPacketData = m_nPacketBufferMark + bytes_transferred; // �� Ŭ���̾�Ʈ�� ������ Write �� �� ����Ͽ� �� ��û���� ����� ó�� 
		int nReadData = 0; // ��Ŷ �ε���

		while (nPacketData > 0) // ���� �����͸� ��� ó���� ������ �ݺ�
		{
			if (nPacketData < sizeof(PACKET_HEADER))
			{
				break; // ���� �����Ͱ� ��Ŷ ������� ������ �ߴ�
			}

			PACKET_HEADER* pHeader = (PACKET_HEADER*)&m_PacketBuffer[nReadData]; // �ش� ��Ŷ ����

			if (pHeader->nSize <= nPacketData) // ó���� �� �ִ� ��ŭ �����Ͱ� �ִٸ� ��Ŷ�� ó��
			{
				m_pServer->ProcessPacket(m_nSessionID, &m_PacketBuffer[nReadData]); // ��Ŷ ó��

				nPacketData -= pHeader->nSize; // ��Ŷ ũ�� ����
				nReadData += pHeader->nSize; // ���� ��Ŷ��ŭ ����
			}
			else
			{
				break; // ��Ŷ���� ó���� �� �ִ� ��ŭ�� �ƴϸ� �ߴ�
			}
		}

		if (nPacketData > 0) // ���� �����ʹ� m_PacketBuffer�� ����
		{
			char TempBuffer[MAX_RECEIVE_BUFFER_LEN] = { 0, };
			memcpy(&TempBuffer[0], &m_PacketBuffer[nReadData], nPacketData);
			memcpy(&m_PacketBuffer[0], &TempBuffer[0], nPacketData);
		}

		// ���� ������ ���� �����ϰ� ������ �ޱ� ��û
		m_nPacketBufferMark = nPacketData;
		PostReceive();
	}
}

