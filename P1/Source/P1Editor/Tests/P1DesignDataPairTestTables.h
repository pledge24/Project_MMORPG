#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Game/Data/P1ItemAssetData.h"
#include "Game/Data/P1MonsterAssetData.h"

/**
 * 짝 검사 테스트가 쓰는 임시 데이터 테이블을 만드는 도우미.
 *
 * 수치 테이블은 행 이름만 쓰므로 빈 행으로 채우고, 에셋 테이블은 에셋 행 구조체로 채운다.
 * 만든 테이블은 이 도우미가 살아 있는 동안 남는다.
 */
class FP1DesignDataPairTestTables
{
public:
    ~FP1DesignDataPairTestTables()
    {
        for (UDataTable* Table : Tables)
            Table->RemoveFromRoot();
    }

    UDataTable* MakeDesignTable(const TArray<int32>& Ids)
    {
        UDataTable* Table = MakeTable(FTableRowBase::StaticStruct());
        for (int32 Id : Ids)
            Table->AddRow(RowName(Id), FTableRowBase());
        return Table;
    }

    UDataTable* MakeItemAssetTable(const TMap<int32, FP1ItemAssetData>& Rows)
    {
        UDataTable* Table = MakeTable(FP1ItemAssetData::StaticStruct());
        for (const TPair<int32, FP1ItemAssetData>& Row : Rows)
            Table->AddRow(RowName(Row.Key), Row.Value);
        return Table;
    }

    UDataTable* MakeMonsterAssetTable(const TMap<int32, FP1MonsterAssetData>& Rows)
    {
        UDataTable* Table = MakeTable(FP1MonsterAssetData::StaticStruct());
        for (const TPair<int32, FP1MonsterAssetData>& Row : Rows)
            Table->AddRow(RowName(Row.Key), Row.Value);
        return Table;
    }

    /** 빈 경로는 참조를 비워 둔다. */
    static FP1ItemAssetData ItemAsset(const TCHAR* Icon, const TCHAR* InSkeletalMesh = TEXT(""), const TCHAR* InStaticMesh = TEXT(""))
    {
        FP1ItemAssetData AssetData;
        AssetData.Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Icon));
        AssetData.SkeletalMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(InSkeletalMesh));
        AssetData.StaticMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(InStaticMesh));
        return AssetData;
    }

    /** 빈 경로는 참조를 비워 둔다. 몽타주는 공격 몽타주 하나로, 머티리얼은 첫 슬롯 하나로 넣는다. */
    static FP1MonsterAssetData MonsterAsset(const TCHAR* InSkeletalMesh, const TCHAR* AnimClass,
        const TCHAR* AttackMontage = TEXT(""), const TCHAR* Material = TEXT(""))
    {
        FP1MonsterAssetData AssetData;
        AssetData.SkeletalMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(InSkeletalMesh));
        AssetData.AnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(AnimClass));
        if (*AttackMontage != TEXT('\0'))
            AssetData.AttackMontages.Add(TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(AttackMontage)));
        if (*Material != TEXT('\0'))
            AssetData.Materials.Add(TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Material)));
        return AssetData;
    }

    /** 엔진 콘텐츠에 있는 에셋. 테스트가 프로젝트 콘텐츠에 의존하지 않게 한다. */
    static constexpr const TCHAR* Texture = TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture");
    static constexpr const TCHAR* SkeletalMesh = TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube");
    static constexpr const TCHAR* StaticMesh = TEXT("/Engine/EngineMeshes/Sphere.Sphere");
    static constexpr const TCHAR* NativeAnimClass = TEXT("/Script/Engine.AnimInstance");

    /** 어디에도 없는 에셋. */
    static constexpr const TCHAR* Missing = TEXT("/Game/P1/Missing/T_Missing.T_Missing");
    static constexpr const TCHAR* MissingAnimClass = TEXT("/Game/P1/Missing/ABP_Missing.ABP_Missing_C");
    static constexpr const TCHAR* MissingMontage = TEXT("/Game/P1/Missing/AM_Missing.AM_Missing");

    /** 패키지는 있지만 그 안에 없는 객체. */
    static constexpr const TCHAR* WrongObject = TEXT("/Engine/EngineResources/DefaultTexture.WrongName");

private:
    UDataTable* MakeTable(UScriptStruct* RowStruct)
    {
        // 테스트 도중 GC가 돌아도 남도록 루트에 두고, 소멸자에서 뺀다.
        UDataTable* Table = NewObject<UDataTable>(GetTransientPackage());
        Table->AddToRoot();
        Table->RowStruct = RowStruct;
        Tables.Add(Table);
        return Table;
    }

    static FName RowName(int32 Id)
    {
        return FName(*FString::FromInt(Id));
    }

    TArray<UDataTable*> Tables;
};
