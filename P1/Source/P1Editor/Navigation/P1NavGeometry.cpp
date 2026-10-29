#include "Navigation/P1NavGeometry.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Game/Data/P1GameDataSettings.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/BodySetup.h"

namespace
{
    /** 소수점 둘째 자리에서 반올림하고 끝의 0을 지운다. 배치 추출의 산출물과 같은 표기다. */
    FString FormatNumber(double Value)
    {
        double Rounded = FMath::RoundToDouble(Value * 100.0) / 100.0;
        if (Rounded == 0.0)
            Rounded = 0.0; // -0.0을 0.0으로 쓴다.

        FString Text = FString::Printf(TEXT("%.2f"), Rounded);
        while (Text.EndsWith(TEXT("0")) && Text.EndsWith(TEXT(".0")) == false)
            Text.LeftChopInline(1);
        return Text;
    }

    /**
     * 볼록 형상 하나를 더한다. 삼각형마다 형상 중심의 바깥을 향하게 꼭짓점 순서를 맞춘다.
     * 엔진이 저장한 삼각형의 감김 방향과 음수 스케일을 따지지 않아도 되게 한다.
     */
    void AddConvex(FP1NavGeometry& Geometry, const TArray<FVector>& Corners, const TArray<int32>& Indices)
    {
        if (Corners.IsEmpty())
            return;

        FVector Center = FVector::ZeroVector;
        for (const FVector& Corner : Corners)
            Center += Corner;
        Center /= Corners.Num();

        const int32 Base = Geometry.Vertices.Num();
        Geometry.Vertices.Append(Corners);

        for (int32 i = 0; i + 2 < Indices.Num(); i += 3)
        {
            int32 A = Indices[i];
            int32 B = Indices[i + 1];
            int32 C = Indices[i + 2];
            const FVector Normal = FVector::CrossProduct(Corners[B] - Corners[A], Corners[C] - Corners[A]);
            if (FVector::DotProduct(Normal, Corners[A] - Center) < 0.0)
                Swap(B, C);

            Geometry.Triangles.Append({ Base + A, Base + B, Base + C });
        }
    }

    void AddBox(FP1NavGeometry& Geometry, const FKBoxElem& Box, const FTransform& ComponentTransform)
    {
        const FTransform ElemTransform = Box.GetTransform() * ComponentTransform;
        const FVector Half(Box.X * 0.5, Box.Y * 0.5, Box.Z * 0.5);

        TArray<FVector> Corners;
        for (int32 Corner = 0; Corner < 8; ++Corner)
        {
            const FVector Local(
                (Corner & 1) ? Half.X : -Half.X,
                (Corner & 2) ? Half.Y : -Half.Y,
                (Corner & 4) ? Half.Z : -Half.Z);
            Corners.Add(ElemTransform.TransformPosition(Local));
        }

        // 꼭짓점 번호는 위 비트 배치(x 1, y 2, z 4)를 따른다. 면마다 삼각형 둘이다.
        AddConvex(Geometry, Corners, {
            0, 1, 3, 0, 3, 2,   4, 5, 7, 4, 7, 6,
            0, 1, 5, 0, 5, 4,   2, 3, 7, 2, 7, 6,
            0, 2, 6, 0, 6, 4,   1, 3, 7, 1, 7, 5,
        });
    }

    /** 볼록 요소의 삼각형. 저장된 번호가 없으면 Chaos 볼록 형상에서 계산한다. 그래도 없으면 false. */
    bool AddConvexElem(FP1NavGeometry& Geometry, const FKConvexElem& Convex, const FTransform& ComponentTransform)
    {
        TArray<int32> Indices = Convex.IndexData;
        if (Indices.IsEmpty())
            Indices = Convex.GetChaosConvexIndices();
        if (Indices.IsEmpty() || Convex.VertexData.IsEmpty())
            return false;

        const FTransform ElemTransform = Convex.GetTransform() * ComponentTransform;
        TArray<FVector> Corners;
        for (const FVector& Vertex : Convex.VertexData)
            Corners.Add(ElemTransform.TransformPosition(Vertex));

        AddConvex(Geometry, Corners, Indices);
        return true;
    }

