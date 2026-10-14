#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "P1LoginMenuPlayerController.generated.h"

/**
 * 로그인 맵의 플레이어 컨트롤러.
 *
 * 로그인 맵에서 로그인 화면을 띄우고 소유한다.
 */
UCLASS()
class P1_API AP1LoginMenuPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AP1LoginMenuPlayerController();

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    //~ End AActor Interface

    //~ Login Menu UI
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UP1LoginMenuWidget> LoginMenuWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UI")
    TObjectPtr<UP1LoginMenuWidget> LoginMenuWidget;
};
