#pragma once
#include "Game/Data/Templates.h"

struct GamedataDocuments;

/**
 * 기획 데이터 표의 보관소.
 *
 * 검증을 마친 기획 데이터를 설치하고, 게임 코드가 템플릿 번호로 조회하는 창구가 된다.
 * 설치한 뒤로는 바뀌지 않으므로 여러 스레드가 락 없이 함께 읽는다.
 */
class Gamedata
{
    //~ 설치
public:
    /** 기획표 파일을 읽어 검증하고 설치한다. 실패하면 사유를 로그에 남기고 false. */
    static bool LoadAllGamedata();

    /** 문서를 검증하고 통과하면 설치한다. 실패하면 사유를 돌려주고 이전 표를 그대로 둔다. */
    static optional<string> Load(const GamedataDocuments& documents);

    /**
     * 표 전체를 바꿔 끼운다. 부팅과 테스트에서만 부른다.
     * 다른 스레드가 Find*로 읽는 중에 부르면 안 된다.
     */
    static void Install(GamedataTables tables);

    //~ 조회
public:
    /** Find*는 없는 번호면 nullptr. */
    static const ItemTemplate* FindItem(int32 templateId);
    static const MonsterTemplate* FindMonster(int32 templateId);
    static const MapTemplate* FindMap(int32 templateId);
    static const ClassLevelTable* FindClassLevelTable(int32 classId);

    /** 룸 번호 순서다. */
    static const map<int32, MapTemplate>& GetMaps() { return s_tables.maps; }
    /** 사망한 플레이어가 돌아가는 마을 룸 번호. */
    static int32 GetTownRoomId() { return s_tables.townRoomId; }

    /** 골드인 재화의 번호. DB의 CurrencyId로 쓴다. */
    static int32 GetGoldCurrencyId() { return s_tables.goldCurrencyId; }
    /** 새 캐릭터의 시작 맵, 위치, 골드와 새 계정의 캐릭터 슬롯 수. */
    static const CharacterCreationTemplate& GetCharacterCreation() { return s_tables.characterCreation; }
    /** 인벤토리에서 그 아이템 종류 구역의 칸 수. 구역이 없는 종류(NONE)면 0. */
    static int32 GetInventorySlotCount(Protocol::ItemType itemType);

    //~ 공통
private:
    static GamedataTables s_tables;
};