    /** 플레이어 캡슐이 부딪히는 충돌인가. 캐릭터 이동은 폰 채널로 쓸어 보며 막힌다. */
    bool BlocksPawn(const UPrimitiveComponent& Component)
    {
        return Component.IsRegistered()
            && Component.IsQueryCollisionEnabled()
            && Component.GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
    }
}

FP1NavGeometry FP1NavGeometry::CollectLevel(const UWorld& World, TArray<FString>& OutErrors)
{
    FP1NavGeometry Geometry;
    const FString LevelName = World.GetName();

    // 레벨에 저장된 순서로 돈다. 같은 레벨이면 산출물이 실행마다 같다.
    for (const AActor* Actor : World.PersistentLevel->Actors)
    {
        // 폰은 움직이므로 지형이 아니다.
        if (Actor == nullptr || Actor->IsA<APawn>())
            continue;

        TInlineComponentArray<UPrimitiveComponent*> Components(Actor);
        for (const UPrimitiveComponent* Component : Components)
        {
            if (Component == nullptr || BlocksPawn(*Component) == false)
                continue;

            const FString Where = FString::Printf(TEXT("%s %s %s"), *LevelName, *Actor->GetActorNameOrLabel(), *Component->GetName());
            UBodySetup* Body = const_cast<UPrimitiveComponent*>(Component)->GetBodySetup();
            if (Body == nullptr)
            {
                OutErrors.Add(FString::Printf(TEXT("%s: 폰을 막는데 충돌 형상(BodySetup)이 없다"), *Where));
                continue;
            }
            if (Body->GetCollisionTraceFlag() == CTF_UseComplexAsSimple)
            {
                OutErrors.Add(FString::Printf(TEXT("%s: 복잡한 충돌을 단순 충돌로 쓴다. 상자나 볼록 형상의 단순 충돌만 지원한다"), *Where));
                continue;
            }

            const FKAggregateGeom& Aggregate = Body->AggGeom;
            if (Aggregate.SphereElems.Num() + Aggregate.SphylElems.Num() + Aggregate.TaperedCapsuleElems.Num() + Aggregate.LevelSetElems.Num()
                + Aggregate.SkinnedLevelSetElems.Num() + Aggregate.MLLevelSetElems.Num() + Aggregate.SkinnedTriangleMeshElems.Num() > 0)
            {
                OutErrors.Add(FString::Printf(TEXT("%s: 상자와 볼록 형상이 아닌 충돌 요소가 있다. 상자나 볼록 형상으로 바꾼다"), *Where));
                continue;
            }

            const FTransform ComponentTransform = Component->GetComponentTransform();
            for (const FKBoxElem& Box : Aggregate.BoxElems)
                AddBox(Geometry, Box, ComponentTransform);

            for (int32 i = 0; i < Aggregate.ConvexElems.Num(); ++i)
            {
                if (AddConvexElem(Geometry, Aggregate.ConvexElems[i], ComponentTransform) == false)
                    OutErrors.Add(FString::Printf(TEXT("%s: %d번째 볼록 요소의 삼각형을 읽지 못했다"), *Where, i));
            }
        }
    }

    if (Geometry.Triangles.IsEmpty())
        OutErrors.Add(FString::Printf(TEXT("%s: 폰을 막는 충돌이 하나도 없다"), *LevelName));

    return Geometry;
}

