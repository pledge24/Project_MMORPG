#include "Characters/P1Creature.h"
#include "Core/P1InGamePlayerController.h"
#include "Combat/P1AttackSystemComponent.h"
#include "UI/P1NameplateWidget.h"
#include "Characters/P1MyPlayer.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Utils/LogCategory.h"
#include "Characters/P1Monster.h"

AP1Creature::AP1Creature()
{
	PrimaryActorTick.bCanEverTick = true;
    
    ClientPos = MakeShared<Protocol::PosInfo>();
    ServerPos = MakeShared<Protocol::PosInfo>();

    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    GetCharacterMovement()->bRunPhysicsWithNoController = true;
}

void AP1Creature::BeginPlay()
{
    Super::BeginPlay();

    // Set AttackSystem Component
    AttackSystemComponent = FindComponentByClass<UP1AttackSystemComponent>();
    if (AttackSystemComponent == nullptr)
        UE_LOG(LogP1CharacterComp, Warning, TEXT("AP1Creature {%s} AttackSystemComponent 누락"), *this->GetName());

    // Set and Initialize Nameplate Component
    NameplateComponent = FindComponentByClass<UWidgetComponent>();
    if (NameplateComponent)
    {
        if (UP1NameplateWidget* NameplateWidget = Cast<UP1NameplateWidget>(NameplateComponent->GetWidget()))
            NameplateWidget->InitializeWidget(this);
        else
            UE_LOG(LogP1CharacterComp, Warning, TEXT("AP1Creature::BeginPlay() NameplateWidget 누락"));
    }
    else
    {
        UE_LOG(LogP1CharacterComp, Warning, TEXT("AP1Creature::BeginPlay() NameplateComponent 누락"));
    }

}

void AP1Creature::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AP1Creature::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (IsValid(this) == false)
        return;

    // Cache: 틱마다 플레이어의 이전 틱 위치 정보 저장
    {
        FVector Location = GetActorLocation();
        ClientPos->mutable_pos()->set_x(Location.X);
        ClientPos->mutable_pos()->set_y(Location.Y);
        ClientPos->mutable_pos()->set_z(Location.Z);
        ClientPos->set_yaw(GetActorRotation().Yaw);
    }

    // MyPlayer -> Reposition | OtherPlayer -> Set Dst
    Protocol::PosInfo Info_;
    while (MoveQueue.Dequeue(Info_))
    {
        if (IsMyPlayer() == true)
            SetClientPos(Info_);
        else
            SetServerPos(Info_);
    }

    if (IsMyPlayer() == false)
    {
        S_Move(DeltaTime);
    }

}

void AP1Creature::Initialize(const Protocol::ObjectInfo& ObjectInfo)
{
    FText InName = FText::FromString(UTF8_TO_TCHAR(ObjectInfo.player_info().name().c_str()));
    SetCreatureName(InName);

    ClientPos->CopyFrom(ObjectInfo.pos_info());
    ServerPos->CopyFrom(ObjectInfo.pos_info());
}

bool AP1Creature::IsMyPlayer() const
{
    return IsA<AP1MyPlayer>();
}

bool AP1Creature::PushToMoveQueue(const Protocol::PosInfo& InInfo)
{
    return MoveQueue.Enqueue(InInfo);
}

void AP1Creature::SetMoveState(Protocol::MoveState State)
{
    if (ClientPos->state() == State)
        return;

    ClientPos->set_state(State);
}

void AP1Creature::SetClientPos(const Protocol::PosInfo& Info)
{
    if (ClientPos->object_id() != 0)
    {
        assert(SrcInfo->object_id() == Info.object_id());
    }

    ClientPos->CopyFrom(Info);

    FVector Location(Info.pos().x(), Info.pos().y(), Info.pos().z());
    SetActorLocation(Location);

    FRotator CurrentRotation = GetActorRotation();
    FRotator NewRotation = FRotator(CurrentRotation.Pitch, Info.yaw(), CurrentRotation.Roll);
    SetActorRotation(NewRotation);
}

