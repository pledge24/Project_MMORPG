#include "Core/pch.h"
#include <gtest/gtest.h>
#include "RecordingSession.h"

/*--------------------------------------------------------------
    패킷 핸들러의 세션 상태 검사 테스트

    세션 상태(로그인 여부, 플레이어 유무)는 패킷 핸들러가 본다. 로그인하지 않은 세션의 캐릭터 요청이
    DB 큐까지 가면 생성은 계정 번호 0으로 INSERT를 시도한다(TD-019).

    픽스처 결합도: 전역 DB 관리자(GDBManager)에 큐 하나를 만들고 끝나면 비운다. 핸들러가 큐에 잡을 넣었는지는
    큐를 멈춘 뒤 WaitForSingleJob으로 본다. 멈춘 큐는 남은 잡이 없으면 블로킹하지 않고 nullptr를 돌려준다.
    세션은 보낸 패킷을 기록하는 RecordingSession이다. 1레벨 전사 레벨 표를 Gamedata::Install로 주입한다.
---------------------------------------------------------------*/

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

        GDBManager->Init(1);
        recordingSession = make_shared<RecordingSession>();
        session = recordingSession;
    }

    void TearDown() override
    {
        GDBManager->Clear();
        Gamedata::Install(GamedataTables());
    }

    /** 핸들러가 DB 큐에 잡을 넣었으면 true. 큐를 멈추므로 테스트마다 한 번만 부른다. */
    bool DBJobWasPushed()
    {
        DBQueueRef dbQueue = GDBManager->GetDBQueue(0);
        dbQueue->Stop();
        return dbQueue->WaitForSingleJob() != nullptr;
    }

    shared_ptr<RecordingSession> recordingSession;
    PacketSessionRef session;
};

TEST_F(PacketHandlerTest, CreateCharacterBeforeLoginIsRejected)
{
    Protocol::C_CREATE_CHARACTER pkt;
    pkt.mutable_character()->set_class_(Protocol::CLASS_TYPE_WARRIOR);
    pkt.mutable_character()->set_name("전사");

    Handle_C_CREATE_CHARACTER(session, pkt);

    EXPECT_FALSE(DBJobWasPushed()) << "로그인하지 않은 세션의 생성 요청은 계정 번호 0으로 INSERT를 시도한다";
    const vector<Protocol::S_CREATE_CHARACTER> responses = recordingSession->SentPackets<Protocol::S_CREATE_CHARACTER>(PKT_S_CREATE_CHARACTER);
    ASSERT_EQ(responses.size(), 1u) << "거절해도 응답을 보내야 클라이언트가 기다리지 않는다";
    EXPECT_FALSE(responses[0].success());
}

TEST_F(PacketHandlerTest, DeleteCharacterBeforeLoginIsRejected)
{
    Protocol::C_DELETE_CHARACTER pkt;
    pkt.set_character_id(42);

    Handle_C_DELETE_CHARACTER(session, pkt);

    EXPECT_FALSE(DBJobWasPushed());
    const vector<Protocol::S_DELETE_CHARACTER> responses = recordingSession->SentPackets<Protocol::S_DELETE_CHARACTER>(PKT_S_DELETE_CHARACTER);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_FALSE(responses[0].success());
}

TEST_F(PacketHandlerTest, EnterGameBeforeLoginIsRejected)
{
    Protocol::C_ENTER_GAME pkt;
    pkt.set_character_id(42);

    Handle_C_ENTER_GAME(session, pkt);

    EXPECT_FALSE(DBJobWasPushed());
    const vector<Protocol::S_ENTER_GAME> responses = recordingSession->SentPackets<Protocol::S_ENTER_GAME>(PKT_S_ENTER_GAME);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_FALSE(responses[0].success());
}
