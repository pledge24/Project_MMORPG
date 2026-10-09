#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "Network/ItemRequests.h"
#include "Game/Inventory/InventoryComponent.h"
#include "RecordingSession.h"

/*--------------------------------------------------------------
    룸 요청 처리 테스트

    룸 큐의 잡은 핸들러가 넣은 뒤 나중에 돈다. 그사이 연결이 끊겨 세션이 사라졌을 수 있으므로,
    잡은 세션이 없어도 응답을 건너뛰고 끝나야 한다(TD-006).

    픽스처 결합도: Room::Create로 룸을 만들고 Start()는 부르지 않는다. 플레이어는 세션 없이 불러온 진행으로
    만들어 EnterPlayer로 넣는다. 아이템과 1레벨 전사 레벨 표를 Gamedata::Install로 주입한다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 ROOM_ID = 10;
    constexpr int32 TOWN_ROOM_ID = 1;
    constexpr float TOWN_RESPAWN_X = 300.f;
    constexpr int32 PLAYER_MAX_HP = 100;
    constexpr int32 SWORD_TEMPLATE_ID = 1001;

    PlayerProgress MakeProgressWithSword()
    {
        PlayerProgress progress;
        progress.playerInfo.set_class_(Protocol::CLASS_TYPE_WARRIOR);
        progress.playerInfo.set_level(1);
        progress.playerInfo.set_room_id(ROOM_ID);

        auto* stats = progress.statInfo.mutable_info();
        for (Protocol::StatType statType : { Protocol::STAT_TYPE_EXP, Protocol::STAT_TYPE_HP, Protocol::STAT_TYPE_MP,
            Protocol::STAT_TYPE_PHYSICAL_ATTACK, Protocol::STAT_TYPE_MAGICAL_ATTACK })
            (*stats)[statType] = 0;

        Protocol::Slot* slot = progress.possession.mutable_inventory()->add_gear();
        slot->set_type(Protocol::SLOT_TYPE_INVENTORY_GEAR);
        slot->set_slot_id(0);
        slot->mutable_item()->set_template_id(SWORD_TEMPLATE_ID);
        slot->mutable_item()->set_item_uid(500);
        slot->mutable_item()->set_count(1);
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
        town.respawnPoint = TemplatePos{ TOWN_RESPAWN_X, 0.f, 0.f };

        GamedataTables tables;
        tables.items[SWORD_TEMPLATE_ID] = sword;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1 });
        tables.maps[TOWN_ROOM_ID] = town;
        tables.townRoomId = TOWN_ROOM_ID;
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
        enterData.enterType = Protocol::ENTER_TYPE_INITIAL;
        ASSERT_TRUE(room->EnterPlayer(player, enterData));
    }

    void TearDown() override
    {
        player.reset();
        room.reset();
        Gamedata::Install(GamedataTables());
    }

    Protocol::Slot SwordSlot() const
    {
        Protocol::Slot slot;
        slot.set_type(Protocol::SLOT_TYPE_INVENTORY_GEAR);
        slot.set_slot_id(0);
        slot.mutable_item()->set_template_id(SWORD_TEMPLATE_ID);
        slot.mutable_item()->set_item_uid(500);
        return slot;
    }

    RoomRef room;
    PlayerRef player;
};

// TD-007: 이동은 보낸 사람의 위치만 바꾼다. 패킷의 엔티티 번호로 대상을 찾으면 같은 룸의 다른 플레이어를 옮길 수 있다.
TEST_F(RoomRequestTest, MoveChangesOnlySenderPosition)
{
    const PlayerProgress progress = MakeProgressWithSword();
    PlayerSpawnParams params;
    params.progress = &progress;
    PlayerRef victim = EntityFactory::Create<Player>(params);
    ASSERT_NE(victim, nullptr);
    RoomEnterData enterData{};
    enterData.nextRoomId = ROOM_ID;
    enterData.enterType = Protocol::ENTER_TYPE_INITIAL;
    ASSERT_TRUE(room->EnterPlayer(victim, enterData));
    const float victimX = victim->GetPosInfo().pos().x();

    Protocol::C_MOVE movePkt;
    movePkt.mutable_info()->set_entity_id(victim->GetEntityId());
    movePkt.mutable_info()->mutable_pos()->set_x(victimX + 777.f);
    room->C_HandleMove(movePkt, player);

    EXPECT_FLOAT_EQ(victim->GetPosInfo().pos().x(), victimX) << "다른 플레이어의 위치가 바뀌면 그 계정의 진행이 손상된다";
    EXPECT_FLOAT_EQ(player->GetPosInfo().pos().x(), victimX + 777.f);
    EXPECT_EQ(player->GetPosInfo().entity_id(), player->GetEntityId()) << "위치의 엔티티 id는 패킷 값이 아니라 보낸 사람의 것이다";
}

// TD-021: 처음 입장하면 다른 플레이어 알림(SpawnPlayer)과 룸 정보 복제(ReplicateRoomData)가 둘 다 본인을 실었다.
TEST_F(RoomRequestTest, InitialEntrySpawnsSelfOnce)
{
    shared_ptr<RecordingSession> session = make_shared<RecordingSession>();
    const PlayerProgress progress = MakeProgressWithSword();
    PlayerSpawnParams params;
    params.session = session;
    params.progress = &progress;
    PlayerRef newcomer = EntityFactory::Create<Player>(params);
    ASSERT_NE(newcomer, nullptr);

    Protocol::C_ENTER_ROOM enterPkt;
    enterPkt.set_enter_type(Protocol::ENTER_TYPE_INITIAL);
    enterPkt.set_room_id(ROOM_ID);
    room->C_HandleEnterRoom(enterPkt, newcomer);

    int32 selfSpawnCount = 0;
    for (const Protocol::S_SPAWN& spawnPkt : session->SentPackets<Protocol::S_SPAWN>(PKT_S_SPAWN))
    {
        for (const Protocol::EntityInfo& entity : spawnPkt.entities())
        {
            if (entity.entity_id() == newcomer->GetEntityId())
                selfSpawnCount++;
        }
    }

    EXPECT_EQ(selfSpawnCount, 1) << "클라이언트의 중복 검사를 지우면 내 플레이어가 두 번 처리된다";
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
    enterData.enterType = Protocol::ENTER_TYPE_INITIAL;
    ASSERT_TRUE(room->EnterPlayer(traveler, enterData));
    ASSERT_TRUE(room->LeavePlayer(traveler, true));

    Protocol::C_SELL_ITEM sellPkt;
    *sellPkt.mutable_slot() = SwordSlot();
    ItemRequests::HandleSellItem(*room, traveler, sellPkt);

    EXPECT_TRUE(traveler->GetInventory().GetSlot(Protocol::SLOT_TYPE_INVENTORY_GEAR, 0)->has_item())
        << "룸을 떠난 플레이어의 소지품을 옛 룸 큐에서 바꾸면 새 룸 큐와 경쟁한다";
}

// TD-006: 착용과 해제는 실패 응답 전에만 세션을 확인하고, 성공 응답은 확인 없이 보냈다.
// 세션이 없는 플레이어(잡이 기다리는 사이 끊긴 세션)로 성공 경로를 타면 널 세션을 역참조했다.
TEST_F(RoomRequestTest, EquipAndUnequipWithoutSessionDoNotSend)
{
    Protocol::C_EQUIP_GEAR equipPkt;
    *equipPkt.mutable_slot() = SwordSlot();
    ItemRequests::HandleEquipGear(*room, player, equipPkt);

    Protocol::C_UNEQUIP_GEAR unequipPkt;
    Protocol::Slot equippedSlot;
    equippedSlot.set_type(Protocol::SLOT_TYPE_EQUIPPED);
    equippedSlot.set_slot_id(Protocol::GEAR_TYPE_WEAPON);
    equippedSlot.mutable_item()->set_template_id(SWORD_TEMPLATE_ID);
    equippedSlot.mutable_item()->set_item_uid(500);
    *unequipPkt.mutable_slot() = equippedSlot;
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

    EXPECT_EQ(saveData->progress.playerInfo.room_id(), TOWN_ROOM_ID);
    EXPECT_FLOAT_EQ(saveData->progress.posInfo.pos().x(), TOWN_RESPAWN_X);
    EXPECT_EQ(saveData->progress.statInfo.info().at(Protocol::STAT_TYPE_HP), PLAYER_MAX_HP / 2)
        << "마을 리스폰은 HP를 절반으로 채운다";
}