bool FP1NavGeometry::ReadAgent(const UP1GameDataSettings& Settings, FAgent& OutAgent, TArray<FString>& OutErrors)
{
    TOptional<FAgent> First;
    FString FirstSource;
    bool bSucceeded = true;

    TMap<int32, TSoftClassPtr<ACharacter>> Players = Settings.CharacterClassPlayers;
    Players.KeySort(TLess<int32>());
    for (const TPair<int32, TSoftClassPtr<ACharacter>>& Entry : Players)
    {
        UClass* PlayerClass = Entry.Value.LoadSynchronous();
        const ACharacter* Default = PlayerClass != nullptr ? PlayerClass->GetDefaultObject<ACharacter>() : nullptr;
        if (Default == nullptr)
        {
            OutErrors.Add(FString::Printf(TEXT("직업 %d: 플레이어 블루프린트 %s를 열지 못했다"), Entry.Key, *Entry.Value.ToString()));
            bSucceeded = false;
            continue;
        }

        FAgent Agent;
        Agent.Radius = Default->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
        Agent.Height = Default->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() * 2.0;
        Agent.MaxSlope = Default->GetCharacterMovement()->GetWalkableFloorAngle();
        Agent.MaxStep = Default->GetCharacterMovement()->MaxStepHeight;

        if (First.IsSet() == false)
        {
            First = Agent;
            FirstSource = PlayerClass->GetName();
        }
        else if (AgentToJson(Agent) != AgentToJson(First.GetValue()))
        {
            OutErrors.Add(FString::Printf(TEXT("직업 %d: %s의 캡슐이나 이동 설정이 %s와 다르다. 맵마다 navmesh가 하나라서 모든 직업이 같아야 한다"),
                Entry.Key, *PlayerClass->GetName(), *FirstSource));
            bSucceeded = false;
        }
    }

    if (First.IsSet() == false)
    {
        OutErrors.Add(TEXT("직업마다 플레이어 블루프린트가 없다. 프로젝트 설정 「P1 Game Data」의 CharacterClassPlayers에 직업을 더한다"));
        return false;
    }

    OutAgent = First.GetValue();
    return bSucceeded;
}

FString FP1NavGeometry::ToJson(int32 MapId, const FString& LevelPath) const
{
    // 꼭짓점과 삼각형이 한 줄에 하나다. 줄바꿈은 LF다.
    TArray<FString> Lines;
    Lines.Add(TEXT("{"));
    Lines.Add(FString::Printf(TEXT("\"MapId\": %d,"), MapId));
    Lines.Add(FString::Printf(TEXT("\"Level\": \"%s\","), *LevelPath));

    Lines.Add(TEXT("\"Vertices\": ["));
    for (int32 i = 0; i < Vertices.Num(); ++i)
    {
        const FVector& Vertex = Vertices[i];
        Lines.Add(FString::Printf(TEXT("[%s, %s, %s]%s"), *FormatNumber(Vertex.X), *FormatNumber(Vertex.Y), *FormatNumber(Vertex.Z),
            i + 1 < Vertices.Num() ? TEXT(",") : TEXT("")));
    }
    Lines.Add(TEXT("],"));

    Lines.Add(TEXT("\"Triangles\": ["));
    for (int32 i = 0; i + 2 < Triangles.Num(); i += 3)
    {
        Lines.Add(FString::Printf(TEXT("[%d, %d, %d]%s"), Triangles[i], Triangles[i + 1], Triangles[i + 2],
            i + 3 < Triangles.Num() ? TEXT(",") : TEXT("")));
    }
    Lines.Add(TEXT("]"));
    Lines.Add(TEXT("}"));

    return FString::Join(Lines, TEXT("\n")) + TEXT("\n");
}

FString FP1NavGeometry::AgentToJson(const FAgent& Agent)
{
    return FString::Printf(TEXT("{\"Radius\": %s, \"Height\": %s, \"MaxSlope\": %s, \"MaxStep\": %s}\n"),
        *FormatNumber(Agent.Radius), *FormatNumber(Agent.Height), *FormatNumber(Agent.MaxSlope), *FormatNumber(Agent.MaxStep));
}
