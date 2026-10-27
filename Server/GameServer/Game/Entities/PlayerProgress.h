#pragma once
#include "Game/Items/ItemTypes.h"
#include "Game/QuickSlots/QuickSlotTypes.h"

/** 접속이 끊겨도 남는 플레이어의 진행. 불러오기와 저장이 함께 쓰는 사본이라 DB 스레드가 다뤄도 된다. */
struct PlayerProgress
{
    int64 characterId = 0;
    /** 직업. DB의 Character.ClassId 값이다. */
    int32 classId = 0;
    string name;
    /** 목록에 보일 레벨 사본. 입장할 때 누적 경험치로 다시 계산한다. */
    int32 level = 0;
    /** 마지막 맵. 지역 번호가 아니라 맵 표의 맵 번호다. 룸은 저장하지 않고 입장할 때 맵과 좌표로 고른다. */
    int32 mapId = 0;
    /** 마지막 위치. */
    vector3D pos;
    float yaw = 0.f;
    /** 누적 경험치. 레벨 안의 경험치가 아니다. */
    int64 totalExp = 0;
    int32 hp = 0;
    int32 mp = 0;
    int64 gold = 0;
    /** 인벤토리와 착용 장비에 든 아이템. 빈 칸은 싣지 않는다. */
    vector<PlacedItem> items;
    /** 등록된 퀵슬롯. 빈 퀵슬롯은 싣지 않는다. */
    vector<QuickSlot> quickSlots;
};
