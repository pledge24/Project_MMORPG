#include "CreatureCombat/P1CreatureCombatExtractCommandlet.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CreatureCombat/P1CreatureCombatCheck.h"
#include "CreatureCombat/P1MontageExtract.h"
#include "Engine/DataTable.h"
#include "Game/Data/P1ActionAssetData.h"
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

    /** DataBuilder의 서버 산출물 폴더. 다음 액션과 몬스터의 일반 공격 액션은 서버만 받는 열이라 클라이언트 산출물에 없다. */
    FString GetServerDesignDataDir()
    {
        return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../Server/GameServer/Game/Data/Generated/"));
    }

    /** 몬스터 에셋 테이블의 행마다 몬스터 하나. */
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
        }
    }

    /**
     * 직업마다 플레이어 블루프린트 하나. 블루프린트의 메시 상대 트랜스폼(위치 제외)을 OutMeshToActor에 쓴다.
     * 액션은 직업을 모르므로 모든 직업의 메시가 같은 회전과 크기로 붙어 있어야 한다.
     */
    void CollectClasses(const UP1GameDataSettings& Settings, TArray<FP1CreatureCombatCheck::FCreature>& OutClasses, FTransform& OutMeshToActor,
        TArray<FString>& OutErrors)
    {
        TOptional<FTransform> MeshToActor;
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

            // 루트 모션은 이동량이라 메시의 위치는 쓰지 않는다.
            const FTransform& Relative = Default->GetMesh()->GetRelativeTransform();
            const FTransform ClassMeshToActor(Relative.GetRotation(), FVector::ZeroVector, Relative.GetScale3D());
            if (MeshToActor.IsSet() && MeshToActor->Equals(ClassMeshToActor) == false)
                OutErrors.Add(FString::Printf(TEXT("직업 %d(%s): 메시의 상대 회전이나 크기가 다른 직업과 다르다. 액션의 루트 모션을 캐릭터 로컬로 옮길 수 없다"),
                    Class.Id, *Class.Source));
            else
                MeshToActor = ClassMeshToActor;
        }
        OutMeshToActor = MeshToActor.Get(FTransform::Identity);
    }

    /** 액션 에셋 테이블의 행마다 액션 하나. 루트 모션은 플레이어의 메시 트랜스폼으로 옮긴다. 몬스터의 액션은 루트 모션이 없다(검사가 막는다). */
    void CollectActions(const UP1GameDataSettings& Settings, const FTransform& PlayerMeshToActor, TArray<FP1CreatureCombatCheck::FAction>& OutActions,
        TArray<FString>& OutErrors)
    {
        const UDataTable* Table = Settings.ActionAssetTable.LoadSynchronous();
        if (Table == nullptr)
        {
            OutErrors.Add(TEXT("액션 에셋 테이블을 열지 못했다. 프로젝트 설정 「P1 Game Data」의 ActionAssetTable을 본다"));
            return;
        }

        for (const TPair<FName, uint8*>& Row : Table->GetRowMap())
        {
            const FString RowName = Row.Key.ToString();
            const FString Source = FString::Printf(TEXT("%s 행 %s"), *Table->GetName(), *RowName);
            if (RowName.IsNumeric() == false)
            {
                OutErrors.Add(FString::Printf(TEXT("%s: 행 이름이 액션 번호가 아니다"), *Source));
                continue;
            }

            const FP1ActionAssetData& AssetData = *reinterpret_cast<const FP1ActionAssetData*>(Row.Value);
            FP1CreatureCombatCheck::FAction Action;
            const UAnimMontage* Montage = AssetData.Montage.LoadSynchronous();
            if (Montage != nullptr && P1MontageExtract::ExtractSection(*Montage, AssetData.Section, PlayerMeshToActor, Action) == false)
            {
                OutErrors.Add(FString::Printf(TEXT("%s: 몽타주 %s에 섹션 %s가 없다"), *Source, *Montage->GetPathName(), *AssetData.Section.ToString()));
                continue;
            }

            // 몽타주 칸이 비었으면 Montage가 빈 채로 넘겨 검사가 알린다.
            Action.Id = FCString::Atoi(*RowName);
            Action.Source = Source;
            OutActions.Add(MoveTemp(Action));
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

    // 기획 데이터의 몬스터와 액션. 몬스터 에셋 테이블과 액션 에셋 테이블의 행이 이 번호와 짝을 맺는다.
    const FString DesignDataDir = GetServerDesignDataDir();
    FString ActionJson;
    FString MonsterJson;
    if (FFileHelper::LoadFileToString(ActionJson, *(DesignDataDir / TEXT("Action.json"))) == false
        || FFileHelper::LoadFileToString(MonsterJson, *(DesignDataDir / TEXT("Monster.json"))) == false)
        Errors.Add(FString::Printf(TEXT("%s에서 Action.json과 Monster.json을 읽지 못했다. DataBuilder를 돌린다"), *DesignDataDir));
    else
        FP1CreatureCombatCheck::ParseDesignData(ActionJson, MonsterJson, Input, Errors);

    const UP1GameDataSettings& Settings = *GetDefault<UP1GameDataSettings>();
    FTransform PlayerMeshToActor;
    CollectMonsters(Settings, Input.Monsters, Errors);
    CollectClasses(Settings, Input.Classes, PlayerMeshToActor, Errors);
    CollectActions(Settings, PlayerMeshToActor, Input.Actions, Errors);

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
    UE_LOG(LogP1CreatureCombat, Display, TEXT("[크리처 추출] 몬스터 %d개와 직업 %d개, 액션 %d개의 판정 데이터를 썼다"),
        Input.Monsters.Num(), Input.Classes.Num(), Input.Actions.Num());
    return 0;
}
