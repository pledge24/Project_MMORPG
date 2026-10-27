#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/P1ItemAssetData.h"
#include "Game/Data/Generated/P1ItemConsumeEffectRow.h"
#include "Game/Data/Generated/P1ItemEquipStatRow.h"
#include "Game/Data/Generated/P1ItemRow.h"
#include "Game/Data/Generated/P1MonsterRow.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"

namespace
{
    /** 기획 데이터 테이블의 행 이름은 기획 데이터의 키(번호)다. 종류별 표에 행이 없는 것은 정상이라 경고하지 않는다. */
    template <typename RowType>
    const RowType* FindRowById(const TSoftObjectPtr<UDataTable>& TableRef, int32 Id, const TCHAR* Context)
    {
        const UDataTable* Table = TableRef.LoadSynchronous();
        if (Table == nullptr)
            return nullptr;

        return Table->FindRow<RowType>(FName(*FString::FromInt(Id)), Context, false);
    }
}

const FP1ItemRow* UP1GameDataSettings::FindItemRow(int32 ItemId)
{
    return FindRowById<FP1ItemRow>(GetDefault<UP1GameDataSettings>()->ItemTable, ItemId, TEXT("UP1GameDataSettings::FindItemRow"));
}

const FP1ItemEquipStatRow* UP1GameDataSettings::FindItemEquipStatRow(int32 ItemId)
{
    return FindRowById<FP1ItemEquipStatRow>(GetDefault<UP1GameDataSettings>()->ItemEquipStatTable, ItemId, TEXT("UP1GameDataSettings::FindItemEquipStatRow"));
}

const FP1ItemConsumeEffectRow* UP1GameDataSettings::FindItemConsumeEffectRow(int32 ItemId)
{
    return FindRowById<FP1ItemConsumeEffectRow>(GetDefault<UP1GameDataSettings>()->ItemConsumeEffectTable, ItemId, TEXT("UP1GameDataSettings::FindItemConsumeEffectRow"));
}

UTexture2D* UP1GameDataSettings::LoadItemIcon(int32 ItemId)
{
    const FP1ItemAssetData* AssetData = FindRowById<FP1ItemAssetData>(GetDefault<UP1GameDataSettings>()->ItemAssetTable, ItemId, TEXT("UP1GameDataSettings::LoadItemIcon"));
    if (AssetData == nullptr || AssetData->Icon.IsNull())
        return nullptr;

    return AssetData->Icon.LoadSynchronous();
}

const FP1MonsterRow* UP1GameDataSettings::FindMonsterRow(int32 MonsterId)
{
    return FindRowById<FP1MonsterRow>(GetDefault<UP1GameDataSettings>()->MonsterTable, MonsterId, TEXT("UP1GameDataSettings::FindMonsterRow"));
}

TSoftObjectPtr<UWorld> UP1GameDataSettings::FindMapLevel(int32 MapId) const
{
    const TSoftObjectPtr<UWorld>* Found = MapLevels.Find(MapId);
    return Found ? *Found : TSoftObjectPtr<UWorld>();
}
