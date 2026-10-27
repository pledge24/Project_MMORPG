#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "Network/ItemRequests.h"
#include "Network/ChatRequests.h"
#include "PlayerTestAccess.h"
#include "TestItemTables.h"
#include "RecordingSession.h"
#include "RoomManagerTestAccess.h"
#include "TestLevelTables.h"
#include "TestAreaTables.h"

/*--------------------------------------------------------------
    룸의 아이템과 채팅 요청 테스트

    응답은 요청한 플레이어에게만 가고, 실패는 결과 enum 하나로 싣는다. 착용 외형은 요청한 플레이어를 포함한
    룸의 모든 플레이어에게, 채팅 메시지는 말한 플레이어를 포함한 그 룸의 모든 플레이어에게 통지로 간다.
    다른 룸에는 가지 않는다. 거절한 요청은 통지를 보내지 않는다.

    픽스처 결합도: 맵 표의 두 행으로 시작하지 않은 룸 둘을 ScopedTestRoomManager에 만든다. 요청한 플레이어와
    지켜보는 플레이어를 한 룸에, 다른 플레이어 하나를 다른 룸에 RecordingSession으로 묶어 넣는다.
    칼 하나와 1레벨 전사 레벨 표, 인벤토리 구역 표를 Gamedata::Install로 주입한다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 ROOM_ID = 10;
    constexpr int32 OTHER_ROOM_ID = 20;
    const string REQUESTER_NAME = "검은늑대";
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
}

class ItemChatRequestTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        TestLevelTables::AddWarriorLevel(tables, 1).MaxHp = 100;
        TestItemTables::AddGear(tables, SWORD_TEMPLATE_ID, Protocol::GEAR_PART_WEAPON);
        tables.inventorySlotCounts = MakeInventorySections();
        TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(ROOM_ID, 0, 0.f, 0.f, 5000.f));
        TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(OTHER_ROOM_ID, 0, 20000.f, 0.f, 5000.f));
        Gamedata::Install(std::move(tables));

        rooms = make_unique<ScopedTestRoomManager>();
        room = rooms->FindRoom(ROOM_ID);
        otherRoom = rooms->FindRoom(OTHER_ROOM_ID);
        ASSERT_NE(room, nullptr);
        ASSERT_NE(otherRoom, nullptr);

        requester = EnterPlayer(room, requesterSession, REQUESTER_NAME);
        observer = EnterPlayer(room, observerSession, "아린");
        otherRoomPlayer = EnterPlayer(otherRoom, otherRoomSession, "북부상인");
        ASSERT_NE(requester, nullptr);
        ASSERT_NE(observer, nullptr);
        ASSERT_NE(otherRoomPlayer, nullptr);

        requesterSession->ClearSent();
        observerSession->ClearSent();
        otherRoomSession->ClearSent();
    }

    void TearDown() override
    {
        requester.reset();
        observer.reset();
        otherRoomPlayer.reset();
        room.reset();
        otherRoom.reset();
        rooms.reset();
        Gamedata::Install(GamedataTables());
    }

    PlayerRef EnterPlayer(const RoomRef& targetRoom, shared_ptr<RecordingSession>& outSession, const string& name)
    {
        outSession = make_shared<RecordingSession>();

        PlayerProgress progress;
        progress.name = name;
        progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
        progress.level = 1;
        progress.hp = 100;

        PlayerSpawnParams params;
        params.session = outSession;
        params.progress = &progress;
        PlayerRef player = EntityFactory::Create<Player>(params);
        if (player == nullptr)
            return nullptr;

        RoomEnterData enterData{};
        enterData.nextRoomId = targetRoom->GetRoomId();
        if (targetRoom->EnterPlayer(player, enterData) == false)
            return nullptr;

        return player;
    }

    // 요청한 플레이어의 인벤토리에 칼을 넣고, 클라이언트가 착용 요청에 실을 칸과 uid를 돌려준다.
    Protocol::C_EQUIP_GEAR MakeEquipSword()
    {
        const optional<SlotAddress> address = PlayerTestAccess::AddNewItem(*requester, SWORD_TEMPLATE_ID);
        EXPECT_TRUE(address.has_value());
        const ItemSlotRequest request = MakeInventoryRequest(*requester, address.value_or(SlotAddress{}));

        Protocol::C_EQUIP_GEAR pkt;
        pkt.mutable_address()->set_container(Protocol::CONTAINER_TYPE_INVENTORY_GEAR);
        pkt.mutable_address()->set_slot_index(request.address.slotIndex);
        pkt.set_item_uid(request.itemUid);
        return pkt;
    }

    unique_ptr<ScopedTestRoomManager> rooms;
    RoomRef room;
    RoomRef otherRoom;
    PlayerRef requester;
    PlayerRef observer;
    PlayerRef otherRoomPlayer;
    shared_ptr<RecordingSession> requesterSession;
    shared_ptr<RecordingSession> observerSession;
    shared_ptr<RecordingSession> otherRoomSession;
};

/* 착용 */

