#include "Core/pch.h"
#include "DB/ProgressStorage.h"
#include "DB/CharacterStateDAO.h"
#include "DB/ItemDAO.h"
#include "Game/Entities/PlayerSaveData.h"

bool ProgressStorage::Load(int64 userId, int64 characterId, OUT PlayerProgress& progress)
{
    // 1. 캐릭터 기본 정보(이름, 레벨 등)
    if (CharacterStateDAO::LoadCharacter(userId, characterId, OUT progress) == false)
    {
        GLogger->Warning("캐릭터 {} 기본 정보 불러오기 실패", characterId);
        return false;
    }

    // 2. 캐릭터 마지막 상태(LastState)
    if (CharacterStateDAO::LoadLastState(characterId, OUT progress) == false)
    {
        GLogger->Warning("캐릭터 {} 마지막 상태(LastState) 불러오기 실패", characterId);
        return false;
    }

    // 3. 캐릭터 소유 아이템
    if (ItemDAO::LoadItems(characterId, OUT progress) == false)
    {
        GLogger->Warning("캐릭터 {} 소유 아이템 불러오기 실패", characterId);
        return false;
    }

    return true;
}

void ProgressStorage::Save(const PlayerSaveData& data)
{
    // 1. 캐릭터 기본 정보 업데이트(이름, 레벨 등 필요)
    if (CharacterStateDAO::SaveCharacter(data) == false)
    {
        wcout << L"캐릭터 기본 정보 업데이트 실패" << endl;
        return;
    }

    // 2. 캐릭터 마지막 상태(LastState) 업데이트
    if (CharacterStateDAO::SaveLastState(data) == false)
    {
        wcout << L"캐릭터 마지막 상태(LastState) 업데이트 실패" << endl;
        return;
    }

    // 3. 캐릭터 소유 아이템 정보 업데이트
    if (ItemDAO::SaveItems(data) == false)
    {
        wcout << L"캐릭터 소유 아이템 정보 업데이트 실패" << endl;
        return;
    }
}
