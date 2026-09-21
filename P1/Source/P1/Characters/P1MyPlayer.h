#pragma once

#include "CoreMinimal.h"
#include "Characters/P1Player.h"
#include "InputActionValue.h"
#include "Logging/LogMacros.h"
#include "P1MyPlayer.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UInventoryComponent;
class UEquippedGearComponent;

/** 로컬 플레이어가 조종하는 캐릭터다. 입력과 카메라를 이 클래스가 갖는다. */
UCLASS()
class P1_API AP1MyPlayer : public AP1Player
{
    GENERATED_BODY()

public:
    AP1MyPlayer();

    //~ Begin AActor Interface
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;
    //~ End AActor Interface

    //~ Begin APawn Interface
protected:
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    //~ End APawn Interface

    //~ Begin AP1Creature Interface
public:
    /** 서버가 보낸 엔티티 정보로 초기화한다. 서버가 보낸 값으로만 부른다. */
    virtual void Initialize(const Protocol::EntityInfo& InEntityInfo) override;

protected:
    /** 내 캐릭터는 서버가 보낸 위치를 되받지 않는다. 입력이 곧 위치다. */
    virtual void S_Move(float DeltaSeconds) override final {};

    /** 내 캐릭터의 공격은 입력이 시작한다. 서버 통지로 다시 재생하지 않는다. */
    virtual void S_NormalAttack(uint32 Combo, float Yaw) override final {};
    //~ End AP1Creature Interface

    //~ Camera
public:
    FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
    void Look(const FInputActionValue& Value);

private:
    /** 카메라를 캐릭터 뒤에 두는 스프링 암이다. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCameraComponent> FollowCamera;

    //~ Input
private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UInputAction> NormalAttackAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UInputAction> ToggleBattleModeAction;

    //~ Movement
protected:
    void Move(const FInputActionValue& Value);

    bool CanInputMovement() const;

private:
    Protocol::C_MOVE MovePkt;

    /**
     * 초 단위 이동 패킷 전송 주기다.
     * MovePacketSendTimer가 이 값으로 초기화되므로 그보다 먼저 선언한다.
     */
    const float MOVE_PACKET_SEND_DELAY = 0.2f;
    const float YAW_TOLERANCE = 60.f;
    float MovePacketSendTimer = MOVE_PACKET_SEND_DELAY;

    FVector2D DesiredInput;
    FVector DesiredMoveDirectionVec;    // 이동할 방향(단위 벡터)
    float DesiredMoveDirectionYaw;      // 이동할 방향(Yaw)

    /** 직전 프레임의 입력이다. 값이 바뀌었을 때만 패킷을 보낸다. */
    FVector2D LastDesiredInput;

    //~ Movement Debug
private:
    /** 켜면 평균 전송 속도를 로그로 남긴다. */
    const bool Activate = false;
    int32 SendCounter = 1;
    float TotalSecond = 0.2f;

    //~ Combat
protected:
    void NormalAttack(const FInputActionValue& Value);

    //~ Battle Mode
public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bBattleMode = false;

protected:
    void ToggleBattleMode(const FInputActionValue& Value);
};
