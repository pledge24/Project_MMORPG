#include "CreatureCombat/P1CreatureCombatExtractCommandlet.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CreatureCombat/P1CreatureCombatCheck.h"
#include "CreatureCombat/P1MontageExtract.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/DataTable.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Game/Combat/P1AttackSystemComponent.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/P1MonsterAssetData.h"
#include "GameFramework/Character.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Utils/LogCategory.h"

namespace
{
    /** 서버가 읽는 추출 산출물 폴더와 파일. 서버의 Gamedata가 읽는 폴더(Game/Data/Extracted/)와 CREATURE_COMBAT_FILE과 같아야 한다. */
    FString GetOutputPath()
    {
        return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../Server/GameServer/Game/Data/Extracted/CreatureCombat.json"));
    }

    void AddAttacks(const TArray<UAnimMontage*>& Montages, const FTransform& MeshToActor, FP1CreatureCombatCheck::FCreature& OutCreature)
    {
        for (const UAnimMontage* Montage : Montages)
        {
            if (Montage == nullptr)
                OutCreature.Attacks.AddDefaulted();
            else
                OutCreature.Attacks.Add(P1MontageExtract::Extract(*Montage, MeshToActor));
        }
    }

    /**
     * 몬스터 에셋 테이블의 행마다 몬스터 하나. 메시의 상대 회전은 몬스터 블루프린트가 정하지만, 몬스터 몽타주는 루트 모션을 쓸 수 없으므로
     * 곡선을 만들지 않아 회전이 필요 없다.
     */
    void CollectMonsters(const UP1GameDataSettings& Settings, TArray<FP1CreatureCombatCheck::FCreature>& OutMonsters, TArray<FString>& OutErrors)
    {
        const UDataTable* Table = Settings.MonsterAssetTable.LoadSynchronous();
        if (Table == nullptr)
        {
            OutErrors.Add(TEXT("몬스터 에셋 테이블을 열지 못했다. 프로젝트 설정 「P1 Game Data」의 MonsterAssetTable을 본다"));
            return;
        }

        for (const TPair<FName, uint8*>& Row : Table->GetRowMap())
        {
            const FString RowName = Row.Key.ToString();
            if (RowName.IsNumeric() == false)
            {
                OutErrors.Add(FString::Printf(TEXT("%s 행 %s: 행 이름이 몬스터 번호가 아니다"), *Table->GetName(), *RowName));
                continue;
            }

            const FP1MonsterAssetData& AssetData = *reinterpret_cast<const FP1MonsterAssetData*>(Row.Value);
            FP1CreatureCombatCheck::FCreature& Monster = OutMonsters.AddDefaulted_GetRef();
            Monster.Id = FCString::Atoi(*RowName);
            Monster.Source = FString::Printf(TEXT("%s 행 %s"), *Table->GetName(), *RowName);
            Monster.CapsuleRadius = AssetData.CapsuleRadius;
            Monster.CapsuleHalfHeight = AssetData.CapsuleHalfHeight;

            TArray<UAnimMontage*> Montages;
            for (const TSoftObjectPtr<UAnimMontage>& Montage : AssetData.AttackMontages)
                Montages.Add(Montage.LoadSynchronous());
            AddAttacks(Montages, FTransform::Identity, Monster);
        }
    }

    /**
     * 블루프린트가 더한 공격 시스템 컴포넌트의 템플릿. 블루프린트 컴포넌트는 CDO에 없고 구성 스크립트에 있다.
     * 자식 블루프린트가 덮어쓴 템플릿을 먼저 본다. 없으면 nullptr.
     */
    const UP1AttackSystemComponent* FindAttackSystemTemplate(UClass* PlayerClass)
    {
        UBlueprintGeneratedClass* MostDerived = Cast<UBlueprintGeneratedClass>(PlayerClass);
        for (UClass* Class = PlayerClass; Class != nullptr; Class = Class->GetSuperClass())
        {
            const UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(Class);
            if (BlueprintClass == nullptr || BlueprintClass->SimpleConstructionScript == nullptr)
                continue;

            for (USCS_Node* Node : BlueprintClass->SimpleConstructionScript->GetAllNodes())
            {
                if (Node == nullptr || Cast<UP1AttackSystemComponent>(Node->ComponentTemplate) == nullptr)
                    continue;

                if (MostDerived != nullptr && MostDerived != BlueprintClass && MostDerived->InheritableComponentHandler != nullptr)
                {
                    if (const UP1AttackSystemComponent* Override = Cast<UP1AttackSystemComponent>(
                        MostDerived->InheritableComponentHandler->GetOverridenComponentTemplate(FComponentKey(Node))))
                        return Override;
                }
                return Cast<UP1AttackSystemComponent>(Node->ComponentTemplate);
            }
        }

        // 네이티브로 붙인 컴포넌트는 CDO에 있다.
        const AActor* Default = PlayerClass->GetDefaultObject<AActor>();
        return Default != nullptr ? Default->FindComponentByClass<UP1AttackSystemComponent>() : nullptr;
    }

