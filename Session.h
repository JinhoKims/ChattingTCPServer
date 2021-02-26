#pragma once
#include<sdkddkver.h>
#include<boost/asio.hpp>
#include<boost/bind.hpp>
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <string>
#include <vector>

#include "ChattingServer.h"
#include "Protocol.h"


class Session
{
private:
	boost::asio::ip::tcp::socket m_Socket;
	int m_nSessionID;
	std::array<char, MAX_RECEIVE_BUFFER_LEN> m_ReceiveBuffer;
	int m_nPacketBufferMark;
	char m_PacketBuffer[MAX_RECEIVE_BUFFER_LEN * 2];
	std::deque<char*> m_SendDataQueue;
	std::string m_Name;
	class ChatServer* m_pServer;

	void handle_write(const boost::system::error_code&, size_t);
	void handle_receive(const boost::system::error_code&, size_t);

public:
	boost::asio::ip::tcp::socket& Socket();

	Session(int, boost::asio::io_context& io_service, ChatServer* pServer);
	~Session();
	void Init();
	void PostReceive();
	void PostSend(const bool, const int, char*);
	void SetName(const char* pszName) { m_Name = pszName; }
	int GetSessionID() { return m_nSessionID; }
	const char* GetName() { return m_Name.c_str(); }


};