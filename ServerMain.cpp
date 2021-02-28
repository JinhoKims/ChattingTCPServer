#include <sdkddkver.h>
#include "ChattingServer.h"

const int MAX_SESSION_COUNT = 100;

int main()
{
	boost::asio::io_context io_service;

	ChatServer server(io_service);
	server.Init(MAX_SESSION_COUNT); // ���� �ʱ�ȭ
	server.Start(); // ���� ����

	io_service.run(); // main ���� ����
	/*
		������ ���α׷���ó�� run() �Լ��� ȣ���ϸ� main() �Լ��� ���� ��� ���°� �ǰ�
		Boost.Asio�� �񵿱� �Լ� �۾��� ��� ���� ������ ������ ����ϴٰ�
		�۾��� ��� ������ run() �Լ��� �������ͼ� ���� �۾��� �����Ѵ�.
	*/

	std::cout << "���� ����" << endl;

	return 0;
}