    /** 직업마다 플레이어 블루프린트 하나. 루트 모션은 블루프린트의 메시 상대 트랜스폼으로 캐릭터 로컬에 옮긴다. */
    void CollectClasses(const UP1GameDataSettings& Settings, TArray<FP1CreatureCombatCheck::FCreature>& OutClasses, TArray<FString>& OutErrors)
    {
        for (const TPair<int32, TSoftClassPtr<ACharacter>>& Entry : Settings.CharacterClassPlayers)
        {
            UClass* PlayerClass = Entry.Value.LoadSynchronous();
            const ACharacter* Default = PlayerClass != nullptr ? PlayerClass->GetDefaultObject<ACharacter>() : nullptr;
            if (Default == nullptr)
            {
                OutErrors.Add(FString::Printf(TEXT("직업 %d: 플레이어 블루프린트 %s를 열지 못했다"), Entry.Key, *Entry.Value.ToString()));
                continue;
            }

            FP1CreatureCombatCheck::FCreature& Class = OutClasses.AddDefaulted_GetRef();
            Class.Id = Entry.Key;
            Class.Source = PlayerClass->GetName();
            Class.CapsuleRadius = Default->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
            Class.CapsuleHalfHeight = Default->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

            const UP1AttackSystemComponent* AttackSystem = FindAttackSystemTemplate(PlayerClass);
            if (AttackSystem == nullptr)
                continue; // 공격 몽타주가 없다는 오류를 검사가 낸다.

            TArray<UAnimMontage*> Montages;
            for (const TObjectPtr<UAnimMontage>& Montage : AttackSystem->GetNormalAttackMontages())
                Montages.Add(Montage.Get());
            AddAttacks(Montages, Default->GetMesh()->GetRelativeTransform(), Class);
        }
    }
}

UP1CreatureCombatExtractCommandlet::UP1CreatureCombatExtractCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UP1CreatureCombatExtractCommandlet::Main(const FString& Params)
{
    TArray<FString> Errors;
    FP1CreatureCombatCheck::FInput Input;

    // 기획 데이터의 몬스터 번호. 몬스터 에셋 테이블의 행이 이 번호와 짝을 맺는다.
    const FString MonsterPath = FPaths::ProjectDir() / TEXT("DesignData/Generated/Monster.json");
    FString MonsterJson;
    if (FFileHelper::LoadFileToString(MonsterJson, *MonsterPath) == false)
        Errors.Add(FString::Printf(TEXT("%s를 읽지 못했다. DataBuilder를 돌린다"), *MonsterPath));
    else
        FP1CreatureCombatCheck::ParseMonsterIds(MonsterJson, Input.DesignMonsterIds, Errors);

    const UP1GameDataSettings& Settings = *GetDefault<UP1GameDataSettings>();
    CollectMonsters(Settings, Input.Monsters, Errors);
    CollectClasses(Settings, Input.Classes, Errors);

    FP1CreatureCombatCheck::FResult Result;
    if (Errors.IsEmpty())
    {
        Result = FP1CreatureCombatCheck::Run(Input);
        Errors = Result.Errors;
    }

    if (Errors.IsEmpty() == false)
    {
        for (const FString& Error : Errors)
            UE_LOG(LogP1CreatureCombat, Error, TEXT("[크리처 추출] %s"), *Error);
        UE_LOG(LogP1CreatureCombat, Error, TEXT("[크리처 추출] 오류 %d건. 산출물을 바꾸지 않았다"), Errors.Num());
        return 1;
    }

    const FString OutputPath = GetOutputPath();
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath), true);
    if (FFileHelper::SaveStringToFile(Result.File, *OutputPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) == false)
    {
        UE_LOG(LogP1CreatureCombat, Error, TEXT("[크리처 추출] %s를 쓰지 못했다"), *OutputPath);
        return 1;
    }

    UE_LOG(LogP1CreatureCombat, Display, TEXT("[크리처 추출] %s"), *OutputPath);
    UE_LOG(LogP1CreatureCombat, Display, TEXT("[크리처 추출] 몬스터 %d개와 직업 %d개의 판정 데이터를 썼다"), Input.Monsters.Num(), Input.Classes.Num());
    return 0;
}
