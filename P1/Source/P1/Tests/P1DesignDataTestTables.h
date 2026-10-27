#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/Generated/P1ItemConsumeEffectRow.h"
#include "Game/Data/Generated/P1ItemEquipStatRow.h"
#include "Game/Data/Generated/P1ItemRow.h"
#include "Game/Data/Generated/P1MonsterRow.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

/**
 * 저장소의 기획 데이터로 채운 임시 테이블을 프로젝트 설정에 끼우는 테스트 도우미.
 *
 * 테스트가 에디터에서 만드는 데이터 테이블 에셋 없이 DataBuilder의 클라이언트 JSON을 읽게 한다.
 * 살아 있는 동안 UP1GameDataSettings의 기획 데이터 테이블을 바꾸고, 사라질 때 원래 테이블로 되돌린다.
 */
class FP1DesignDataTestTables
{
public:
    FP1DesignDataTestTables()
    {
        UP1GameDataSettings* Settings = GetMutableDefault<UP1GameDataSettings>();
        Install(Settings->ItemTable, FP1ItemRow::StaticStruct(), TEXT("Item"));
        Install(Settings->ItemEquipStatTable, FP1ItemEquipStatRow::StaticStruct(), TEXT("ItemEquipStat"));
        Install(Settings->ItemConsumeEffectTable, FP1ItemConsumeEffectRow::StaticStruct(), TEXT("ItemConsumeEffect"));
        Install(Settings->MonsterTable, FP1MonsterRow::StaticStruct(), TEXT("Monster"));
    }

    ~FP1DesignDataTestTables()
    {
        for (const FInstalled& Entry : Installed)
        {
            *Entry.Slot = Entry.Original;
            Entry.Table->RemoveFromRoot();
        }
    }

    FP1DesignDataTestTables(const FP1DesignDataTestTables&) = delete;
    FP1DesignDataTestTables& operator=(const FP1DesignDataTestTables&) = delete;

private:
    void Install(TSoftObjectPtr<UDataTable>& Slot, UScriptStruct* RowStruct, const TCHAR* TableName)
    {
        FString Json;
        FFileHelper::LoadFileToString(Json, *FPaths::Combine(FPaths::ProjectDir(), TEXT("DesignData/Generated"), FString(TableName) + TEXT(".json")));

        // 에디터 메뉴와 같은 설정으로 채운다. 테스트 도중 GC가 돌아도 테이블이 남도록 루트에 둔다.
        UDataTable* Table = NewObject<UDataTable>(GetTransientPackage());
        Table->AddToRoot();
        Table->RowStruct = RowStruct;
        Table->ImportKeyField = TEXT("RowName");
        Table->CreateTableFromJSONString(Json);

        Installed.Add({ &Slot, Slot, Table });
        Slot = Table;
    }

    struct FInstalled
    {
        TSoftObjectPtr<UDataTable>* Slot = nullptr;
        TSoftObjectPtr<UDataTable> Original;
        UDataTable* Table = nullptr;
    };
    TArray<FInstalled> Installed;
};
