#include "Core/pch.h"
#include <gtest/gtest.h>
#include "ServerCore/Network/Service.h"
#include "ServerCore/Network/Session.h"
#include "ServerCore/Network/IocpCore.h"
#include "ServerCore/Job/JobTimer.h"
#include "ServerCore/Job/JobQueue.h"
#include "ServerCore/Job/GlobalQueue.h"

/*--------------------------------------------------------------
    서버가 건 끊기 테스트

    서버가 건 끊기(DisconnectEx)는 상대가 소켓을 닫아야 완료된다. 받기만 하고 닫지 않는 상대면 OnDisconnected가
    불리지 않아, 밀어낸 세션의 저장과 저장 대기 해제가 일어나지 않았다(TD-046).

    픽스처 결합도: 루프백(127.0.0.1)의 고정 포트로 실제 ServerService를 띄우고 Winsock 소켓으로 접속한다.
    워커 스레드는 없다. 테스트가 IOCP 디스패치, 예약 잡 분배, 글로벌 큐 소비를 직접 돌린다.
    전역 JobTimer와 GlobalQueue는 TestMain의 ServerContext가 만든 것을 쓴다.
---------------------------------------------------------------*/

namespace
{
    constexpr uint16 TEST_PORT = 47913;

    class RecordingSession : public Session
    {
    public:
        atomic<bool> disconnected = false;

    protected:
        void OnDisconnected() override { disconnected.store(true); }
    };
}

class SessionDisconnectTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        core = make_shared<IocpCore>();
        service = make_shared<ServerService>(
            NetAddress("127.0.0.1"s, TEST_PORT),
            core,
            [this]()
            {
                auto session = make_shared<RecordingSession>();
                sessions.push_back(session);
                return session;
            },
            1);
        ASSERT_TRUE(service->Start());
    }

    void TearDown() override
    {
        if (peer != INVALID_SOCKET)
            ::closesocket(peer);

        // 걸어 둔 AcceptEx와 수신이 실패로 완료되어 owner 참조를 놓도록 몇 번 더 돌린다.
        service->CloseService();
        PumpUntil([]() { return false; }, 100);
        sessions.clear();
        service.reset();
        core.reset();
    }

    /** 워커 한 바퀴를 테스트 스레드에서 돈다. 조건이 맞거나 시간이 다 되면 멈춘다. */
    bool PumpUntil(const function<bool()>& done, uint64 timeoutMs)
    {
        const uint64 deadline = ::GetTickCount64() + timeoutMs;
        while (::GetTickCount64() < deadline)
        {
            core->Dispatch(10);
            GJobTimer->Distribute(::GetTickCount64());
            while (JobQueueRef queue = GGlobalQueue->Pop())
                queue->Execute();

            if (done())
                return true;
        }
        return done();
    }

    /** 접속하고 받지도 닫지도 않는다. 멈춘 클라이언트나 조작한 클라이언트를 흉내 낸다. */
    shared_ptr<RecordingSession> ConnectSilentPeer()
    {
        peer = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        SOCKADDR_IN address = NetAddress("127.0.0.1"s, TEST_PORT).GetSockAddr();
        if (::connect(peer, reinterpret_cast<SOCKADDR*>(&address), sizeof(address)) == SOCKET_ERROR)
            return nullptr;

        if (PumpUntil([this]() { return service->GetCurrentSessionCount() == 1; }, 2000) == false)
            return nullptr;

        for (const shared_ptr<RecordingSession>& session : sessions)
        {
            if (session->IsConnected())
                return session;
        }
        return nullptr;
    }

    IocpCoreRef core;
    ServerServiceRef service;
    vector<shared_ptr<RecordingSession>> sessions;
    SOCKET peer = INVALID_SOCKET;
};

TEST_F(SessionDisconnectTest, DisconnectFinishesEvenIfPeerNeverCloses)
{
    shared_ptr<RecordingSession> session = ConnectSilentPeer();
    ASSERT_NE(session, nullptr) << "준비: 루프백 접속을 받지 못했다";

    session->Disconnect("test");

    EXPECT_TRUE(PumpUntil([&session]() { return session->disconnected.load(); }, 3000))
        << "상대가 소켓을 닫지 않아도 서버가 건 끊기는 상한 안에 OnDisconnected까지 가야 한다";
    EXPECT_EQ(service->GetCurrentSessionCount(), 0);
}
