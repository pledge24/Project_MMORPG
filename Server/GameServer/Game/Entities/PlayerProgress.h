#pragma once
#include "Game/Items/ItemTypes.h"
#include "Game/QuickSlots/QuickSlotTypes.h"

/** 접속이 끊겨도 남는 플레이어의 진행. 불러오기와 저장이 함께 쓰는 사본이라 DB 스레드가 다뤄도 된다. */
struct PlayerProgress
{
    /** 캐릭터 번호, 직업, 이름, 레벨, 마지막 맵과 룸. 레벨은 누적 경험치로 다시 계산하므로 목록에 보일 사본이다. */
    Protocol::PlayerInfo playerInfo;
    /** 마지막 위치. */
    Protocol::PosInfo posInfo;
    /** 누적 경험치, 현재 HP와 MP, 공격력. 플레이어의 스탯과 달리 경험치가 레벨 안의 값이 아니다. */
    Protocol::StatInfo statInfo;
    int64 gold = 0;
    /** 인벤토리와 착용 장비에 든 아이템. 빈 칸은 싣지 않는다. */
    vector<PlacedItem> items;
    /** 등록된 퀵슬롯. 빈 퀵슬롯은 싣지 않는다. */
    vector<QuickSlot> quickSlots;
};