void AP1Creature::SetServerPos(const Protocol::PosInfo& Info)
{
    if (ClientPos->object_id() != 0)
    {
        assert(ClientPos->object_id() == Info.object_id());
    }

    if (ServerPos == nullptr)
    {
        UE_LOG(LogP1Protobuf, Error, TEXT("ServerPos Is Nullptr"));
        return;
    }

    ServerPos->CopyFrom(Info);
    MoveDirection = FVector(ServerPos->mutable_move_direction()->x(), ServerPos->mutable_move_direction()->y(), 0.f);
    SetMoveState(Info.state()); // state는 ClientPos에 바로 세팅
}

void AP1Creature::SetCreatureName(const FText& InName)
{
    CreatureName = InName;
}

void AP1Creature::SetDeadState(bool IsDead)
{
    _IsDead = IsDead;

    if (IsDead)
    {
        GetCharacterMovement()->DisableMovement();
        GetCharacterMovement()->StopMovementImmediately();
    }
    else
    {
        GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }
}

void AP1Creature::S_Move(float DeltaSeconds)
{

    if (ServerPos->state() == Protocol::MOVE_STATE_RUN)
    {
        AddMovementInput(MoveDirection);
    }
    else if (ServerPos->state() == Protocol::MOVE_STATE_IDLE)
    {
        if (GetVelocity().Size() > 0.f)
        {
            GetCharacterMovement()->StopMovementImmediately();
        }
    }
    else if (ServerPos->state() == Protocol::MOVE_STATE_ACTION)
    {
        // 루트 모션이 들어간 Action 중에는 보정 안 함.
        //return;
    }

    FVector ClientLocation = GetActorLocation();
    FVector ServerLocation = FVector(ServerPos->pos().x(), ServerPos->pos().y(), ClientLocation.Z);
    const float Dist = FVector::Distance(ClientLocation, ServerLocation);

    // 회전 보정.
    bool IsMonster = this->IsA<AP1Monster>();
    bool IsIdlePlayer = ServerPos->state() == Protocol::MOVE_STATE_IDLE && this->IsA<AP1Player>();
    if (IsMonster || IsIdlePlayer)
    {
        if (ServerPos->yaw() != GetActorRotation().Yaw)
        {
            FRotator TargetRot = FRotator(0, ServerPos->yaw(), 0);
            FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaSeconds, CORR_RINTERP_SPEED);

            SetActorRotation(NewRot);
        }
    }

    // 위치 보정
    if (Dist >= CorrectionMaxThreshold)
    {
        // 보정 거리 초과 시 Reposition
        SetActorLocation(ServerLocation);
        SetActorRotation(FRotator(0, ServerPos->yaw(), 0));
    }
    else
    {
        FVector CurPos = ClientLocation;
        FVector TargetPos = FVector(ServerLocation.X, ServerLocation.Y, ClientLocation.Z);

        FVector CorrectionPoint = MoveDirection == FVector::Zero() ? TargetPos : FindPerpendicularPoint();
        FVector CorrectedClientLocation = FMath::VInterpTo(ClientLocation, CorrectionPoint, DeltaSeconds, CORR_INTERP_SPEED);

        SetActorLocation(CorrectedClientLocation);
    }
}

void AP1Creature::S_NormalAttack(uint32 Combo, float Yaw)
{
    if (IsValid(AttackSystemComponent) == false)
        return;

    SetActorRotation(FRotator(0, Yaw, 0));
    AttackSystemComponent->S_PerformNormalAttack(Combo);
}

void AP1Creature::S_Hit(int64 Damage, int64 UpdatedHp)
{
    OnHit.Broadcast(Damage, UpdatedHp);
}

void AP1Creature::S_Die()
{
    SetDeadState(true);
    
    OnDie.Broadcast(this);
}

FVector AP1Creature::FindPerpendicularPoint() const
{
    FVector TargetPoint = FVector(ServerPos->pos().x(), ServerPos->pos().y(), ClientPos->pos().z());
    FVector ClosestPoint = UKismetMathLibrary::FindClosestPointOnLine(GetActorLocation(), TargetPoint, MoveDirection);

    return ClosestPoint;
}


