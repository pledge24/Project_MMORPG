#include "Core/pch.h"
#include <gtest/gtest.h>
#include <sstream>
#include "RecordingSession.h"
#include "Network/GameSessionManager.h"
#include "Lobby/Data/LobbyData.h"

/*--------------------------------------------------------------
    패킷 핸들러와 세션 단계 검사 테스트

    세션은 패킷을 핸들러에 넘기기 전에 단계별 허용표로 단계를 확인하고, 맞지 않는 패킷은 경고 로그를 남기고 버린다.
    단계를 보지 않던 때는 게임에 입장한 세션이 플레이 중인 캐릭터를 지울 수 있었고, 입장한 세션이 다른 계정의
    인증 티켓으로 로비 접속을 다시 보내면 세션의 계정 번호만 바뀌어 접속 종료 저장의 대기가 풀리지 않았다.
    로비 접속 핸들러는 받는 순간 세션을 로비 접속 중 단계로 바꾼다. 잡이 끝난 뒤에 바꾸던 때는 그 전에 온
    두 번째 로비 접속도 통과했다.

    픽스처 결합도: 전역 DB 관리자(GDBManager)에 큐 하나를 만들고 끝나면 비운다. 핸들러가 큐에 잡을 넣었는지는
    큐를 멈춘 뒤 WaitForSingleJob으로 본다. 멈춘 큐는 남은 잡이 없으면 블로킹하지 않고 nullptr를 돌려준다.
    세션은 보낸 패킷을 기록하는 RecordingSession이고, 받은 패킷은 IOCP와 같은 경로(OnRecvPacket)로 넘긴다.
    테스트 실행 파일에는 DB 연결이 없으므로 DB 잡은 연결을 빌리지 못하고 실패 경로로 끝난다.
    1레벨 전사 레벨 표를 Gamedata::Install로 주입한다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 USER_ID = 100;

    template<typename PacketType>
    vector<BYTE> Serialize(const PacketType& pkt, uint16 packetId)
    {
        const int32 bodySize = static_cast<int32>(pkt.ByteSizeLong());
        vector<BYTE> buffer(sizeof(PacketHeader) + bodySize);
        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer.data());
        header->size = static_cast<uint16>(buffer.size());
        header->id = packetId;
        pkt.SerializeToArray(buffer.data() + sizeof(PacketHeader), bodySize);
        return buffer;
    }

    Protocol::C_CREATE_CHARACTER MakeCreateWarrior()
    {
        Protocol::C_CREATE_CHARACTER pkt;
        pkt.set_character_class(Protocol::CLASS_TYPE_WARRIOR);
        pkt.set_name("전사");
        return pkt;
    }

    Protocol::C_DELETE_CHARACTER MakeDelete()
    {
        Protocol::C_DELETE_CHARACTER pkt;
        pkt.set_character_id(42);
        return pkt;
    }

    Protocol::C_ENTER_GAME MakeEnterGame()
    {
        Protocol::C_ENTER_GAME pkt;
        pkt.set_character_id(42);
        return pkt;
    }

    Protocol::C_CONNECT_LOBBY MakeConnectLobby()
    {
        Protocol::C_CONNECT_LOBBY pkt;
        pkt.set_auth_ticket("ticket");
        return pkt;
    }
}

class PacketHandlerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        LevelTemplate level1;
        level1.level = 1;
        GamedataTables tables;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1 });
        Gamedata::Install(std::move(tables));

        ServerPacketHandler::Init();
        GDBManager->Init(1);
        recordingSession = make_shared<RecordingSession>();
        session = recordingSession;
    }

    void TearDown() override
    {
        GDBManager->Clear();
        Gamedata::Install(GamedataTables());
        LobbyData::Install(LobbyTables());
    }

    /** 로비 접속을 마친 세션으로 만든다. 계정 등록은 로비 접속 잡이 하는 것과 같이 세션 관리자로 한다. */
    void ConnectLobby()
    {
        sessionManager.RegisterUser(USER_ID, recordingSession);
        ASSERT_TRUE(recordingSession->BeginLobbyConnect());
        ASSERT_TRUE(recordingSession->CompleteLobbyConnect());
    }

    /** 게임에 입장한 세션으로 만든다. */
    void EnterGame()
    {
        ConnectLobby();
        ASSERT_TRUE(recordingSession->BeginGameEntry());
        ASSERT_TRUE(recordingSession->CompleteGameEntry());
    }

    /** 패킷을 받고, 그동안 남긴 로그를 돌려준다. */
    template<typename PacketType>
    string Receive(const PacketType& pkt, uint16 packetId)
    {
        vector<BYTE> buffer = Serialize(pkt, packetId);

        // GLogger는 cout에 쓴다. 출력을 잠시 붙잡는다.
        stringstream captured;
        streambuf* original = cout.rdbuf(captured.rdbuf());
        recordingSession->Receive(buffer.data(), static_cast<int32>(buffer.size()));
        cout.rdbuf(original);
        return captured.str();
    }

    /** 핸들러가 DB 큐에 넣은 잡. 없으면 nullptr. 큐를 멈추므로 테스트마다 한 번만 부른다. */
    JobRef TakePushedDBJob()
    {
        DBQueueRef dbQueue = GDBManager->GetDBQueue(0);
        dbQueue->Stop();
        return dbQueue->WaitForSingleJob();
    }

    /** 단계에 맞지 않아 버렸다는 경고가 log에 있고, DB 잡도 응답도 없으면 통과. */
    void ExpectDropped(const string& log, uint16 packetId)
    {
        EXPECT_NE(log.find(format("받지 않는 패킷 {}", packetId)), string::npos) << "버린 패킷은 경고 로그를 남긴다. 실제 출력: " << log;
        EXPECT_EQ(TakePushedDBJob(), nullptr) << "버린 패킷은 DB 큐에 가지 않는다";
        EXPECT_TRUE(recordingSession->SentPackets<Protocol::S_CREATE_CHARACTER>(PKT_S_CREATE_CHARACTER).empty());
        EXPECT_TRUE(recordingSession->SentPackets<Protocol::S_DELETE_CHARACTER>(PKT_S_DELETE_CHARACTER).empty());
        EXPECT_TRUE(recordingSession->SentPackets<Protocol::S_ENTER_GAME>(PKT_S_ENTER_GAME).empty());
    }

    GameSessionManager sessionManager;
    shared_ptr<RecordingSession> recordingSession;
    PacketSessionRef session;
};

