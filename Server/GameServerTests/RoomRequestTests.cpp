#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "Network/ItemRequests.h"

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

        LevelTemplate level1;
        level1.level = 1;

        GamedataTables tables;
        tables.items[SWORD_TEMPLATE_ID] = sword;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1 });
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
