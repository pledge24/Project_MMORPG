#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Containers/Queue.h"
#include "Protocol.pb.h"
#include "Sync/P1MoveSyncMode.h"
#include "P1MoveSyncComponent.generated.h"

/**
 * 크리처 하나의 이동 동기화를 맡는다. 원격 크리처의 수신 보간과 내 플레이어의 이동 패킷 송신이 여기 있다.
 * 스포너가 스폰할 때 붙인다. 크리처는 이 컴포넌트를 모른다.
 */
UCLASS()
class P1_API UP1MoveSyncComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UP1MoveSyncComponent();

    //~ Begin UActorComponent Interface
protected:
    /** 소유 액터의 틱 뒤에, 캐릭터 이동 컴포넌트의 틱 앞에 돌도록 선행 조건을 건다. */
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

    //~ Setup
public:
    /** 없으면 nullptr을 돌려준다. */
    static UP1MoveSyncComponent* FindOn(const AActor* Actor);

    /** 스포너가 붙인 직후, FinishSpawning 전에 부른다. */
    void SetMode(EP1MoveSyncMode InMode) { Mode = InMode; }

    /** 서버가 보낸 첫 위치로 화면 위치와 서버 위치를 함께 맞춘다. 액터는 옮기지 않는다. 스폰할 때 부른다. */
    void InitPos(const Protocol::PosInfo& Info);

    uint64 GetEntityId() const { return ClientPos.entity_id(); }

private:
    EP1MoveSyncMode Mode = EP1MoveSyncMode::RemotePlayer;

    //~ Server Position
public:
    /** 다음 틱에 반영한다. 내 플레이어면 그 위치로 옮기고, 원격이면 보간 목표로 삼는다. 게임 스레드 전용. */
    bool PushToMoveQueue(const Protocol::PosInfo& Info);

    /** 화면 위치를 그 위치로 즉시 옮긴다. */
    void SetClientPos(const Protocol::PosInfo& Info);

    /** 보간 목표와 이동 상태를 바꾼다. 화면 위치는 다음 틱부터 따라간다. */
    void SetServerPos(const Protocol::PosInfo& Info);

private:
    void SetMoveState(Protocol::MoveState State);

    /** 지금 화면에 보이는 위치다. 틱마다 액터 위치로 갱신한다. */
    Protocol::PosInfo ClientPos;

    /** 서버에서 받은 위치다. 원격 크리처에만 쓴다. */
    Protocol::PosInfo ServerPos;

    TQueue<Protocol::PosInfo> MoveQueue;
    FVector MoveDirection = FVector::ZeroVector;

    //~ Remote Interpolation
private:
    void TickRemote(float DeltaSeconds);

    //~ My Player Send
private:
    void TickMyPlayer(float DeltaSeconds);

    /** 초 단위로 다음 주기 송신까지 남은 시간이다. 생성자가 전송 주기로 맞춘다. */
    float MovePacketSendTimer;

    /** 직전 프레임의 입력이다. 값이 바뀌었는지 볼 때 쓴다. */
    FVector2D LastDesiredInput = FVector2D::ZeroVector;
};
