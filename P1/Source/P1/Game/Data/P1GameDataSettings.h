#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "P1GameDataSettings.generated.h"

class UDataTable;
class UTexture2D;
class UWorld;
struct FP1ItemConsumeEffectRow;
struct FP1ItemEquipStatRow;
struct FP1ItemRow;
struct FP1MonsterRow;

/**
 * 게임 코드가 읽는 기획 데이터 테이블의 설정.
 *
 * 어느 테이블을 쓸지 프로젝트 설정 「P1 Game Data」에서 고르게 하고, 게임 코드가 기획 데이터를 찾는 창구가 된다.
 * Find와 Load 함수는 모두 게임 스레드 전용이고, 테이블이나 행이 없으면 nullptr이다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "P1 Game Data"))
class P1_API UP1GameDataSettings : public UDeveloperSettings
{
    GENERATED_BODY()

    //~ Items
public:
    static const FP1ItemRow* FindItemRow(int32 ItemId);

    /** 장비가 아니면 nullptr. */
    static const FP1ItemEquipStatRow* FindItemEquipStatRow(int32 ItemId);

    /** 소모품이 아니면 nullptr. */
    static const FP1ItemConsumeEffectRow* FindItemConsumeEffectRow(int32 ItemId);

    /** 아이템 번호의 아이콘을 동기 로드한다. 아이콘이 없어도 nullptr. */
    static UTexture2D* LoadItemIcon(int32 ItemId);

    /** 모든 아이템의 공통 행. 행 구조체는 FP1ItemRow이고 행 이름은 아이템 번호. */
    UPROPERTY(Config, EditAnywhere, Category = "Items")
    TSoftObjectPtr<UDataTable> ItemTable;

    /** 장비의 착용 부위와 능력치. 행 구조체는 FP1ItemEquipStatRow이고 행 이름은 아이템 번호. */
    UPROPERTY(Config, EditAnywhere, Category = "Items")
    TSoftObjectPtr<UDataTable> ItemEquipStatTable;

    /** 소모품의 회복량과 재사용 대기. 행 구조체는 FP1ItemConsumeEffectRow이고 행 이름은 아이템 번호. */
    UPROPERTY(Config, EditAnywhere, Category = "Items")
    TSoftObjectPtr<UDataTable> ItemConsumeEffectTable;

    /** 아이템의 에셋 참조. 행 구조체는 FP1ItemAssetData이고 행 이름은 아이템 번호. */
    UPROPERTY(Config, EditAnywhere, Category = "Items")
    TSoftObjectPtr<UDataTable> ItemAssetTable;

    //~ Monsters
public:
    static const FP1MonsterRow* FindMonsterRow(int32 MonsterId);

    /** 몬스터 정의. 행 구조체는 FP1MonsterRow이고 행 이름은 몬스터 번호. */
    UPROPERTY(Config, EditAnywhere, Category = "Monsters")
    TSoftObjectPtr<UDataTable> MonsterTable;

    //~ Maps
public:
    /** MapId 맵의 레벨. 표에 없으면 null인 참조. */
    TSoftObjectPtr<UWorld> FindMapLevel(int32 MapId) const;

    /** 맵 번호(맵 표의 Id)마다 열 레벨. 서버가 알린 맵을 이 표로 연다. */
    UPROPERTY(Config, EditAnywhere, Category = "Maps")
    TMap<int32, TSoftObjectPtr<UWorld>> MapLevels;
};
