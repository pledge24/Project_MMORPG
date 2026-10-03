#include "Sync/P1MoveSyncComponent.h"
#include "Sync/P1MoveCorrection.h"
#include "Sync/P1MoveSendThrottle.h"
#include "Sync/P1MoveSyncConstants.h"
#include "Game/Entities/P1MyPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Network/P1PacketSender.h"

UP1MoveSyncComponent::UP1MoveSyncComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    // 캐릭터 이동 컴포넌트가 이 컴포넌트를 선행 조건으로 기다린다. 기본 그룹(DuringPhysics)이면
    // 엔진이 캐릭터 이동 컴포넌트까지 그 그룹으로 미루므로, 캐릭터 이동 컴포넌트와 같은 PrePhysics에 둔다.
    PrimaryComponentTick.TickGroup = TG_PrePhysics;

    MovePacketSendTimer = P1MoveSync::MOVE_PACKET_SEND_DELAY;
}

void UP1MoveSyncComponent::BeginPlay()
{
    Super::BeginPlay();

    // 액터 Tick 뒤에 돌고, 여기서 넣은 이동 입력과 보정을 캐릭터 이동 컴포넌트가 같은 프레임에 이어받아야 한다.
    if (ACharacter* Character = GetOwner<ACharacter>())
    {
        AddTickPrerequisiteActor(Character);

        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
            Movement->AddTickPrerequisiteComponent(this);
    }
}

void UP1MoveSyncComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AActor* Owner = GetOwner();
    if (IsValid(Owner) == false)
        return;

    // 틱마다 지금 화면 위치를 캐시한다
    {
        const FVector Location = Owner->GetActorLocation();
        ClientPos.mutable_pos()->set_x(Location.X);
        ClientPos.mutable_pos()->set_y(Location.Y);
        ClientPos.mutable_pos()->set_z(Location.Z);
        ClientPos.set_yaw(Owner->GetActorRotation().Yaw);
    }

    // 내 플레이어는 받은 위치로 바로 옮기고, 원격 크리처는 보간 목표로 삼는다
    Protocol::PosInfo Info;
    while (MoveQueue.Dequeue(Info))
    {
        if (Mode == EP1MoveSyncMode::MyPlayer)
            SetClientPos(Info);
        else
            SetServerPos(Info);
    }

    if (Mode == EP1MoveSyncMode::MyPlayer)
        TickMyPlayer(DeltaTime);
    else
        TickRemote(DeltaTime);
}

UP1MoveSyncComponent* UP1MoveSyncComponent::FindOn(const AActor* Actor)
{
    return Actor ? Actor->FindComponentByClass<UP1MoveSyncComponent>() : nullptr;
}

void UP1MoveSyncComponent::InitPos(const Protocol::PosInfo& Info)
{
    ClientPos.CopyFrom(Info);
    ServerPos.CopyFrom(Info);
    LastSentYaw = Info.yaw();
}

bool UP1MoveSyncComponent::PushToMoveQueue(const Protocol::PosInfo& Info)
{
    return MoveQueue.Enqueue(Info);
}

void UP1MoveSyncComponent::SetClientPos(const Protocol::PosInfo& Info)
{
    if (ClientPos.entity_id() != 0)
    {
        ensureMsgf(ClientPos.entity_id() == Info.entity_id(), TEXT("다른 엔티티의 위치로 덮어쓴다. 기존 %lld, 새 %lld"),
            ClientPos.entity_id(), Info.entity_id());
    }

    ClientPos.CopyFrom(Info);
    LastSentYaw = Info.yaw();

    AActor* Owner = GetOwner();
    if (Owner == nullptr)
        return;

    FVector Location(Info.pos().x(), Info.pos().y(), Info.pos().z());
    Owner->SetActorLocation(Location);

    FRotator CurrentRotation = Owner->GetActorRotation();
    FRotator NewRotation = FRotator(CurrentRotation.Pitch, Info.yaw(), CurrentRotation.Roll);
    Owner->SetActorRotation(NewRotation);
}

void UP1MoveSyncComponent::SetServerPos(const Protocol::PosInfo& Info)
{
    if (ClientPos.entity_id() != 0)
    {
        ensureMsgf(ClientPos.entity_id() == Info.entity_id(), TEXT("다른 엔티티의 서버 위치를 받았다. 기존 %lld, 새 %lld"),
            ClientPos.entity_id(), Info.entity_id());
    }

    ServerPos.CopyFrom(Info);
    MoveDirection = FVector(ServerPos.move_direction().x(), ServerPos.move_direction().y(), 0.f);
    SetMoveState(Info.state()); // state는 ClientPos에 바로 세팅
}

