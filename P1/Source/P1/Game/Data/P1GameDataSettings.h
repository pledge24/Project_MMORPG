#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "P1GameDataSettings.generated.h"

class UDataTable;
struct FP1ItemData;

/**
 * 게임 코드가 읽는 기획 데이터 테이블이다. 기본값은 Config/DefaultGame.ini에 있고,
 * 에디터의 프로젝트 설정 「P1 Game Data」에서 바꿀 수 있다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "P1 Game Data"))
class P1_API UP1GameDataSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** 아이템 정의다. 행 구조체는 FP1ItemData이고 행 이름은 템플릿 id다. */
    UPROPERTY(Config, EditAnywhere, Category = "Items")
    TSoftObjectPtr<UDataTable> ItemTable;

    /**
     * 템플릿 id의 아이템 정의를 찾는다. 테이블이나 행이 없으면 nullptr을 돌려준다.
     * 테이블이 아직 로드되지 않았으면 동기 로드한다. 게임 스레드 전용.
     */
    static const FP1ItemData* FindItemData(int32 TemplateId);
};