TEST_F(ItemChatRequestTest, EquipRespondsToRequesterAndShowsAppearanceToWholeRoom)
{
    ItemRequests::HandleEquipGear(*room, requester, MakeEquipSword());

    const auto responses = requesterSession->SentPackets<Protocol::S_EQUIP_GEAR>(PKT_S_EQUIP_GEAR);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ITEM_RESULT_SUCCESS);
    EXPECT_FALSE(responses[0].changed_slots().empty());
    EXPECT_TRUE(observerSession->SentPackets<Protocol::S_EQUIP_GEAR>(PKT_S_EQUIP_GEAR).empty()) << "응답은 요청한 플레이어에게만 간다";

    for (const auto& session : { requesterSession, observerSession })
    {
        const auto notices = session->SentPackets<Protocol::S_GEAR_APPEARANCE>(PKT_S_GEAR_APPEARANCE);
        ASSERT_EQ(notices.size(), 1u) << "착용한 플레이어도 외형을 통지로 받는다";
        EXPECT_EQ(notices[0].entity_id(), requester->GetEntityId());
        EXPECT_EQ(notices[0].gear_part(), Protocol::GEAR_PART_WEAPON);
        EXPECT_EQ(notices[0].template_id(), SWORD_TEMPLATE_ID);
    }
}

TEST_F(ItemChatRequestTest, RejectedEquipCarriesResultWithoutAppearance)
{
    Protocol::C_EQUIP_GEAR pkt = MakeEquipSword();
    pkt.set_item_uid(pkt.item_uid() + 1);

    ItemRequests::HandleEquipGear(*room, requester, pkt);

    const auto responses = requesterSession->SentPackets<Protocol::S_EQUIP_GEAR>(PKT_S_EQUIP_GEAR);
    ASSERT_EQ(responses.size(), 1u) << "거절한 요청에도 응답한다";
    EXPECT_EQ(responses[0].result(), Protocol::ITEM_RESULT_SLOT_CHANGED);
    EXPECT_TRUE(responses[0].changed_slots().empty());
    EXPECT_TRUE(requesterSession->SentPackets<Protocol::S_GEAR_APPEARANCE>(PKT_S_GEAR_APPEARANCE).empty());
    EXPECT_TRUE(observerSession->SentPackets<Protocol::S_GEAR_APPEARANCE>(PKT_S_GEAR_APPEARANCE).empty());
}