void UP1MoveSyncComponent::SetMoveState(Protocol::MoveState State)
{
    if (ClientPos.state() == State)
        return;

    ClientPos.set_state(State);
}

void UP1MoveSyncComponent::TickRemote(float DeltaSeconds)
{
    ACharacter* Character = GetOwner<ACharacter>();
    if (Character == nullptr)
        return;

    if (ServerPos.state() == Protocol::MOVE_STATE_RUN)
    {
        Character->AddMovementInput(MoveDirection);
    }
    else if (ServerPos.state() == Protocol::MOVE_STATE_IDLE)
    {
        if (Character->GetVelocity().Size() > 0.f)
        {
            Character->GetCharacterMovement()->StopMovementImmediately();
        }
    }

    const bool bIsMonster = Mode == EP1MoveSyncMode::RemoteMonster;
    const bool bIsIdlePlayer = ServerPos.state() == Protocol::MOVE_STATE_IDLE && Mode == EP1MoveSyncMode::RemotePlayer;
    const bool bInAction = ServerPos.state() == Protocol::MOVE_STATE_ACTION;

    const FP1MoveCorrection Correction = FP1MoveCorrection::Compute(
        Character->GetActorLocation(),
        Character->GetActorRotation(),
        FVector(ServerPos.pos().x(), ServerPos.pos().y(), ServerPos.pos().z()),
        ServerPos.yaw(),
        MoveDirection,
        bIsMonster || bIsIdlePlayer,
        bInAction,
        DeltaSeconds);

    // 회전이 그대로면 SetActorRotation을 부르지 않는다.
    if (Correction.Rotation != Character->GetActorRotation())
        Character->SetActorRotation(Correction.Rotation);

    Character->SetActorLocation(Correction.Location);
}

void UP1MoveSyncComponent::TickMyPlayer(float DeltaSeconds)
{
    AP1MyPlayer* MyPlayer = GetOwner<AP1MyPlayer>();
    if (MyPlayer == nullptr)
        return;

    const bool bCanInputMovement = MyPlayer->CanInputMovement();
    const FVector2D DesiredInput = MyPlayer->GetDesiredInput();

    const bool bInputChanged = LastDesiredInput != DesiredInput;
    LastDesiredInput = DesiredInput;

    const bool bAttacking = MyPlayer->IsAttacking();

    // 이동 상태 판정
    if (bAttacking)
        SetMoveState(Protocol::MOVE_STATE_ACTION);
    else if (bCanInputMovement && DesiredInput != FVector2D::Zero())
        SetMoveState(Protocol::MOVE_STATE_RUN);
    else
        SetMoveState(Protocol::MOVE_STATE_IDLE);

    FP1MoveSendThrottle::FInput SendInput;
    SendInput.RemainingTimer = MovePacketSendTimer;
    SendInput.DeltaSeconds = DeltaSeconds;
    SendInput.bInputChanged = bInputChanged;
    SendInput.bCanInputMovement = bCanInputMovement;
    SendInput.bHasMoveInput = DesiredInput != FVector2D::Zero();
    SendInput.DesiredYaw = MyPlayer->GetDesiredMoveDirectionYaw();
    SendInput.CurrentYaw = MyPlayer->GetActorRotation().Yaw;
    SendInput.LastSentYaw = LastSentYaw;
    SendInput.bAttacking = bAttacking;

    const FP1MoveSendThrottle Decision = FP1MoveSendThrottle::Decide(SendInput);

    MovePacketSendTimer = Decision.NextTimer;
    if (Decision.bSend == false)
        return;

    const FVector DesiredMoveDirection = MyPlayer->GetDesiredMoveDirection();

    Protocol::C_MOVE MovePkt;
    Protocol::PosInfo* Info = MovePkt.mutable_info();
    Info->CopyFrom(ClientPos);
    Info->mutable_move_direction()->set_x(DesiredMoveDirection.X);
    Info->mutable_move_direction()->set_y(DesiredMoveDirection.Y);
    Info->set_state(ClientPos.state());

    FP1PacketSender::Send(this, MovePkt);
    LastSentYaw = Info->yaw();
}
