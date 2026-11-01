// 클라이언트가 공격 범위를 서버와 같은 규칙으로 계산하는지 고정한다.
//
// 왜 이것인가: 공격 범위 디버그 표시는 기획자가 화면에서 범위를 보고 수치를 정하는 도구다. 클라이언트가 오프셋 방향,
// 방향의 회전, 높이 기준, 형상의 뜻을 서버와 다르게 읽으면 화면의 범위와 서버의 판정 범위가 어긋나고, 기획자는 틀린
// 그림을 보고 수치를 정한다.
//
// 겹침 사례는 서버 테스트와 같은 파일(Server/GameServerTests/Data/AttackAreaCases.json)을 읽는다. 사례를 고치면 두 테스트가
// 함께 그 답을 따른다. 그린 윤곽선은 그 겹침 규칙으로 확인한다. 윤곽선 위의 점은 범위에 닿고, 윤곽선 밖으로 조금 나간
// 점은 닿지 않아야 한다.
//
// 실행: pwsh P1/Scripts/Run-UeTests.ps1 -Filter P1.Combat.AttackArea

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Game/Combat/P1AttackArea.h"
#include "Game/Data/Generated/P1ActionHitRow.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace P1AttackAreaTest
{
    FString CasesPath()
    {
        return FPaths::ConvertRelativePathToFull(
            FPaths::Combine(FPaths::ProjectDir(), TEXT("../Server/GameServerTests/Data/AttackAreaCases.json")));
    }

    TSharedPtr<FJsonObject> ReadCases()
    {
        FString Text;
        if (FFileHelper::LoadFileToString(Text, *CasesPath()) == false)
            return nullptr;

        TSharedPtr<FJsonObject> Root;
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root);
        return Root;
    }

    /** 사례 파일은 형상을 서버 열거형 이름(Circle)으로 적는다. */
    EP1AttackAreaShape ReadShape(const FString& Name)
    {
        if (Name == TEXT("Circle"))
            return EP1AttackAreaShape::CIRCLE;
        if (Name == TEXT("Sector"))
            return EP1AttackAreaShape::SECTOR;
        if (Name == TEXT("Rectangle"))
            return EP1AttackAreaShape::RECTANGLE;
        return EP1AttackAreaShape::None;
    }

    FP1ActionHitRow ReadArea(const FJsonObject& Object)
    {
        FP1ActionHitRow Hit;
        Hit.Shape = ReadShape(Object.GetStringField(TEXT("Shape")));
        Hit.Radius = Object.GetNumberField(TEXT("Radius"));
        Hit.Angle = Object.GetNumberField(TEXT("Angle"));
        Hit.Length = Object.GetNumberField(TEXT("Length"));
        Hit.Width = Object.GetNumberField(TEXT("Width"));
        Hit.OffsetX = Object.GetNumberField(TEXT("OffsetX"));
        Hit.OffsetY = Object.GetNumberField(TEXT("OffsetY"));
        Hit.ZMin = Object.GetNumberField(TEXT("ZMin"));
        Hit.ZMax = Object.GetNumberField(TEXT("ZMax"));
        return Hit;
    }

    FVector ReadPosition(const FJsonObject& Object)
    {
        return FVector(Object.GetNumberField(TEXT("X")), Object.GetNumberField(TEXT("Y")), Object.GetNumberField(TEXT("Z")));
    }

    FP1ActionHitRow MakeHit(EP1AttackAreaShape Shape)
    {
        FP1ActionHitRow Hit;
        Hit.Shape = Shape;
        Hit.OffsetX = 40.f;
        Hit.OffsetY = -25.f;
        Hit.ZMin = -100.f;
        Hit.ZMax = 200.f;
        return Hit;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FP1AttackAreaTest,
    "P1.Combat.AttackArea",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1AttackAreaTest::RunTest(const FString& Parameters)
{
    using namespace P1AttackAreaTest;

    const TSharedPtr<FJsonObject> Cases = ReadCases();
    if (TestTrue(FString::Printf(TEXT("사례 파일 %s를 읽음"), *CasesPath()), Cases.IsValid()) == false)
        return false;

    // 1) 경계 사례의 답은 여유에 달려 있다. 서버와 같은 여유를 쓴다.
    TestEqual(TEXT("경계 여유"), P1AttackArea::OverlapTolerance, static_cast<float>(Cases->GetNumberField(TEXT("Tolerance"))));

    // 2) 공용 겹침 사례마다 서버와 같은 답을 낸다. 사례가 없으면 아무것도 확인하지 않고 통과하므로 형상마다 사례가 있는지도 본다.
    {
        const TArray<TSharedPtr<FJsonValue>>& OverlapCases = Cases->GetArrayField(TEXT("OverlapCases"));
        TestTrue(TEXT("겹침 사례가 있음"), OverlapCases.Num() > 0);

        TSet<EP1AttackAreaShape> CoveredShapes;
        for (const TSharedPtr<FJsonValue>& Value : OverlapCases)
        {
            const FJsonObject& Case = *Value->AsObject();
            const FJsonObject& Attacker = *Case.GetObjectField(TEXT("Attacker"));
            const FJsonObject& Target = *Case.GetObjectField(TEXT("Target"));
            const FP1ActionHitRow Hit = ReadArea(*Case.GetObjectField(TEXT("Area")));
            CoveredShapes.Add(Hit.Shape);

            const bool bOverlaps = P1AttackArea::Overlaps(
                Hit, ReadPosition(Attacker), Attacker.GetNumberField(TEXT("Yaw")),
                ReadPosition(Target), Target.GetNumberField(TEXT("Radius")), Target.GetNumberField(TEXT("HalfHeight")));
            TestEqual(Case.GetStringField(TEXT("Name")), bOverlaps, Case.GetBoolField(TEXT("Expected")));
        }

        TestEqual(TEXT("형상마다 사례가 있음"), CoveredShapes.Num(), 3);
    }

    // 3) 윤곽선의 꼭짓점을 월드로 옮긴 점은 범위에 닿고, 형상 안쪽에서 바깥쪽으로 2cm 나간 점은 닿지 않는다.
    //    공격자를 돌리고 오프셋을 줘서, 그리는 쪽의 회전과 오프셋이 판정과 같은지 함께 본다.
    {
        const FVector Foot(1000.0, -500.0, 50.0);
        const float Yaw = 35.f;

        FP1ActionHitRow Circle = MakeHit(EP1AttackAreaShape::CIRCLE);
        Circle.Radius = 150.f;
        FP1ActionHitRow Sector = MakeHit(EP1AttackAreaShape::SECTOR);
        Sector.Radius = 200.f;
        Sector.Angle = 120.f;
        FP1ActionHitRow WideSector = MakeHit(EP1AttackAreaShape::SECTOR);
        WideSector.Radius = 200.f;
        WideSector.Angle = 270.f;
        FP1ActionHitRow FullSector = MakeHit(EP1AttackAreaShape::SECTOR);
        FullSector.Radius = 200.f;
        FullSector.Angle = 360.f;
        FP1ActionHitRow Rectangle = MakeHit(EP1AttackAreaShape::RECTANGLE);
        Rectangle.Length = 600.f;
        Rectangle.Width = 80.f;

        const TPair<const TCHAR*, FP1ActionHitRow> Shapes[] = {
            { TEXT("원"), Circle },
            { TEXT("부채꼴"), Sector },
            { TEXT("180도 넘는 부채꼴"), WideSector },
            { TEXT("360도 부채꼴"), FullSector },
            { TEXT("직사각형"), Rectangle },
        };
        for (const TPair<const TCHAR*, FP1ActionHitRow>& Shape : Shapes)
        {
            const TArray<FVector2D> Outline = P1AttackArea::MakeOutline(Shape.Value, 32);
            if (TestTrue(FString::Printf(TEXT("%s: 윤곽선이 있음"), Shape.Key), Outline.Num() >= 3) == false)
                continue;

            // 바깥 방향은 형상 기준점이 아니라 형상 안쪽의 점에서 잰다. 부채꼴의 기준점은 꼭짓점이라 윤곽선 위에 있다.
            const FVector2D Inside = FVector2D(Shape.Value.OffsetX, Shape.Value.OffsetY)
                + (Shape.Value.Shape == EP1AttackAreaShape::CIRCLE ? FVector2D::ZeroVector : FVector2D(20.0, 0.0));
            for (int32 Index = 0; Index < Outline.Num(); ++Index)
            {
                const FVector OnEdge = P1AttackArea::ToWorld(Outline[Index], Foot, Yaw, 0.f);
                TestTrue(FString::Printf(TEXT("%s: 꼭짓점 %d가 닿음"), Shape.Key, Index),
                    P1AttackArea::Overlaps(Shape.Value, Foot, Yaw, OnEdge, 0.f, 1.f));

                const FVector2D Outward = (Outline[Index] - Inside).GetSafeNormal();
                const FVector Outside = P1AttackArea::ToWorld(Outline[Index] + Outward * 2.0, Foot, Yaw, 0.f);
                TestFalse(FString::Printf(TEXT("%s: 꼭짓점 %d 바깥은 닿지 않음"), Shape.Key, Index),
                    P1AttackArea::Overlaps(Shape.Value, Foot, Yaw, Outside, 0.f, 1.f));
            }
        }
    }

    // 4) 월드로 옮긴 점의 높이는 공격자 발밑 기준이다. 높이 범위의 위아래를 그 높이로 그린다.
    {
        const FVector Point = P1AttackArea::ToWorld(FVector2D(100.0, 0.0), FVector(0.0, 0.0, 50.0), 90.f, 200.f);
        TestTrue(TEXT("방향 90도의 앞은 +Y"), Point.Equals(FVector(0.0, 100.0, 250.0), 0.01));
    }

    // 5) 형상이 정해지지 않은 행은 그리지 않는다.
    TestEqual(TEXT("형상 없음"), P1AttackArea::MakeOutline(FP1ActionHitRow(), 32).Num(), 0);

    return true;
}

#endif