// 받은 패킷의 처리가 실패해도(핸들러가 거절하거나 본문을 풀지 못해도) 로그가 없어서 원인을 찾을 수 없었다.
TEST_F(PacketHandlerTest, FailedPacketIsLogged)
{
    EnterGame();

    // 플레이어가 없는 세션의 채팅은 핸들러가 거절한다.
    Protocol::C_CHAT chatPkt;
    chatPkt.set_msg("hello");
    const string log = Receive(chatPkt, PKT_C_CHAT);

    EXPECT_NE(log.find(format("패킷 {} 처리에 실패했다", static_cast<int32>(PKT_C_CHAT))), string::npos)
        << "처리에 실패한 패킷의 id가 로그에 남아야 한다. 실제 출력: " << log;
}

// 삭제 잡이 실행될 때 세션에서 계정 번호를 다시 읽었다. 그사이 세션의 계정이 바뀌면 다른 계정으로 일한다.
// 잡은 연결을 빌리지 못하고, 실패 로그에 쓴 계정 번호를 남긴다.
TEST_F(PacketHandlerTest, DeleteJobUsesAccountReadByHandler)
{
    constexpr int64 SECOND_USER_ID = 200;
    ConnectLobby();

    Receive(MakeDelete(), PKT_C_DELETE_CHARACTER);

    // 잡이 돌기 전에 세션의 계정이 바뀐다.
    sessionManager.RegisterUser(SECOND_USER_ID, recordingSession);

    JobRef job = TakePushedDBJob();
    ASSERT_NE(job, nullptr);

    stringstream captured;
    streambuf* original = cout.rdbuf(captured.rdbuf());
    job->Execute();
    cout.rdbuf(original);

    EXPECT_NE(captured.str().find(format("계정 {}", USER_ID)), string::npos)
        << "잡은 핸들러가 읽은 계정 번호로 일해야 한다. 실제 출력: " << captured.str();
}

// DB 작업이 실패해도 응답한다. 결과 값은 요청을 바꿔도 풀리지 않는 실패다.
TEST_F(PacketHandlerTest, DeleteWithoutDBRespondsInternalError)
{
    ConnectLobby();
    Receive(MakeDelete(), PKT_C_DELETE_CHARACTER);

    JobRef job = TakePushedDBJob();
    ASSERT_NE(job, nullptr);

    stringstream captured;
    streambuf* original = cout.rdbuf(captured.rdbuf());
    job->Execute();
    cout.rdbuf(original);

    const vector<Protocol::S_DELETE_CHARACTER> responses = recordingSession->SentPackets<Protocol::S_DELETE_CHARACTER>(PKT_S_DELETE_CHARACTER);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::LOBBY_RESULT_INTERNAL_ERROR);
    EXPECT_EQ(responses[0].character_id(), 42) << "실패해도 요청한 캐릭터 번호를 싣는다";
}

