#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/P1ItemData.h"
#include "Engine/DataTable.h"

const FP1ItemData* UP1GameDataSettings::FindItemData(int32 TemplateId)
{
    const UDataTable* Table = GetDefault<UP1GameDataSettings>()->ItemTable.LoadSynchronous();
    if (Table == nullptr)
        return nullptr;

    return Table->FindRow<FP1ItemData>(FName(*FString::FromInt(TemplateId)), TEXT("UP1GameDataSettings::FindItemData"), false);
}
