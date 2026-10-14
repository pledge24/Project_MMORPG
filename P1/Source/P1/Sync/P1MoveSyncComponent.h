#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Containers/Queue.h"
#include "Protocol.pb.h"
#include "Sync/P1MoveSyncMode.h"
#include "P1MoveSyncComponent.generated.h"

/**
 * 크리처 하나의 이동 동기화.
 *
 * 원격 크리처는 화면 위치를 서버 위치로 끌어당기고, 내 플레이어는 이동을 서버에 보낸다.
 * 크리처는 이 컴포넌트를 모른다.
 */
UCLASS()
class P1_API UP1MoveSyncComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UP1MoveSyncComponent();

    //~ Begin UActorComponent Interface
protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

    //~ Setup
public:
    /** Actor에 붙은 이동 동기화 컴포넌트. Actor가 nullptr이거나 붙어 있지 않으면 nullptr. */
    static UP1MoveSyncComponent* FindOn(const AActor* Actor);

    /** 붙인 직후, FinishSpawning 전에 부른다. */
    void SetMode(EP1MoveSyncMode InMode) { Mode = InMode; }

    /** 스폰할 때 서버가 보낸 첫 위치로 화면 위치와 서버 위치를 맞춘다. 액터는 옮기지 않는다. */
    void InitPos(const Protocol::PosInfo& Info);

    uint64 GetEntityId() const { return ClientPos.entity_id(); }

private:
    EP1MoveSyncMode Mode = EP1MoveSyncMode::RemotePlayer;

    //~ Server Position
public:
    /**
     * 서버가 보낸 위치를 다음 틱에 반영한다. 내 플레이어면 그 위치로 옮기고, 원격이면 보간 목표로 삼는다.
     * 게임 스레드 전용.
     */
    bool PushToMoveQueue(const Protocol::PosInfo& Info);

    /** 화면 위치를 그 위치로 즉시 옮긴다. */
    void SetClientPos(const Protocol::PosInfo& Info);

    /** 보간 목표와 이동 상태를 바꾼다. 화면 위치는 다음 틱부터 따라간다. */
    void SetServerPos(const Protocol::PosInfo& Info);

private:
    void SetMoveState(Protocol::MoveState State);

    /** 지금 화면에 보이는 위치. 이동 상태와 엔티티 id도 여기에 둔다. */
    Protocol::PosInfo ClientPos;

    /** 서버에서 받은 위치. 원격 크리처에만 쓴다. */
    Protocol::PosInfo ServerPos;

    TQueue<Protocol::PosInfo> MoveQueue;
    FVector MoveDirection = FVector::ZeroVector;

    //~ Remote Interpolation
private:
    void TickRemote(float DeltaSeconds);

    //~ My Player Send
private:
    void TickMyPlayer(float DeltaSeconds);

    /** 다음 주기 송신까지 남은 시간(초). */
    float MovePacketSendTimer;

    /** 직전 프레임의 이동 입력. 입력이 바뀌었는지 볼 때 쓴다. */
    FVector2D LastDesiredInput = FVector2D::ZeroVector;

    /** 서버와 마지막으로 맞춘 yaw. 내 플레이어 모드에서만 읽는다. */
    float LastSyncedYaw = 0.f;
};
