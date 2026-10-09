#include "Core/pch.h"
#include "DB/ProgressStorage.h"
#include "DB/CharacterStateDAO.h"
#include "DB/ItemDAO.h"
#include "Game/Entities/PlayerSaveData.h"

bool ProgressStorage::Load(DBConnection& conn, int64 userId, int64 characterId, OUT PlayerProgress& progress)
{
    // 1. 캐릭터 기본 정보(이름, 레벨 등)
    if (CharacterStateDAO::LoadCharacter(conn, userId, characterId, OUT progress) == false)
    {
        GLogger->Warning("캐릭터 {} 기본 정보 불러오기 실패", characterId);
        return false;
    }

    // 2. 캐릭터 마지막 상태(LastState)
    if (CharacterStateDAO::LoadLastState(conn, characterId, OUT progress) == false)
    {
        GLogger->Warning("캐릭터 {} 마지막 상태(LastState) 불러오기 실패", characterId);
        return false;
    }

    // 3. 캐릭터 소유 아이템
    if (ItemDAO::LoadItems(conn, characterId, OUT progress) == false)
    {
        GLogger->Warning("캐릭터 {} 소유 아이템 불러오기 실패", characterId);
        return false;
    }

    return true;
}

bool ProgressStorage::Save(DBConnection& conn, const PlayerSaveData& data)
{
    const int64 characterId = data.progress.playerInfo.character_id();

    // 세 단계가 따로 확정되면 레벨은 오르고 아이템은 저장되지 않는 식으로 진행이 어긋난다.
    if (conn.BeginTransaction() == false)
    {
        GLogger->Error("캐릭터 {} 저장 트랜잭션을 시작하지 못했습니다", characterId);
        return false;
    }

    auto rollback = [&](string_view step)
        {
            GLogger->Error("캐릭터 {} {} 저장 실패. 이번 저장을 모두 되돌립니다", characterId, step);
            conn.Rollback();
            return false;
        };

    // 1. 캐릭터 기본 정보(레벨)
    if (CharacterStateDAO::SaveCharacter(conn, data) == false)
        return rollback("기본 정보");

    // 2. 캐릭터 마지막 상태(LastState)
    if (CharacterStateDAO::SaveLastState(conn, data) == false)
        return rollback("마지막 상태(LastState)");

    // 3. 캐릭터 소유 아이템
    if (ItemDAO::SaveItems(conn, data) == false)
        return rollback("소유 아이템");

    if (conn.Commit() == false)
    {
        GLogger->Error("캐릭터 {} 저장을 확정하지 못했습니다", characterId);
        return false;
    }

    return true;
}
