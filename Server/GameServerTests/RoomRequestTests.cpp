#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "Network/ItemRequests.h"
#include "Game/Inventory/InventoryComponent.h"
#include "RecordingSession.h"
#include "PlayerTestAccess.h"

/*--------------------------------------------------------------
    룸 요청 처리 테스트

    룸 큐의 잡은 핸들러가 넣은 뒤 나중에 돈다. 그사이 연결이 끊겨 세션이 사라졌을 수 있으므로,
    잡은 세션이 없어도 응답을 건너뛰고 끝나야 한다.

    픽스처 결합도: Room::Create로 룸을 만들고 Start()는 부르지 않는다. 플레이어는 세션 없이 불러온 진행으로
    만들어 EnterPlayer로 넣는다. 아이템, 1레벨 전사 레벨 표, 인벤토리 구역 표를 Gamedata::Install로 주입한다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 ROOM_ID = 10;
    constexpr int32 TOWN_ROOM_ID = 1;
    constexpr int32 TOWN_MAP_ID = 1001;
    constexpr float TOWN_RESPAWN_X = 300.f;
    constexpr int32 PLAYER_MAX_HP = 100;
    constexpr int32 SWORD_TEMPLATE_ID = 1001;

    PlayerProgress MakeProgressWithSword()
    {
        PlayerProgress progress;
        progress.classId = Protocol::CLASS_TYPE_WARRIOR;
        progress.level = 1;

        PlacedItem sword;
        sword.address = SlotAddress{ ContainerType::INVENTORY_GEAR, 0 };
        sword.item.itemUid = 500;
        sword.item.templateId = SWORD_TEMPLATE_ID;
        sword.item.gear = GearDetail{};
        progress.items.push_back(sword);
        return progress;
    }
}

class RoomRequestTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ItemTemplate sword;
        sword.templateId = SWORD_TEMPLATE_ID;
        sword.itemType = Protocol::ITEM_TYPE_GEAR;
        sword.gearType = Protocol::GEAR_TYPE_WEAPON;
        sword.sellable = true;

        LevelTemplate level1;
        level1.level = 1;
        level1.maxHp = PLAYER_MAX_HP;

        // 마을은 룸 객체 없이 맵 표에만 둔다. 리스폰 규칙은 맵 표만 읽는다.
        MapTemplate town;
        town.templateId = TOWN_ROOM_ID;
        town.mapId = TOWN_MAP_ID;
        town.respawnPoint = TemplatePos{ TOWN_RESPAWN_X, 0.f, 0.f };

        GamedataTables tables;
        tables.items[SWORD_TEMPLATE_ID] = sword;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1 });
        tables.maps[TOWN_ROOM_ID] = town;
        tables.townRoomId = TOWN_ROOM_ID;
        tables.inventorySlotCounts = MakeInventorySections();
        Gamedata::Install(std::move(tables));

        MapTemplate mapTemplate;
        mapTemplate.templateId = ROOM_ID;
        mapTemplate.depthHalfExtent = 5000.f;
        mapTemplate.widthHalfExtent = 5000.f;
        room = Room::Create(mapTemplate);
        ASSERT_NE(room, nullptr);

        const PlayerProgress progress = MakeProgressWithSword();
        PlayerSpawnParams params;
        params.progress = &progress;
        player = EntityFactory::Create<Player>(params);
        ASSERT_NE(player, nullptr);

        RoomEnterData enterData{};
        enterData.nextRoomId = ROOM_ID;
        ASSERT_TRUE(room->EnterPlayer(player, enterData));
    }

    void TearDown() override
    {
        player.reset();
        room.reset();
        Gamedata::Install(GamedataTables());
    }

    /** 클라이언트가 인벤토리의 칼을 가리키는 요청. */
    template<typename RequestType>
    RequestType MakeSwordRequest(Protocol::ContainerType container, int32 slotIndex) const
    {
        RequestType pkt;
        pkt.mutable_address()->set_container(container);
        pkt.mutable_address()->set_slot_index(slotIndex);
        pkt.set_item_uid(500);
        return pkt;
    }

    RoomRef room;
    PlayerRef player;
};

// 클라이언트가 보낸 위치를 그대로 썼다. 조작한 클라이언트는 룸 안 어디로든 순간이동했다.
// 판정 기준은 MoveValidationTests가 덮는다. 여기서는 룸이 그 판정으로 패킷을 버리는지만 본다.
TEST_F(RoomRequestTest, MoveFartherThanAllowedIsIgnored)
{
    const float startX = player->GetPosInfo().pos().x();

    Protocol::C_MOVE movePkt;
    movePkt.mutable_info()->mutable_pos()->set_x(startX + 3000.f);
    room->C_HandleMove(movePkt, player);

    EXPECT_FLOAT_EQ(player->GetPosInfo().pos().x(), startX) << "한 번에 허용 거리를 넘는 이동은 버린다";
}

