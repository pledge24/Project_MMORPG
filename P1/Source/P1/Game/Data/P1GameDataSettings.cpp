#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/P1ItemData.h"
#include "Game/Data/P1ItemAssetData.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"

const FP1ItemData* UP1GameDataSettings::FindItemData(int32 TemplateId)
{
    const UDataTable* Table = GetDefault<UP1GameDataSettings>()->ItemTable.LoadSynchronous();
    if (Table == nullptr)
        return nullptr;

    return Table->FindRow<FP1ItemData>(FName(*FString::FromInt(TemplateId)), TEXT("UP1GameDataSettings::FindItemData"), false);
}

UTexture2D* UP1GameDataSettings::LoadItemIcon(int32 TemplateId)
{
    const UDataTable* Table = GetDefault<UP1GameDataSettings>()->ItemAssetTable.LoadSynchronous();
    if (Table == nullptr)
        return nullptr;

    const FP1ItemAssetData* AssetData = Table->FindRow<FP1ItemAssetData>(FName(*FString::FromInt(TemplateId)), TEXT("UP1GameDataSettings::LoadItemIcon"), false);
    if (AssetData == nullptr || AssetData->Icon.IsNull())
        return nullptr;

    return AssetData->Icon.LoadSynchronous();
}

TSoftObjectPtr<UWorld> UP1GameDataSettings::FindMapLevel(int32 MapId) const
{
    const TSoftObjectPtr<UWorld>* Found = MapLevels.Find(MapId);
    return Found ? *Found : TSoftObjectPtr<UWorld>();
}
