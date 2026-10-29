#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "P1NpcInteractionComponent.generated.h"

class AP1MyPlayer;
class AP1Npc;
class APlayerController;
class IP1ShopScreen;

/**
 * 내 플레이어의 NPC 상호작용.
 *
 * 화면 중앙 조준으로 사거리 안의 NPC에 초점을 맞추고, 초점이 맞은 NPC를 클릭하면 그 NPC의 상점을 연다.
 * 상점이 열린 동안 카메라를 NPC 쪽으로 당기고, 닫으면 원래 카메라로 되돌린다.
 * 인게임 플레이어 컨트롤러에 붙는다. 화면은 컨트롤러가 구현한 IP1ShopScreen으로 다룬다.
 */
UCLASS()
class P1_API UP1NpcInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UP1NpcInteractionComponent();

    //~ Begin UActorComponent Interface
public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    //~ End UActorComponent Interface

    //~ Focus
public:
    /** 비전투 모드의 왼쪽 클릭. 초점이 맞은 NPC가 있으면 그 NPC의 상점을 연다. */
    void Interact();

private:
    /** 카메라에서 화면 중앙으로 스윕해 초점을 다시 고른다. */
    void UpdateFocus();
    void SetFocus(AP1Npc* Npc);

    /** 조준 스윕의 구 반지름(cm). 조준점이 NPC의 가장자리에 걸쳐도 잡히는 정도다. */
    static constexpr float AIM_SWEEP_RADIUS = 20.f;

    TWeakObjectPtr<AP1Npc> FocusedNpc;

    //~ Shop
public:
    bool IsShopOpen() const { return ShopNpc.IsValid(); }

    /** 상점을 닫고 카메라를 되돌린다. 상점 창의 닫기 버튼과 메뉴 키가 부른다. 닫혀 있으면 아무것도 하지 않는다. */
    void CloseShop();

private:
    /** 상점을 연 NPC. 그 NPC가 사라지면(룸 이동, 시야 밖) 상점을 닫는다. */
    TWeakObjectPtr<AP1Npc> ShopNpc;

    //~ Camera
private:
    /** 상점이 열려 있으면 NPC 쪽으로, 닫히는 중이면 저장한 카메라 쪽으로 한 틱만큼 옮긴다. */
    void UpdateCamera(float DeltaTime);

    /** 상점을 여는 동안의 카메라 붐 길이(cm). */
    static constexpr float SHOP_ARM_LENGTH = 220.f;
    /** 카메라가 목표로 다가가는 보간 속도. 클수록 빠르다. */
    static constexpr float CAMERA_INTERP_SPEED = 6.f;

    /** 상점을 열기 직전의 카메라. 닫으면 이리로 돌아온다. */
    float SavedArmLength = 0.f;
    FRotator SavedControlRotation = FRotator::ZeroRotator;
    /** 상점을 닫은 뒤 저장한 카메라로 돌아가는 중이다. */
    bool bRestoringCamera = false;

    //~ 공통
private:
    APlayerController* GetPlayerController() const;
    /** 컨트롤러가 조종하는 내 플레이어. 없으면 nullptr. */
    AP1MyPlayer* GetMyPlayer() const;
    IP1ShopScreen* GetScreen() const;
};
