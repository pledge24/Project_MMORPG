// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Objects/Creature.h"
#include "InGamePlayerController.h"
#include "AttackSystemComponent.h"
#include "NameplateWidget.h"
#include "P1MyPlayer.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Log/LogCategory.h"

ACreature::ACreature()
{
	PrimaryActorTick.bCanEverTick = true;
    
    ClientPos = new Protocol::PosInfo();
    ServerPos = new Protocol::PosInfo();

    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    GetCharacterMovement()->bRunPhysicsWithNoController = true;
}

void ACreature::BeginPlay()
{
    Super::BeginPlay();

    // Set AttackSystem Component
    AttackSystemComponent = FindComponentByClass<UAttackSystemComponent>();
    if (AttackSystemComponent == nullptr)
        UE_LOG(LogTemp, Warning, TEXT("ACreature {%s} AttackSystemComponent 누락"), *this->GetName());

    // Set and Initialize Nameplate Component
    NameplateComponent = FindComponentByClass<UWidgetComponent>();
    if (NameplateComponent)
    {
        if (UNameplateWidget* NameplateWidget = Cast<UNameplateWidget>(NameplateComponent->GetWidget()))
            NameplateWidget->InitializeWidget(this);
        else
            UE_LOG(LogTemp, Warning, TEXT("ACreature::BeginPlay() NameplateWidget 누락"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ACreature::BeginPlay() NameplateComponent 누락"));
    }


    {
        FVector Location = GetActorLocation();
        ServerPos->set_x(Location.X);
        ServerPos->set_y(Location.Y);
        ServerPos->set_z(Location.Z);
        ServerPos->set_yaw(GetControlRotation().Yaw);
        ServerPos->set_state(Protocol::MOVE_STATE_IDLE);

        ClientPos->CopyFrom(*ServerPos);
    }
}

void ACreature::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    {
        delete ClientPos;
        delete ServerPos;

        ClientPos = nullptr;
        ServerPos = nullptr;
    }
}

void ACreature::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (IsPendingKill() || IsActorBeingDestroyed())
        return;

    // Cache: 틱마다 플레이어의 이전 틱 위치 정보 저장
    {
        FVector Location = GetActorLocation();
        ClientPos->set_x(Location.X);
        ClientPos->set_y(Location.Y);
        ClientPos->set_z(Location.Z);
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

void ACreature::Initialize(const Protocol::ObjectInfo& ObjectInfo)
{
    FText InName = FText::FromString(UTF8_TO_TCHAR(ObjectInfo.player_info().name().c_str()));
    SetCreatureName(InName);
}

bool ACreature::IsMyPlayer() const
{
    return IsA<AP1MyPlayer>();
}

bool ACreature::PushToMoveQueue(const Protocol::PosInfo& InInfo)
{
    return MoveQueue.Enqueue(InInfo);
}

void ACreature::SetMoveState(Protocol::MoveState State)
{
    if (ClientPos->state() == State)
        return;

    ClientPos->set_state(State);
}

void ACreature::SetClientPos(const Protocol::PosInfo& Info)
{
    if (ClientPos->object_id() != 0)
    {
        assert(SrcInfo->object_id() == Info.object_id());
    }

    ClientPos->CopyFrom(Info);

    FVector Location(Info.x(), Info.y(), Info.z());
    SetActorLocation(Location);

    FRotator CurrentRotation = GetActorRotation();
    FRotator NewRotation = FRotator(CurrentRotation.Pitch, Info.yaw(), CurrentRotation.Roll);
    SetActorRotation(NewRotation);
}

void ACreature::SetServerPos(const Protocol::PosInfo& Info)
{
    if (ClientPos->object_id() != 0)
    {
        assert(ClientPos->object_id() == Info.object_id());
    }

    if (ServerPos == nullptr)
    {
        UE_LOG(LogProtobuf, Error, TEXT("ServerPos Is Nullptr"));
        return;
    }

    ServerPos->CopyFrom(Info);
    MoveDirection = FRotator(0.f, ServerPos->desired_yaw(), 0.f).Vector();
    SetMoveState(Info.state()); // state는 ClientPos에 바로 세팅
}

void ACreature::SetCreatureName(const FText& InName)
{
    CreatureName = InName;
}

void ACreature::S_Move(float DeltaSeconds)
{
    FVector ClientLocation = GetActorLocation();
    FVector ServerLocation = FVector(ServerPos->x(), ServerPos->y(), ServerPos->z());
    const float Dist = FVector::Distance(ClientLocation, ServerLocation);

    // S_Move
    if (ServerPos->state() == Protocol::MOVE_STATE_RUN)
    {
        AddMovementInput(MoveDirection);
    }
    // 제자리 회전 보정
    else if (ServerPos->state() == Protocol::MOVE_STATE_IDLE)
    {
        if (ServerPos->yaw() != GetActorRotation().Yaw)
        {
            FRotator TargetRot = FRotator(0, ServerPos->yaw(), 0);
            FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaSeconds, CORR_RINTERP_SPEED);

            SetActorRotation(NewRot);
        }
    }
    else if (ServerPos->state() == Protocol::MOVE_STATE_ACTION)
    {
        // 루트 모션이 들어간 Action 중에는 보정 안 함.
        return;
    }

    // Correction
    if (Dist > CorrectionThreshold)
    {
        // 보정 거리 초과 시 Reposition
        SetActorLocation(ServerLocation);
        SetActorRotation(FRotator(0, ServerPos->yaw(), 0));
    }
    else
    {
        FVector CorrectionPoint = MoveDirection == FVector::Zero() ?
            ServerLocation : FindPerpendicularPoint();

        FVector CorrectedClientLocation = FMath::VInterpTo(ClientLocation, CorrectionPoint, DeltaSeconds, CORR_INTERP_SPEED);
        CorrectedClientLocation.Z = ClientLocation.Z;

        SetActorLocation(CorrectedClientLocation);
    }
}

void ACreature::S_NormalAttack(uint32 Combo)
{
    if (IsValid(AttackSystemComponent) == false)
        return;

    AttackSystemComponent->S_PerformNormalAttack(Combo);
}

FVector ACreature::FindPerpendicularPoint() const
{
    FVector ServerPoint = FVector(ServerPos->x(), ServerPos->y(), ServerPos->z());
    FVector ClosestPoint = UKismetMathLibrary::FindClosestPointOnLine(GetActorLocation(), ServerPoint, MoveDirection);

    return ClosestPoint;
}


