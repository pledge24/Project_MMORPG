#include "Validation/P1DesignDataPairCheck.h"
#include "Engine/DataTable.h"
#include "Game/Data/P1ItemAssetData.h"
#include "Game/Data/P1MonsterAssetData.h"
#include "Game/Data/P1NpcAssetData.h"
#include "Misc/PackageName.h"

namespace
{
    using FIssue = FP1DesignDataPairCheck::FIssue;

    /** 비어 있지 않은데 가리키는 에셋이 없는 참조. 네이티브 클래스는 패키지 파일이 없어서 메모리에서 찾는다. */
    bool IsBroken(const FSoftObjectPath& Path)
    {
        if (Path.IsNull() || Path.ResolveObject() != nullptr)
            return false;

        if (!FPackageName::DoesPackageExist(Path.GetLongPackageName()))
            return true;

        // 에셋 패키지에는 패키지 이름과 같은 에셋 하나가 든다. 블루프린트 클래스는 그 이름에 _C가 붙는다.
        FString AssetName = Path.GetAssetName();
        AssetName.RemoveFromEnd(TEXT("_C"));
        return AssetName != FPackageName::GetShortName(Path.GetLongPackageName());
    }

    /** 선택 참조는 깨졌을 때만 오류다. */
    void CheckOptional(TArray<FIssue>& Issues, const FString& Owner, const TCHAR* Field, const FSoftObjectPath& Path)
    {
        if (IsBroken(Path))
            Issues.Add({ true, FString::Printf(TEXT("%s의 %s 참조가 없는 에셋을 가리킴: %s"), *Owner, Field, *Path.ToString()) });
    }

    /** 필수 참조는 비어도 깨져도 오류다. */
    void CheckRequired(TArray<FIssue>& Issues, const FString& Owner, const TCHAR* Field, const FSoftObjectPath& Path)
    {
        if (Path.IsNull())
            Issues.Add({ true, FString::Printf(TEXT("%s의 %s 참조가 비어 있음"), *Owner, Field) });
        else
            CheckOptional(Issues, Owner, Field, Path);
    }

    /**
     * 두 테이블의 행 이름을 양쪽에서 대조하고, 짝이 있는 행은 CheckRow로 참조를 검사한다.
     * 에셋 테이블의 행 구조체가 RowType이 아니면 행을 읽지 않고 오류 하나로 끝낸다.
     */
    template <typename RowType>
    TArray<FIssue> CheckPair(const UDataTable& DesignTable, const UDataTable& AssetTable, const TCHAR* Noun,
        TFunctionRef<void(TArray<FIssue>&, FName, const FString&, const RowType&)> CheckRow)
    {
        TArray<FIssue> Issues;
        if (AssetTable.GetRowStruct() != RowType::StaticStruct())
        {
            Issues.Add({ true, FString::Printf(TEXT("에셋 테이블 %s의 행 구조체가 %s가 아님"),
                *AssetTable.GetName(), *RowType::StaticStruct()->GetStructCPPName()) });
            return Issues;
        }

        for (const TPair<FName, uint8*>& DesignRow : DesignTable.GetRowMap())
        {
            const FString Owner = FString::Printf(TEXT("%s %s"), Noun, *DesignRow.Key.ToString());
            const uint8* const* AssetRow = AssetTable.GetRowMap().Find(DesignRow.Key);
            if (AssetRow == nullptr)
            {
                Issues.Add({ true, FString::Printf(TEXT("%s의 행이 에셋 테이블 %s에 없음"), *Owner, *AssetTable.GetName()) });
                continue;
            }

            CheckRow(Issues, DesignRow.Key, Owner, *reinterpret_cast<const RowType*>(*AssetRow));
        }

        // 기획에서 지운 행은 게임이 읽지 않으므로 지울 때까지 경고로 알린다.
        for (const TPair<FName, uint8*>& AssetRow : AssetTable.GetRowMap())
        {
            if (!DesignTable.GetRowMap().Contains(AssetRow.Key))
            {
                Issues.Add({ false, FString::Printf(TEXT("에셋 테이블 %s의 %s %s 행이 수치 테이블 %s에 없음(기획에서 지운 행)"),
                    *AssetTable.GetName(), Noun, *AssetRow.Key.ToString(), *DesignTable.GetName()) });
            }
        }

        return Issues;
    }
}

