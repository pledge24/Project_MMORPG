#include "pch.h"
#include "Player.h"
#include "Inventory.h"
#include "EquippedGear.h"

Player::Player()
{
	_isPlayer = true;

    playerInfo = objectInfo->mutable_player_info();
    statInfo = playerInfo->mutable_stat_info();

    inventory = make_shared<Inventory>();
    equippedGear = make_shared<EquippedGear>();

    inventory->player = static_pointer_cast<Player>(shared_from_this());
    equippedGear->player = static_pointer_cast<Player>(shared_from_this());
}

Player::~Player()
{
}

bool Player::Init()
{
    // inventory 채우기
    inventory->Init(playerInfo);

    // equippedGear 채우기
    equippedGear->Init(playerInfo);

    // 최종 스텟 계산 + playerInfo에 계산 결과 채워넣기
    bool success = CalculateFinalStat();

    return success;
}

bool Player::CalculateFinalStat()
{
    struct FinalStat
    {
        int32 maxHp = 0;
        int32 maxMp = 0;
        int32 physical_attack = 0;
        int32 magical_attack = 0;
    } finalStat;

    // 스킬 패시브, 내실 등 캐릭터 스텟을 올릴 수 있는 요소가 추가되면 여기에 작성...
    // ===========================================================================
    
    // 1. 레벨당 캐릭터 기본 스텟
    DataTable& classLevelDataTable = (*Gamedata::ClassLevelDataTableMappings[playerInfo->class_()]);
    int32 level = playerInfo->level();
    finalStat.maxHp += classLevelDataTable[level]["maxHp"];
    finalStat.maxMp += classLevelDataTable[level]["maxMp"];
    finalStat.physical_attack += classLevelDataTable[level]["physicalAttack"];
    finalStat.magical_attack += classLevelDataTable[level]["magicalAttack"];

    // 2. 장착 중이 장비 스텟 추가
    for (const auto& slot : playerInfo->equipped_gear())
    {
        const Protocol::Item& item = slot.item();
        finalStat.maxHp += Gamedata::ItemDataTable[item.template_id()]["hp"];
        finalStat.maxMp += Gamedata::ItemDataTable[item.template_id()]["mp"];
        finalStat.physical_attack += Gamedata::ItemDataTable[item.template_id()]["physicalAttack"];
        finalStat.magical_attack += Gamedata::ItemDataTable[item.template_id()]["magicalAttack"];
    }

    // validate
    try
    {
        if (statInfo->hp() > finalStat.maxHp)
            throw wstring(L"현재 HP가 최대 HP를 초과");
        if (statInfo->mp() > finalStat.maxMp)
            throw wstring(L"현재 MP가 최대 MP를 초과");
        if(statInfo->physical_attack() != finalStat.physical_attack)
            throw wstring(L"물리 공격력이 계산 결과와 일치하지 않음");
        if (statInfo->magical_attack() != finalStat.magical_attack)
            throw wstring(L"마법 공격력이 계산 결과와 일치하지 않음");
    }
    catch (wstring& cause)
    {
        wcerr << L"스텟 계산에 문제가 생겼습니다. 사유: " << cause << endl;
        return false;
    }

    // Protocol::statInfo에 최종 스텟 적용
    statInfo->set_max_hp(finalStat.maxHp);
    statInfo->set_max_mp(finalStat.maxMp);
    statInfo->set_physical_attack(finalStat.physical_attack);
    statInfo->set_magical_attack(finalStat.magical_attack);

    return true;
}