TEST_F(PacketHandlerTest, CreateCharacterBeforeLobbyConnectIsDropped)
{
    ExpectDropped(Receive(MakeCreateWarrior(), PKT_C_CREATE_CHARACTER), PKT_C_CREATE_CHARACTER);
}

TEST_F(PacketHandlerTest, DeleteCharacterBeforeLobbyConnectIsDropped)
{
    ExpectDropped(Receive(MakeDelete(), PKT_C_DELETE_CHARACTER), PKT_C_DELETE_CHARACTER);
}

TEST_F(PacketHandlerTest, EnterGameBeforeLobbyConnectIsDropped)
{
    ExpectDropped(Receive(MakeEnterGame(), PKT_C_ENTER_GAME), PKT_C_ENTER_GAME);
    EXPECT_EQ(recordingSession->GetPhase(), SessionPhase::BEFORE_LOBBY);
}

TEST_F(PacketHandlerTest, SecondLobbyConnectIsDropped)
{
    ConnectLobby();

    ExpectDropped(Receive(MakeConnectLobby(), PKT_C_CONNECT_LOBBY), PKT_C_CONNECT_LOBBY);
    EXPECT_EQ(recordingSession->GetUserId(), USER_ID);
}

// 로비 접속 잡이 끝나야 단계가 바뀌던 때는, 잡이 끝나기 전에 같은 세션이 보낸 두 번째 로비 접속도 단계 검사를 통과했다.
TEST_F(PacketHandlerTest, SecondLobbyConnectWhileConnectingIsDropped)
{
    Receive(MakeConnectLobby(), PKT_C_CONNECT_LOBBY);
    EXPECT_EQ(recordingSession->GetPhase(), SessionPhase::CONNECTING_LOBBY) << "핸들러가 받는 순간 단계를 바꾼다";

    const string log = Receive(MakeConnectLobby(), PKT_C_CONNECT_LOBBY);
    EXPECT_NE(log.find(format("받지 않는 패킷 {}", static_cast<int32>(PKT_C_CONNECT_LOBBY))), string::npos) << log;

    DBQueueRef dbQueue = GDBManager->GetDBQueue(0);
    dbQueue->Stop();
    EXPECT_NE(dbQueue->WaitForSingleJob(), nullptr) << "첫 로비 접속의 잡";
    EXPECT_EQ(dbQueue->WaitForSingleJob(), nullptr) << "버린 로비 접속은 잡을 넣지 않는다";
}

TEST_F(PacketHandlerTest, LobbyRequestsWhileConnectingAreDropped)
{
    Receive(MakeConnectLobby(), PKT_C_CONNECT_LOBBY);

    string log = Receive(MakeCreateWarrior(), PKT_C_CREATE_CHARACTER);
    log += Receive(MakeDelete(), PKT_C_DELETE_CHARACTER);
    log += Receive(MakeEnterGame(), PKT_C_ENTER_GAME);

    EXPECT_NE(log.find(format("받지 않는 패킷 {}", static_cast<int32>(PKT_C_CREATE_CHARACTER))), string::npos) << log;
    EXPECT_NE(log.find(format("받지 않는 패킷 {}", static_cast<int32>(PKT_C_DELETE_CHARACTER))), string::npos) << log;
    EXPECT_NE(log.find(format("받지 않는 패킷 {}", static_cast<int32>(PKT_C_ENTER_GAME))), string::npos) << log;
    EXPECT_EQ(recordingSession->GetPhase(), SessionPhase::CONNECTING_LOBBY);
}

// 입장한 세션이 다른 계정의 인증 티켓으로 로비 접속을 보내면 세션의 계정 번호만 바뀌었다.
TEST_F(PacketHandlerTest, LobbyConnectInGameIsDropped)
{
    EnterGame();

    ExpectDropped(Receive(MakeConnectLobby(), PKT_C_CONNECT_LOBBY), PKT_C_CONNECT_LOBBY);
    EXPECT_EQ(recordingSession->GetUserId(), USER_ID);
}