TArray<FP1DesignDataPairCheck::FIssue> FP1DesignDataPairCheck::CheckItems(const UDataTable& ItemTable, const UDataTable& ItemEquipStatTable, const UDataTable& ItemAssetTable)
{
    return CheckPair<FP1ItemAssetData>(ItemTable, ItemAssetTable, TEXT("아이템"),
        [&ItemEquipStatTable](TArray<FIssue>& Issues, FName RowName, const FString& Owner, const FP1ItemAssetData& AssetData)
        {
            CheckRequired(Issues, Owner, TEXT("Icon"), AssetData.Icon.ToSoftObjectPath());
            CheckOptional(Issues, Owner, TEXT("SkeletalMesh"), AssetData.SkeletalMesh.ToSoftObjectPath());
            CheckOptional(Issues, Owner, TEXT("StaticMesh"), AssetData.StaticMesh.ToSoftObjectPath());

            // 장비는 착용 외형이 필요하다. 부위마다 스켈레탈 메시나 스태틱 메시 중 하나를 쓴다.
            const bool bGear = ItemEquipStatTable.GetRowMap().Contains(RowName);
            if (bGear && AssetData.SkeletalMesh.IsNull() && AssetData.StaticMesh.IsNull())
                Issues.Add({ true, FString::Printf(TEXT("%s는 장비인데 SkeletalMesh와 StaticMesh가 모두 비어 있음"), *Owner) });
        });
}

TArray<FP1DesignDataPairCheck::FIssue> FP1DesignDataPairCheck::CheckMonsters(const UDataTable& MonsterTable, const UDataTable& MonsterAssetTable)
{
    return CheckPair<FP1MonsterAssetData>(MonsterTable, MonsterAssetTable, TEXT("몬스터"),
        [](TArray<FIssue>& Issues, FName RowName, const FString& Owner, const FP1MonsterAssetData& AssetData)
        {
            // 몬스터는 공용 클래스 하나로 스폰되므로, 메시와 애니메이션 블루프린트가 없으면 보이지 않거나 움직이지 않는다.
            CheckRequired(Issues, Owner, TEXT("SkeletalMesh"), AssetData.SkeletalMesh.ToSoftObjectPath());
            CheckRequired(Issues, Owner, TEXT("AnimClass"), AssetData.AnimClass.ToSoftObjectPath());

            for (const TSoftObjectPtr<UMaterialInterface>& Material : AssetData.Materials)
                CheckOptional(Issues, Owner, TEXT("Materials"), Material.ToSoftObjectPath());
            for (const TSoftObjectPtr<UAnimMontage>& Montage : AssetData.AttackMontages)
                CheckOptional(Issues, Owner, TEXT("AttackMontages"), Montage.ToSoftObjectPath());
        });
}

TArray<FP1DesignDataPairCheck::FIssue> FP1DesignDataPairCheck::CheckNpcs(const UDataTable& NpcTable, const UDataTable& NpcAssetTable)
{
    return CheckPair<FP1NpcAssetData>(NpcTable, NpcAssetTable, TEXT("NPC"),
        [](TArray<FIssue>& Issues, FName RowName, const FString& Owner, const FP1NpcAssetData& AssetData)
        {
            // NPC는 공용 클래스 하나로 세우고 외형을 이 행으로 입히므로, 메시가 없으면 보이지 않는 NPC가 된다.
            CheckRequired(Issues, Owner, TEXT("SkeletalMesh"), AssetData.SkeletalMesh.ToSoftObjectPath());
            CheckOptional(Issues, Owner, TEXT("IdleAnimation"), AssetData.IdleAnimation.ToSoftObjectPath());
        });
}
