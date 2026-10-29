#include "Validation/P1DesignDataPairValidator.h"
#include "Engine/DataTable.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Validation/P1DesignDataPairCheck.h"

namespace
{
    enum class EP1DesignDataPair : uint8
    {
        None,
        Item,
        Monster,
        Npc,
    };

    /**
     * 이 테이블이 프로젝트 설정의 어느 짝에 드는지. 수치 테이블과 에셋 테이블 어느 쪽이어도 같은 짝이다.
     * 장비 능력치 테이블은 메시가 필수인 아이템을 정하므로 아이템 짝에 든다.
     */
    EP1DesignDataPair FindPair(const UObject* Table)
    {
        const UP1GameDataSettings* Settings = GetDefault<UP1GameDataSettings>();
        const FSoftObjectPath Path(Table);
        if (Path == Settings->ItemTable.ToSoftObjectPath() || Path == Settings->ItemEquipStatTable.ToSoftObjectPath()
            || Path == Settings->ItemAssetTable.ToSoftObjectPath())
            return EP1DesignDataPair::Item;
        if (Path == Settings->MonsterTable.ToSoftObjectPath() || Path == Settings->MonsterAssetTable.ToSoftObjectPath())
            return EP1DesignDataPair::Monster;
        if (Path == Settings->NpcTable.ToSoftObjectPath() || Path == Settings->NpcAssetTable.ToSoftObjectPath())
            return EP1DesignDataPair::Npc;
        return EP1DesignDataPair::None;
    }

    /** 설정의 테이블을 불러온다. 못 불러오면 MissingSettings에 설정 이름을 더한다. */
    const UDataTable* LoadTable(const TSoftObjectPtr<UDataTable>& TableRef, const TCHAR* SettingName, TArray<FString>& MissingSettings)
    {
        const UDataTable* Table = TableRef.LoadSynchronous();
        if (Table == nullptr)
            MissingSettings.Add(FString::Printf(TEXT("%s(%s)"), SettingName, *TableRef.ToString()));
        return Table;
    }
}

bool UP1DesignDataPairValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
    return Cast<UDataTable>(InObject) != nullptr && FindPair(InObject) != EP1DesignDataPair::None;
}

EDataValidationResult UP1DesignDataPairValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
    const UP1GameDataSettings* Settings = GetDefault<UP1GameDataSettings>();
    TArray<FString> MissingSettings;
    TArray<FP1DesignDataPairCheck::FIssue> Issues;

    // 저장 중인 테이블도 설정 경로로 불러온다. 이미 메모리에 있으므로 고친 내용 그대로 대조한다.
    const EP1DesignDataPair Pair = FindPair(InAsset);
    if (Pair == EP1DesignDataPair::Item)
    {
        const UDataTable* ItemTable = LoadTable(Settings->ItemTable, TEXT("ItemTable"), MissingSettings);
        const UDataTable* EquipStatTable = LoadTable(Settings->ItemEquipStatTable, TEXT("ItemEquipStatTable"), MissingSettings);
        const UDataTable* AssetTable = LoadTable(Settings->ItemAssetTable, TEXT("ItemAssetTable"), MissingSettings);
        if (MissingSettings.IsEmpty())
            Issues = FP1DesignDataPairCheck::CheckItems(*ItemTable, *EquipStatTable, *AssetTable);
    }
    else if (Pair == EP1DesignDataPair::Npc)
    {
        const UDataTable* NpcTable = LoadTable(Settings->NpcTable, TEXT("NpcTable"), MissingSettings);
        const UDataTable* AssetTable = LoadTable(Settings->NpcAssetTable, TEXT("NpcAssetTable"), MissingSettings);
        if (MissingSettings.IsEmpty())
            Issues = FP1DesignDataPairCheck::CheckNpcs(*NpcTable, *AssetTable);
    }
    else
    {
        const UDataTable* MonsterTable = LoadTable(Settings->MonsterTable, TEXT("MonsterTable"), MissingSettings);
        const UDataTable* AssetTable = LoadTable(Settings->MonsterAssetTable, TEXT("MonsterAssetTable"), MissingSettings);
        if (MissingSettings.IsEmpty())
            Issues = FP1DesignDataPairCheck::CheckMonsters(*MonsterTable, *AssetTable);
    }

    for (const FString& Setting : MissingSettings)
    {
        AssetFails(InAsset, FText::FromString(FString::Printf(TEXT("짝 검사에 필요한 테이블을 불러오지 못함: 프로젝트 설정 P1 Game Data의 %s"), *Setting)));
    }

    bool bFailed = !MissingSettings.IsEmpty();
    for (const FP1DesignDataPairCheck::FIssue& Issue : Issues)
    {
        if (Issue.bError)
        {
            AssetFails(InAsset, FText::FromString(Issue.Message));
            bFailed = true;
        }
        else
        {
            AssetWarning(InAsset, FText::FromString(Issue.Message));
        }
    }

    // 통과를 알리지 않으면 검증 메뉴가 이 검증기를 「검사하지 않음」으로 본다.
    if (!bFailed)
        AssetPasses(InAsset);

    return GetValidationResult();
}
