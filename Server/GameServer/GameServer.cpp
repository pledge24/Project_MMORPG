#include "pch.h"
#include <thread>
#include "Service.h"
#include "IocpCore.h"
#include "GameSession.h"
#include "ServerPacketHandler.h"

void WorkerThreadMain(ServerServiceRef service)
{
	while (true)
	{
		service->GetIocpCore()->Dispatch();
	}
}

int main(void)
{
	ServerPacketHandler::Init();

	ServerServiceRef service = make_shared<ServerService>(
		NetAddress("127.0.0.1"s, 7777),
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); }, // TODO : SessionManager 등
		30
	);

	ASSERT_CRASH(service->Start());

	// worker Thread
	const int threadN = 5;
	for (int i = 0; i < threadN; i++)
	{
		GThreadManager->Launch([&service]()
			{
				WorkerThreadMain(service);
			});
	}

	while (true)
	{
		this_thread::sleep_for(1s);
	}

	GThreadManager->Join();

	return 0;
}