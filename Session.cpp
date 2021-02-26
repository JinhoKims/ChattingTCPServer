#include "Session.h"

boost::asio::ip::tcp::socket& Session::Socket()
{
	return m_Socket;
}

Session::Session(int nSessionID, boost::asio::io_context& io_service, ChatServer* pServer) : m_Socket(io_service), m_nSessionID(nSessionID), m_pServer(pServer)
{ 
} // 세션 생성자(세션번호, io_service, 서버)

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

void Session::PostReceive() // 데이터 수신
{
	m_Socket.async_read_some(boost::asio::buffer(m_ReceiveBuffer), boost::bind(&Session::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}	// m_ReceiveBuffer에 데이터 수신 후 handle_receive 함수 핸들링

void Session::PostSend(const bool bImmediately, const int nSize ,char* pData) // 데이터 전송 함수
{
	char* pSendData = nullptr;
	
	if (bImmediately == false)
	{
		pSendData = new char[nSize];
		memcpy(pSendData, pData, nSize); // 데이터 복사

		m_SendDataQueue.push_back(pSendData); // 보낼 데이터를 큐에 저장
	}
	else
	{
		pSendData = pData;
	}
	
	/* handle_write가 호출될 때까진 데이터를 다 보낸 게 아니기 때문에 삭제하지 않고 보관한다.
	   데이터가 모두 다 보내진 걸 확인한 후에야 데이터를 삭제해야 한다. */
	if (bImmediately == false && m_SendDataQueue.size() > 1)
	{
		return; // 보내야 할 데이터가 아직 다 보내지지 않으면 삭제하지 않고 다음에 보낸다.
	}

	// 데이터 전송이 완료되면 바인딩할 함수 (handle_write)
	boost::asio::async_write(m_Socket, boost::asio::buffer(pSendData, nSize), boost::bind(&Session::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void Session::handle_write(const boost::system::error_code&, size_t) // 데이터 모두 전송 시
{
	delete[] m_SendDataQueue.front(); // 큐에 저장한 데이터는 삭제 (front 할당 부분을 해제)
	m_SendDataQueue.pop_front(); // 할당 해제한 front를 pop

	// 큐에 저장한 데이터가 있다면 데이터를 보낸다.
	if (m_SendDataQueue.empty() == false)
	{
		char* pData = m_SendDataQueue.front();
		PACKET_HEADER* pHeader = (PACKET_HEADER*)pData;
		PostSend(true, pHeader->nSize, pData);
	}
}

void Session::handle_receive(const boost::system::error_code& error, size_t bytes_transferred) // 데이터 수신 후 처리
{
	if (error) // 수신 에러시
	{
		if (error == boost::asio::error::eof)
		{
			std::cout << "클라이언트와 연결이 끊어졌습니다" << std::endl;
		}
		else
		{
			std::cout << "error No: " << error.value() << " error Message: " << error.message() << std::endl;
		}

		m_pServer->CloseSession(m_nSessionID); // 세션 닫기
	}

	else // 성공 시
	{
		memcpy(&m_PacketBuffer[m_nPacketBufferMark], m_ReceiveBuffer.data(), bytes_transferred); // 받은 데이터를 패킷 버퍼에 저장

		int nPacketData = m_nPacketBufferMark + bytes_transferred; // 한 클라이언트가 여러번 Write 한 걸 대비하여 각 요청별로 나누어서 처리 
		int nReadData = 0; // 패킷 인덱스

		while (nPacketData > 0) // 받은 데이터를 모두 처리할 때까지 반복
		{
			if (nPacketData < sizeof(PACKET_HEADER))
			{
				break; // 남은 데이터가 패킷 헤더보다 작으면 중단
			}

			PACKET_HEADER* pHeader = (PACKET_HEADER*)&m_PacketBuffer[nReadData]; // 해당 패킷 선택

			if (pHeader->nSize <= nPacketData) // 처리할 수 있는 만큼 데이터가 있다면 패킷을 처리
			{
				m_pServer->ProcessPacket(m_nSessionID, &m_PacketBuffer[nReadData]); // 패킷 처리

				nPacketData -= pHeader->nSize; // 패킷 크기 감소
				nReadData += pHeader->nSize; // 다음 패킷만큼 점프
			}
			else
			{
				break; // 패킷으로 처리할 수 있는 만큼이 아니면 중단
			}
		}

		if (nPacketData > 0) // 남은 데이터는 m_PacketBuffer에 저장
		{
			char TempBuffer[MAX_RECEIVE_BUFFER_LEN] = { 0, };
			memcpy(&TempBuffer[0], &m_PacketBuffer[nReadData], nPacketData);
			memcpy(&m_PacketBuffer[0], &TempBuffer[0], nPacketData);
		}

		// 남은 데이터 양을 저장하고 데이터 받기 요청
		m_nPacketBufferMark = nPacketData;
		PostReceive();
	}
}

