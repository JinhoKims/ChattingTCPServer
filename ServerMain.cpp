#include <sdkddkver.h>
#include "ChattingServer.h"

const int MAX_SESSION_COUNT = 100;

int main()
{
	boost::asio::io_context io_service;

	ChatServer server(io_service);
	server.Init(MAX_SESSION_COUNT); // 서버 초기화
	server.Start(); // 서버 실행

	io_service.run(); // main 종료 방지
	/*
		스레드 프로그래밍처럼 run() 함수를 호출하면 main() 함수가 무한 대기 상태가 되고
		Boost.Asio의 비동기 함수 작업이 모두 끝날 때까지 무한히 대기하다가
		작업이 모두 끝나면 run() 함수를 빠져나와서 다음 작업을 진행한다.
	*/

	std::cout << "서버 시작" << endl;

	return 0;
}