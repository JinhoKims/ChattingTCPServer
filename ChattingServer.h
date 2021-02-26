#pragma once
#include<sdkddkver.h>
#include<boost/asio.hpp>
#include<boost/bind.hpp>
#include<iostream>
#include<vector>
#include<deque>

#include "Session.h"
using namespace std;

class ChatServer
{
public:
	boost::asio::ip::tcp::acceptor m_acceptor;
	vector<class Session*> m_SessionList; // ������ ��� ����Ʈ
	deque<int> m_SessionQueue; // ������� �ʴ� ���� ť
	int m_SeqNumber;
	bool m_bIsAccepting;


	ChatServer(boost::asio::io_context&);
	~ChatServer();
	void Init(const int);
	void Start();
	void CloseSession(const int);
	void ProcessPacket(const int, const char*);
	
private:
	bool PostAccept();
	void handle_accept(Session*, const boost::system::error_code&);

};