// 이동은 보낸 사람의 위치만 바꾼다. 패킷의 엔티티 번호로 대상을 찾으면 같은 룸의 다른 플레이어를 옮길 수 있다.
TEST_F(RoomRequestTest, MoveChangesOnlySenderPosition)
{
    const PlayerProgress progress = MakeProgressWithSword();
    PlayerSpawnParams params;
    params.progress = &progress;
    PlayerRef victim = EntityFactory::Create<Player>(params);
    ASSERT_NE(victim, nullptr);
    RoomEnterData enterData{};
    enterData.nextRoomId = ROOM_ID;
    ASSERT_TRUE(room->EnterPlayer(victim, enterData));
    const float victimX = victim->GetPosInfo().pos().x();

    Protocol::C_MOVE movePkt;
    movePkt.mutable_info()->set_entity_id(victim->GetEntityId());
    movePkt.mutable_info()->mutable_pos()->set_x(victimX + 30.f);
    room->C_HandleMove(movePkt, player);

    EXPECT_FLOAT_EQ(victim->GetPosInfo().pos().x(), victimX) << "다른 플레이어의 위치가 바뀌면 그 계정의 진행이 손상된다";
    EXPECT_FLOAT_EQ(player->GetPosInfo().pos().x(), victimX + 30.f);
    EXPECT_EQ(player->GetPosInfo().entity_id(), player->GetEntityId()) << "위치의 엔티티 id는 패킷 값이 아니라 보낸 사람의 것이다";
}

// 아이템 요청은 디스패치할 때의 룸 큐에서 돈다. 그사이 플레이어가 다른 룸으로 옮겼으면 그 플레이어의 상태는
// 새 룸 큐의 것이므로 옛 룸 큐에서 바꾸지 않는다.
TEST_F(RoomRequestTest, ItemRequestOfPlayerWhoLeftIsIgnored)
{
    shared_ptr<RecordingSession> session = make_shared<RecordingSession>();
    const PlayerProgress progress = MakeProgressWithSword();
    PlayerSpawnParams params;
    params.session = session;
    params.progress = &progress;
    PlayerRef traveler = EntityFactory::Create<Player>(params);
    ASSERT_NE(traveler, nullptr);

    RoomEnterData enterData{};
    enterData.nextRoomId = ROOM_ID;
    ASSERT_TRUE(room->EnterPlayer(traveler, enterData));
    ASSERT_TRUE(room->LeavePlayer(traveler, true));

    const auto sellPkt = MakeSwordRequest<Protocol::C_SELL_ITEM>(Protocol::CONTAINER_TYPE_INVENTORY_GEAR, 0);
    ItemRequests::HandleSellItem(*room, traveler, sellPkt);

    EXPECT_NE(traveler->GetInventory().FindItem(SlotAddress{ ContainerType::INVENTORY_GEAR, 0 }), nullptr)
        << "룸을 떠난 플레이어의 소지품을 옛 룸 큐에서 바꾸면 새 룸 큐와 경쟁한다";
}

// 착용과 해제는 실패 응답 전에만 세션을 확인하고, 성공 응답은 확인 없이 보냈다.
// 세션이 없는 플레이어(잡이 기다리는 사이 끊긴 세션)로 성공 경로를 타면 널 세션을 역참조했다.
TEST_F(RoomRequestTest, EquipAndUnequipWithoutSessionDoNotSend)
{
    const auto equipPkt = MakeSwordRequest<Protocol::C_EQUIP_GEAR>(Protocol::CONTAINER_TYPE_INVENTORY_GEAR, 0);
    ItemRequests::HandleEquipGear(*room, player, equipPkt);

    const auto unequipPkt = MakeSwordRequest<Protocol::C_UNEQUIP_GEAR>(Protocol::CONTAINER_TYPE_EQUIPPED, Protocol::GEAR_TYPE_WEAPON);
    ItemRequests::HandleUnequipGear(*room, player, unequipPkt);

    SUCCEED() << "세션이 없어도 응답을 건너뛰고 끝나야 한다";
}

// 사망한 채 끊긴 플레이어는 사망 화면에서 마을 리스폰을 누른 상태로 저장한다. 사망 여부는 저장되지 않아서,
// 그대로 저장하면 다시 접속했을 때 HP 0으로 살아서 들어온다. 저장은 룸에서 뺀 뒤에 뜨므로 리스폰 규칙이
// 소속 룸이나 전역 룸 관리자에 기대면 이 저장이 실패한다.
TEST_F(RoomRequestTest, DeadPlayerIsSavedAsTownRespawn)
{
    Protocol::AttackInfo lethalAttack;
    lethalAttack.set_damage(PLAYER_MAX_HP * 10);
    player->OnHit(nullptr, lethalAttack);
    ASSERT_TRUE(player->IsDead());

    optional<PlayerSaveData> saveData = room->HandleDisconnect(player);
    ASSERT_TRUE(saveData.has_value());

    EXPECT_EQ(saveData->progress.mapId, TOWN_MAP_ID) << "DB에는 룸이 아니라 맵과 좌표를 저장한다";
    EXPECT_EQ(player->GetRoomId(), TOWN_ROOM_ID);
    EXPECT_FLOAT_EQ(saveData->progress.pos.x, TOWN_RESPAWN_X);
    EXPECT_EQ(saveData->progress.hp, PLAYER_MAX_HP / 2)
        << "마을 리스폰은 HP를 절반으로 채운다";
}