TEST_F(ItemChatRequestTest, UnequipShowsEmptiedPartToWholeRoom)
{
    ItemRequests::HandleEquipGear(*room, requester, MakeEquipSword());
    requesterSession->ClearSent();
    observerSession->ClearSent();

    const ItemSlotRequest equipped = MakeEquippedRequest(*requester, Protocol::GEAR_PART_WEAPON);
    Protocol::C_UNEQUIP_GEAR pkt;
    pkt.mutable_address()->set_container(Protocol::CONTAINER_TYPE_EQUIPPED);
    pkt.mutable_address()->set_slot_index(Protocol::GEAR_PART_WEAPON);
    pkt.set_item_uid(equipped.itemUid);
    ItemRequests::HandleUnequipGear(*room, requester, pkt);

    const auto responses = requesterSession->SentPackets<Protocol::S_UNEQUIP_GEAR>(PKT_S_UNEQUIP_GEAR);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ITEM_RESULT_SUCCESS);

    const auto notices = observerSession->SentPackets<Protocol::S_GEAR_APPEARANCE>(PKT_S_GEAR_APPEARANCE);
    ASSERT_EQ(notices.size(), 1u);
    EXPECT_EQ(notices[0].gear_part(), Protocol::GEAR_PART_WEAPON);
    EXPECT_EQ(notices[0].template_id(), 0) << "template_id가 0이면 그 부위를 비웠다";
}

/* 소모품 */

TEST_F(ItemChatRequestTest, RejectedUseItemCarriesResult)
{
    Protocol::C_USE_ITEM pkt;
    pkt.mutable_address()->set_container(Protocol::CONTAINER_TYPE_INVENTORY_GEAR);
    pkt.set_item_uid(1);

    ItemRequests::HandleUseItem(*room, requester, pkt);

    const auto responses = requesterSession->SentPackets<Protocol::S_USE_ITEM>(PKT_S_USE_ITEM);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ITEM_RESULT_INVALID_REQUEST);
}

/* 채팅 */

TEST_F(ItemChatRequestTest, ChatRespondsToSpeakerAndRelaysToSpeakersRoomOnly)
{
    Protocol::C_CHAT pkt;
    pkt.set_message("안녕하세요");

    ChatRequests::HandleChat(*room, requester, pkt);

    const auto responses = requesterSession->SentPackets<Protocol::S_CHAT>(PKT_S_CHAT);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::CHAT_RESULT_SUCCESS);
    EXPECT_TRUE(observerSession->SentPackets<Protocol::S_CHAT>(PKT_S_CHAT).empty()) << "응답은 말한 플레이어에게만 간다";

    EXPECT_TRUE(otherRoomSession->SentPackets<Protocol::S_CHAT>(PKT_S_CHAT).empty());
    EXPECT_TRUE(otherRoomSession->SentPackets<Protocol::S_CHAT_MESSAGE>(PKT_S_CHAT_MESSAGE).empty()) << "다른 룸에는 중계하지 않는다";

    for (const auto& session : { requesterSession, observerSession })
    {
        const auto messages = session->SentPackets<Protocol::S_CHAT_MESSAGE>(PKT_S_CHAT_MESSAGE);
        ASSERT_EQ(messages.size(), 1u) << "말한 플레이어도 메시지를 통지로 받는다";
        EXPECT_EQ(messages[0].entity_id(), requester->GetEntityId());
        EXPECT_EQ(messages[0].name(), REQUESTER_NAME);
        EXPECT_EQ(messages[0].message(), "안녕하세요");
    }
}

TEST_F(ItemChatRequestTest, InvalidChatIsRejectedWithoutRelay)
{
    Protocol::C_CHAT pkt;

    ChatRequests::HandleChat(*room, requester, pkt);

    const auto responses = requesterSession->SentPackets<Protocol::S_CHAT>(PKT_S_CHAT);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::CHAT_RESULT_INVALID_MESSAGE);
    EXPECT_TRUE(requesterSession->SentPackets<Protocol::S_CHAT_MESSAGE>(PKT_S_CHAT_MESSAGE).empty());
    EXPECT_TRUE(observerSession->SentPackets<Protocol::S_CHAT_MESSAGE>(PKT_S_CHAT_MESSAGE).empty());
    EXPECT_TRUE(otherRoomSession->SentPackets<Protocol::S_CHAT_MESSAGE>(PKT_S_CHAT_MESSAGE).empty());
}
