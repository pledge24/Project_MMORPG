#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "P1GameDataSettings.generated.h"

class UDataTable;
class UTexture2D;
struct FP1ItemData;

/**
 * 게임 코드가 읽는 기획 데이터 테이블의 설정.
 *
 * 어느 테이블을 쓸지 프로젝트 설정 「P1 Game Data」에서 고르게 하고, 게임 코드가 기획 데이터를 찾는 창구가 된다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "P1 Game Data"))
class P1_API UP1GameDataSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** 아이템 정의. 행 구조체는 FP1ItemData이고 행 이름은 템플릿 id. */
    UPROPERTY(Config, EditAnywhere, Category = "Items")
    TSoftObjectPtr<UDataTable> ItemTable;

    /**
     * 템플릿 id의 아이템 정의를 찾는다. 테이블이나 행이 없으면 nullptr.
     * 테이블이 로드되지 않았으면 동기 로드한다. 게임 스레드 전용.
     */
    static const FP1ItemData* FindItemData(int32 TemplateId);

    /** 아이템의 에셋 참조. 행 구조체는 FP1ItemAssetData이고 행 이름은 템플릿 id. */
    UPROPERTY(Config, EditAnywhere, Category = "Items")
    TSoftObjectPtr<UDataTable> ItemAssetTable;

    /**
     * 템플릿 id의 아이템 아이콘을 동기 로드한다. 테이블이나 행이나 아이콘이 없으면 nullptr.
     * 게임 스레드 전용.
     */
    static UTexture2D* LoadItemIcon(int32 TemplateId);
};