// 게임에 입장한 세션이 플레이 중인 캐릭터를 지울 수 있었다.
TEST_F(PacketHandlerTest, LobbyRequestsInGameAreDropped)
{
    EnterGame();

    string log = Receive(MakeDelete(), PKT_C_DELETE_CHARACTER);
    log += Receive(MakeCreateWarrior(), PKT_C_CREATE_CHARACTER);
    log += Receive(MakeEnterGame(), PKT_C_ENTER_GAME);

    EXPECT_NE(log.find(format("받지 않는 패킷 {}", static_cast<int32>(PKT_C_DELETE_CHARACTER))), string::npos) << log;
    EXPECT_NE(log.find(format("받지 않는 패킷 {}", static_cast<int32>(PKT_C_CREATE_CHARACTER))), string::npos) << log;
    ExpectDropped(log, PKT_C_ENTER_GAME);
    EXPECT_EQ(recordingSession->GetPhase(), SessionPhase::IN_GAME);
}

// 생성할 수 없는 직업은 DB 큐로 넘기지 않고 결과 값으로 곧바로 거절한다. 캐릭터 ID도 발급하지 않는다.
TEST_F(PacketHandlerTest, UncreatableClassIsRejectedWithoutDBJob)
{
    LobbyTables lobbyTables;
    lobbyTables.creatableClasses = { Protocol::CLASS_TYPE_WARRIOR };
    LobbyData::Install(std::move(lobbyTables));
    ConnectLobby();

    Protocol::C_CREATE_CHARACTER createPkt;
    createPkt.set_name("마법사");
    createPkt.set_character_class(Protocol::CLASS_TYPE_MAGE);
    Receive(createPkt, PKT_C_CREATE_CHARACTER);

    EXPECT_EQ(TakePushedDBJob(), nullptr);
    const vector<Protocol::S_CREATE_CHARACTER> responses = recordingSession->SentPackets<Protocol::S_CREATE_CHARACTER>(PKT_S_CREATE_CHARACTER);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::LOBBY_RESULT_UNAVAILABLE_CLASS);
    EXPECT_FALSE(responses[0].has_character());
}

TEST_F(PacketHandlerTest, DeleteCharacterInLobbyGoesToDBQueue)
{
    ConnectLobby();

    Receive(MakeDelete(), PKT_C_DELETE_CHARACTER);

    EXPECT_NE(TakePushedDBJob(), nullptr);
}

// 입장 중에는 로비 요청을 받지 않는다. 입장 중인 캐릭터를 지우거나 두 번 입장하지 못한다.
TEST_F(PacketHandlerTest, EnterGameBlocksLobbyRequestsUntilFinished)
{
    ConnectLobby();

    Receive(MakeEnterGame(), PKT_C_ENTER_GAME);
    EXPECT_EQ(recordingSession->GetPhase(), SessionPhase::ENTERING_GAME);

    const string log = Receive(MakeEnterGame(), PKT_C_ENTER_GAME) + Receive(MakeDelete(), PKT_C_DELETE_CHARACTER);
    EXPECT_NE(log.find(format("받지 않는 패킷 {}", static_cast<int32>(PKT_C_ENTER_GAME))), string::npos) << log;
    EXPECT_NE(log.find(format("받지 않는 패킷 {}", static_cast<int32>(PKT_C_DELETE_CHARACTER))), string::npos) << log;

    DBQueueRef dbQueue = GDBManager->GetDBQueue(0);
    dbQueue->Stop();
    EXPECT_NE(dbQueue->WaitForSingleJob(), nullptr) << "첫 입장의 소유 확인 잡";
    EXPECT_EQ(dbQueue->WaitForSingleJob(), nullptr) << "버린 요청은 잡을 넣지 않는다";
}

// 로비가 소유를 확인하지 못하면 게임 입장 창구로 넘기지 않고, 세션을 로비로 되돌린 뒤 실패를 응답한다.
TEST_F(PacketHandlerTest, EnterGameWithoutOwnershipCheckReturnsToLobby)
{
    ConnectLobby();
    Receive(MakeEnterGame(), PKT_C_ENTER_GAME);

    JobRef job = TakePushedDBJob();
    ASSERT_NE(job, nullptr);

    stringstream captured;
    streambuf* original = cout.rdbuf(captured.rdbuf());
    job->Execute();
    cout.rdbuf(original);

    EXPECT_EQ(recordingSession->GetPhase(), SessionPhase::LOBBY) << "다시 캐릭터를 골라 입장할 수 있어야 한다";
    const vector<Protocol::S_ENTER_GAME> responses = recordingSession->SentPackets<Protocol::S_ENTER_GAME>(PKT_S_ENTER_GAME);
    ASSERT_EQ(responses.size(), 1u) << "거절해도 응답을 보내야 클라이언트가 기다리지 않는다";
    EXPECT_FALSE(responses[0].success());
}
