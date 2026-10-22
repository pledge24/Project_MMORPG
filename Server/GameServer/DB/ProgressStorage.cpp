#include "Core/pch.h"
#include "DB/ProgressStorage.h"
#include "DB/CharacterStateDAO.h"
#include "DB/ItemDAO.h"
#include "DB/QuickSlotDAO.h"
#include "DB/DAOCommon.h"
#include "Game/Entities/PlayerSaveData.h"
#include "Game/Progress/InitialProgress.h"

bool ProgressStorage::Load(DBConnection& conn, int64 userId, int64 characterId, OUT PlayerProgress& progress)
{
    // 초기 진행 행을 만드는 일과 불러오기를 트랜잭션 하나로 한다. 실패하면 만든 행도 되돌리므로 다음 입장이 다시 만든다.
    // DBError가 아닌 예외도 받아 되돌린다. 열린 트랜잭션을 둔 채 연결을 풀에 돌려주면 다음 잡이 그 위에서 돈다.
    if (conn.BeginTransaction() == false)
    {
        GLogger->Error("캐릭터 {} 불러오기 트랜잭션을 시작하지 못했습니다", characterId);
        return false;
    }

    try
    {
        CharacterLoadResult result = CharacterStateDAO::LoadCharacter(conn, userId, characterId, OUT progress);
        if (result == CharacterLoadResult::NOT_FOUND)
        {
            GLogger->Warning("계정 {}에 캐릭터 {}가 없어 불러오지 않습니다", userId, characterId);
            conn.Rollback();
            return false;
        }

        // 진행 행이 없는 캐릭터는 아직 게임에 입장한 적이 없다. 첫 입장이 초기 진행 행을 만들고 이어서 불러온다.
        if (result == CharacterLoadResult::NO_PROGRESS)
        {
            if (InitialProgress::Fill(OUT progress) == false)
            {
                GLogger->Error("캐릭터 {}의 직업 {}에 레벨 표가 없어 초기 진행을 만들지 못했습니다", characterId, progress.classId);
                conn.Rollback();
                return false;
            }

            CharacterStateDAO::CreateInitialProgress(conn, progress);

            result = CharacterStateDAO::LoadCharacter(conn, userId, characterId, OUT progress);
            if (result != CharacterLoadResult::LOADED)
                throw DBError(__func__, format("초기 진행을 만든 뒤 다시 불러오지 못했다(결과 {})", static_cast<int32>(result)));
        }

        ItemDAO::LoadItems(conn, characterId, OUT progress);
        QuickSlotDAO::LoadQuickSlots(conn, characterId, OUT progress);
    }
    catch (const exception& error)
    {
        GLogger->Error("캐릭터 {} 불러오기 실패: {}", characterId, error.what());
        conn.Rollback();
        return false;
    }

    if (conn.Commit() == false)
    {
        GLogger->Error("캐릭터 {} 불러오기를 확정하지 못했습니다", characterId);
        return false;
    }

    return true;
}

bool ProgressStorage::Save(DBConnection& conn, const PlayerSaveData& data)
{
    const int64 characterId = data.progress.characterId;

    // 단계가 따로 확정되면 레벨은 오르고 아이템은 저장되지 않는 식으로 진행이 어긋난다.
    // DBError가 아닌 예외도 받아 되돌린다. 열린 트랜잭션을 둔 채 연결을 풀에 돌려주면 다음 잡이 그 위에서 돈다.
    if (conn.BeginTransaction() == false)
    {
        GLogger->Error("캐릭터 {} 저장 트랜잭션을 시작하지 못했습니다", characterId);
        return false;
    }

    try
    {
        CharacterStateDAO::SaveCharacter(conn, data);
        CharacterStateDAO::SaveState(conn, data);
        CharacterStateDAO::SaveGold(conn, data);
        ItemDAO::SaveItems(conn, data);
        QuickSlotDAO::SaveQuickSlots(conn, data);
    }
    catch (const exception& error)
    {
        GLogger->Error("캐릭터 {} 저장 실패. 이번 저장을 모두 되돌립니다: {}", characterId, error.what());
        conn.Rollback();
        return false;
    }

    if (conn.Commit() == false)
    {
        GLogger->Error("캐릭터 {} 저장을 확정하지 못했습니다", characterId);
        return false;
    }

    return true;
}
