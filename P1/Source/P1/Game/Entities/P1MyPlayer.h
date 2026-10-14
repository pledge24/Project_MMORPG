#pragma once

#include "CoreMinimal.h"
#include "Game/Entities/P1Player.h"
#include "InputActionValue.h"
#include "P1MyPlayer.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

/**
 * 로컬 플레이어가 조종하는 캐릭터.
 *
 * 입력과 카메라를 맡고, 플레이어의 조작을 캐릭터의 이동과 공격으로 옮긴다.
 */
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
    //~ End AActor Interface

    //~ Begin APawn Interface
protected:
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    //~ End APawn Interface

    //~ Begin AP1Creature Interface
public:
    virtual void Initialize(const Protocol::EntityInfo& InEntityInfo) override;

    virtual bool IsMyPlayer() const override { return true; }

protected:
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
    /** 카메라를 캐릭터 뒤에 두는 스프링 암. */
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
public:
    /** 이동 입력을 캐릭터에 반영할 수 있으면 true. 공격 중이면 false. */
    bool CanInputMovement() const;

    /** 이동 입력의 원본 값. 입력이 없으면 0 벡터. */
    FVector2D GetDesiredInput() const { return DesiredInput; }

    /** 입력을 카메라 기준으로 바꾼 이동 방향의 단위 벡터. 입력이 없으면 0 벡터. */
    FVector GetDesiredMoveDirection() const { return DesiredMoveDirectionVec; }

    /** 이동 방향의 Yaw(도). */
    float GetDesiredMoveDirectionYaw() const { return DesiredMoveDirectionYaw; }

protected:
    void Move(const FInputActionValue& Value);

private:
    FVector2D DesiredInput = FVector2D::ZeroVector;
    FVector DesiredMoveDirectionVec = FVector::ZeroVector;
    float DesiredMoveDirectionYaw = 0.f;

    //~ Combat
protected:
    void NormalAttack(const FInputActionValue& Value);

    //~ Battle Mode
public:
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnBattleModeChanged, bool /*bBattleMode*/);
    FOnBattleModeChanged OnBattleModeChanged;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bBattleMode = false;

protected:
    void ToggleBattleMode(const FInputActionValue& Value);
};
