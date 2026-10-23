#pragma once

#include "CoreMinimal.h"
#include "Game/Entities/P1Creature.h"
#include "P1Player.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UP1GearAppearanceComponent;
class UWidgetComponent;
class IP1ChatBubbleDisplay;

/**
 * 월드에 보이는 플레이어 캐릭터.
 *
 * 내 플레이어와 다른 플레이어가 함께 쓰는 기반으로, 장착한 장비의 외형과 방금 한 말의 말풍선을 보여 준다.
 */
UCLASS()
class P1_API AP1Player : public AP1Creature
{
    GENERATED_BODY()

public:
    AP1Player();

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PostInitializeComponents() override;
    //~ End AActor Interface

    //~ Begin AP1Creature Interface
public:
    virtual void Initialize(const Protocol::EntityInfo& EntityInfo) override;
    //~ End AP1Creature Interface

    //~ Equipment
public:
    /** 장비 부위(GearPart)에 아이템 외형을 입힌다. TemplateId가 0이면 그 부위를 비운다. */
    void ApplyGear(int32 GearType, int32 TemplateId);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> WeaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> HelmetMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> ChestMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> LegsMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> ArmsMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> BootsMesh;

    /** 부위별 메시 컴포넌트를 장착한 아이템의 메시로 바꾼다. 에셋 테이블은 여기서 지정한다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UP1GearAppearanceComponent> GearAppearance;

    //~ Identity
public:
    void SetPlayerName(const FText& InName);
    FText GetPlayerName() const { return GetCreatureName(); }

    //~ Chat Bubble
public:
    /** 머리 위에 말풍선을 띄운다. 이미 떠 있으면 새 말로 바꾸고 표시 시간을 다시 센다. */
    void ShowChatBubble(const FString& Message);

    /** 말한 플레이어와 보는 플레이어가 MaxDistance 안에 있으면 true. 거리가 MaxDistance와 같아도 안이다. */
    static bool IsWithinChatBubbleRange(const FVector& SpeakerLocation, const FVector& ViewerLocation, float MaxDistance);

    /** 말풍선이 떠 있는 시간(초). */
    static constexpr float CHAT_BUBBLE_SECONDS = 5.f;

private:
    void HideChatBubble();

    /** 말풍선이 떠 있는 동안 보는 플레이어와의 거리로 보일지를 다시 정한다. */
    void RefreshChatBubbleVisibility();

    /** 말풍선 컴포넌트의 위젯. 위젯이 없거나 말풍선 위젯이 아니면 nullptr. */
    IP1ChatBubbleDisplay* GetChatBubble() const;

    /** 같은 룸이어도 보는 플레이어와 이 거리(언리얼 단위)보다 멀면 말풍선을 숨긴다. */
    UPROPERTY(EditDefaultsOnly, Category = "Chat Bubble", meta = (ClampMin = "0"))
    float ChatBubbleVisibleDistance = 2000.f;

    /** 말풍선 위젯을 붙이는 컴포넌트. 위젯 클래스와 높이는 블루프린트에서 네임플레이트 위로 맞춘다. */
    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UWidgetComponent> ChatBubbleComponent;

    /** 말풍선이 떠 있는 동안 거리를 다시 보는 간격(초). */
    static constexpr float CHAT_BUBBLE_RANGE_CHECK_SECONDS = 0.2f;

    /** 표시 시간이 남아 있으면 true. 거리 때문에 숨겨져 있어도 true다. */
    bool bChatBubbleActive = false;

    FTimerHandle ChatBubbleHideTimer;
    FTimerHandle ChatBubbleRangeTimer;
};
