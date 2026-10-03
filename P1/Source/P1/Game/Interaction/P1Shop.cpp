#include "Game/Interaction/P1Shop.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Game/Entities/P1MyPlayer.h"
#include "Game/Interaction/P1ShopScreen.h"
#include "Utils/LogCategory.h"

const FName AP1Shop::RangeComponentName(TEXT("ShopCollision"));

void AP1Shop::BeginPlay()
{
    Super::BeginPlay();

    TInlineComponentArray<UPrimitiveComponent*> Components(this);
    UPrimitiveComponent* const* Found = Components.FindByPredicate(
        [](const UPrimitiveComponent* Component) { return Component->GetFName() == RangeComponentName; });
    if (Found == nullptr)
    {
        UE_LOG(LogP1CharacterInteraction, Warning, TEXT("상점 %s에 %s 컴포넌트가 없다"), *GetName(), *RangeComponentName.ToString());
        return;
    }

    (*Found)->OnComponentBeginOverlap.AddUniqueDynamic(this, &AP1Shop::HandleRangeBeginOverlap);
    (*Found)->OnComponentEndOverlap.AddUniqueDynamic(this, &AP1Shop::HandleRangeEndOverlap);
}

void AP1Shop::HandleRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    const AP1MyPlayer* MyPlayer = Cast<AP1MyPlayer>(OtherActor);
    IP1ShopScreen* ShopScreen = FindShopScreen();
    if (MyPlayer == nullptr || ShopScreen == nullptr)
        return;

    // 전투 모드에서는 상점 창을 열지 않는다. 전투 모드를 끄고 다시 들어와야 열린다.
    if (MyPlayer->bBattleMode)
        ShopScreen->ShowShopWarning(FText::FromString(TEXT("Tab을 눌러 전투모드를 비활성화해주세요")));
    else
        ShopScreen->OpenShopWindow();
}

void AP1Shop::HandleRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    // 블루프린트도 나간 액터를 보지 않았다. 다른 액터가 나가도 닫히는 결함은 #170이 고친다.
    if (IP1ShopScreen* ShopScreen = FindShopScreen())
        ShopScreen->CloseShopWindow();
}

IP1ShopScreen* AP1Shop::FindShopScreen() const
{
    UWorld* World = GetWorld();
    return World ? Cast<IP1ShopScreen>(World->GetFirstPlayerController()) : nullptr;
}
