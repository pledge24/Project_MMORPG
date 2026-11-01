#include "CreatureCombat/P1CreatureCombatCheck.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    using FCreature = FP1CreatureCombatCheck::FCreature;
    using FAction = FP1CreatureCombatCheck::FAction;
    using FDesignAction = FP1CreatureCombatCheck::FDesignAction;

    /** 시간(초)의 소수 자릿수. 30fps 한 프레임(0.0333초)을 구별하고, 다시 저장한 몽타주가 float 오차로 diff를 만들지 않게 한다. */
    constexpr int32 TIME_DIGITS = 4;
    /** 길이(cm)의 소수 자릿수. 배치 데이터와 같다. */
    constexpr int32 DISTANCE_DIGITS = 2;

    //~ 산출물

    /** Digits 자리에서 반올림하고 끝의 0을 지운다. 정수도 소수점 한 자리를 남긴다(34.0). */
    FString FormatNumber(double Value, int32 Digits)
    {
        const double Scale = FMath::Pow(10.0, Digits);
        double Rounded = FMath::RoundToDouble(Value * Scale) / Scale;
        if (Rounded == 0.0)
            Rounded = 0.0; // -0.0을 0.0으로 쓴다.

        FString Text = FString::Printf(TEXT("%.*f"), Digits, Rounded);
        while (Text.EndsWith(TEXT("0")) && Text.EndsWith(TEXT(".0")) == false)
            Text.LeftChopInline(1);
        return Text;
    }

    FString JoinArray(const TArray<FString>& Values)
    {
        return TEXT("[") + FString::Join(Values, TEXT(",")) + TEXT("]");
    }

    FString WriteCreature(const FCreature& Creature)
    {
        return FString::Printf(TEXT("{\"Id\":%d,\"CapsuleRadius\":%s,\"CapsuleHalfHeight\":%s}"), Creature.Id,
            *FormatNumber(Creature.CapsuleRadius, DISTANCE_DIGITS), *FormatNumber(Creature.CapsuleHalfHeight, DISTANCE_DIGITS));
    }

    /** 액션 하나가 한 줄이다. 몽타주를 바꾸면 그 몽타주를 쓰는 액션의 줄만 바뀐다. 입력 창이 없으면 ComboWindow가 null이다. */
    FString WriteAction(const FAction& Action)
    {
        TArray<FString> HitTimes;
        for (float HitTime : Action.HitTimes)
            HitTimes.Add(FormatNumber(HitTime, TIME_DIGITS));

        // 검사를 통과했으므로 입력 창은 없거나 하나다.
        const FString ComboWindow = Action.ComboWindows.IsEmpty() ? FString(TEXT("null"))
            : JoinArray({ FormatNumber(Action.ComboWindows[0].Start, TIME_DIGITS), FormatNumber(Action.ComboWindows[0].End, TIME_DIGITS) });

        TArray<FString> RootMotion;
        for (const FVector& Sample : Action.RootMotion)
        {
            RootMotion.Add(JoinArray({ FormatNumber(Sample.X, DISTANCE_DIGITS), FormatNumber(Sample.Y, DISTANCE_DIGITS),
                FormatNumber(Sample.Z, DISTANCE_DIGITS) }));
        }

        return FString::Printf(TEXT("{\"Id\":%d,\"Montage\":\"%s\",\"Section\":\"%s\",\"Length\":%s,\"HitTimes\":%s,\"ComboWindow\":%s,\"RootMotion\":%s}"),
            Action.Id, *Action.Montage, *Action.Section, *FormatNumber(Action.Length, TIME_DIGITS), *JoinArray(HitTimes), *ComboWindow,
            *JoinArray(RootMotion));
    }

    /** 한 행이 한 줄인 배열. 행은 번호 순서이고, 행이 없으면 []. */
    template<typename TRow>
    FString WriteRows(TArray<TRow> Rows, FString (*WriteRow)(const TRow&))
    {
        if (Rows.IsEmpty())
            return TEXT("[]");

        Rows.Sort([](const TRow& A, const TRow& B) { return A.Id < B.Id; });
        TArray<FString> Lines;
        for (const TRow& Row : Rows)
            Lines.Add(WriteRow(Row));
        return TEXT("[\n") + FString::Join(Lines, TEXT(",\n")) + TEXT("\n]");
    }

    //~ 검사

    /** 크리처 하나의 캡슐. Kind는 오류 메시지의 주어(「몬스터 5000」, 「직업 1」)다. */
    void CheckCreature(const FCreature& Creature, const FString& Kind, TArray<FString>& Errors)
    {
        if (Creature.CapsuleRadius <= 0.f || Creature.CapsuleHalfHeight <= 0.f)
            Errors.Add(FString::Printf(TEXT("%s(%s): 캡슐 반지름과 반높이가 0보다 커야 한다"), *Kind, *Creature.Source));
    }

    /** 액션 하나의 섹션 타이밍을 기획 데이터의 액션과 대조한다. */
    void CheckAction(const FAction& Action, const FDesignAction& Design, TArray<FString>& Errors)
    {
        const FString Who = FString::Printf(TEXT("액션 %d(%s)"), Action.Id, *Action.Source);
        if (Action.Montage.IsEmpty())
        {
            Errors.Add(FString::Printf(TEXT("%s: 몽타주 칸이 비었다"), *Who));
            return;
        }

        const FString Where = FString::Printf(TEXT("%s %s 섹션 %s"), *Who, *Action.Montage, *Action.Section);

        // 서버는 몬스터를 루트 모션으로 옮기지 않는다. 클라이언트만 몬스터를 옮기면 두 위치가 어긋난다.
        if (Action.bHasRootMotion && Design.bUsedByMonster)
            Errors.Add(FString::Printf(TEXT("%s: 몬스터가 쓰는 액션의 몽타주에 루트 모션이 있다. 서버는 몬스터를 루트 모션으로 옮기지 않는다"), *Where));

        // 섹션 길이와 타격 시점은 트랙 시간이라, 재생 속도가 1이 아니면 클라이언트가 재생하는 초와 서버의 판정 시점이 어긋난다.
        if (FMath::IsNearlyEqual(Action.RateScale, 1.f) == false)
            Errors.Add(FString::Printf(TEXT("%s: 몽타주의 RateScale이 1이 아니다(%s). 재생 속도 대신 애니메이션 길이를 고친다"),
                *Where, *FormatNumber(Action.RateScale, TIME_DIGITS)));

        if (Action.Length <= 0.f)
            Errors.Add(FString::Printf(TEXT("%s: 길이가 0보다 커야 한다"), *Where));

        // 서버는 타격 순번 i의 공격 범위를 i번째 타격 시점에 판정한다.
        if (Action.HitTimes.Num() != Design.HitCount)
            Errors.Add(FString::Printf(TEXT("%s: 타격 노티파이(P1 Attack Hit)가 %d개인데 액션 표의 타격은 %d개다"),
                *Where, Action.HitTimes.Num(), Design.HitCount));
        for (float HitTime : Action.HitTimes)
        {
            if (HitTime < 0.f || HitTime > Action.Length)
                Errors.Add(FString::Printf(TEXT("%s: 타격 시점 %s초가 섹션 밖이다"), *Where, *FormatNumber(HitTime, TIME_DIGITS)));
        }

        if (Design.bHasNextAction == false)
        {
            if (Action.ComboWindows.IsEmpty() == false)
                Errors.Add(FString::Printf(TEXT("%s: 다음 액션이 없는데 콤보 입력 창(P1 Combo Window)이 있다"), *Where));
            return;
        }

        // 다음 단계는 이 구간 안의 입력으로만 이어진다. 구간이 섹션 끝을 넘으면 다음 단계가 시작된 뒤에도 앞 단계의 입력 창이 열려 있다.
        if (Action.ComboWindows.Num() != 1)
            Errors.Add(FString::Printf(TEXT("%s: 콤보 입력 창(P1 Combo Window)이 %d개다. 다음 액션이 있으면 하나 둔다"), *Where, Action.ComboWindows.Num()));
        for (const FP1CreatureCombatCheck::FComboWindow& Window : Action.ComboWindows)
        {
            if (Window.End >= Action.Length)
                Errors.Add(FString::Printf(TEXT("%s: 콤보 입력 창이 섹션 끝(%s초) 전에 끝나지 않고 %s초에 끝난다"), *Where,
                    *FormatNumber(Action.Length, TIME_DIGITS), *FormatNumber(Window.End, TIME_DIGITS)));
        }
    }

    //~ 기획 데이터

    bool ReadRows(const FString& Json, const TCHAR* FileName, TArray<TSharedPtr<FJsonValue>>& OutRows, TArray<FString>& OutErrors)
    {
        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
        if (FJsonSerializer::Deserialize(Reader, OutRows) == false)
        {
            OutErrors.Add(FString::Printf(TEXT("%s: 행 배열로 읽지 못했다: %s"), FileName, *Reader->GetErrorMessage()));
            return false;
        }
        return true;
    }
}

bool FP1CreatureCombatCheck::ParseDesignData(const FString& ActionJson, const FString& MonsterJson, FInput& Input, TArray<FString>& OutErrors)
{
    TArray<TSharedPtr<FJsonValue>> ActionRows;
    TArray<TSharedPtr<FJsonValue>> MonsterRows;
    if (ReadRows(ActionJson, TEXT("Action.json"), ActionRows, OutErrors) == false
        || ReadRows(MonsterJson, TEXT("Monster.json"), MonsterRows, OutErrors) == false)
        return false;

    bool bOk = true;
    for (int32 Index = 0; Index < ActionRows.Num(); ++Index)
    {
        const TSharedPtr<FJsonObject> Row = ActionRows[Index]->AsObject();
        int32 Id = 0;
        const TArray<TSharedPtr<FJsonValue>>* Hits = nullptr;
        const TSharedPtr<FJsonValue> NextAction = Row.IsValid() ? Row->Values.FindRef(TEXT("NextAction")) : nullptr;
        if (Row.IsValid() == false || Row->TryGetNumberField(TEXT("Id"), Id) == false || Row->TryGetArrayField(TEXT("Hits"), Hits) == false
            || NextAction.IsValid() == false)
        {
            OutErrors.Add(FString::Printf(TEXT("Action.json %d번째 행: 'Id', 'Hits', 'NextAction' 중 하나가 없다"), Index + 1));
            bOk = false;
            continue;
        }

        FDesignAction& Design = Input.DesignActions.Add(Id);
        Design.HitCount = Hits->Num();
        // 콤보의 마지막 단계는 다음 액션이 null이다.
        Design.bHasNextAction = NextAction->IsNull() == false;
    }

    for (int32 Index = 0; Index < MonsterRows.Num(); ++Index)
    {
        const TSharedPtr<FJsonObject> Row = MonsterRows[Index]->AsObject();
        int32 Id = 0;
        int32 ActionId = 0;
        if (Row.IsValid() == false || Row->TryGetNumberField(TEXT("Id"), Id) == false
            || Row->TryGetNumberField(TEXT("NormalAttackAction"), ActionId) == false)
        {
            OutErrors.Add(FString::Printf(TEXT("Monster.json %d번째 행: 'Id'나 'NormalAttackAction'이 없다"), Index + 1));
            bOk = false;
            continue;
        }

        Input.DesignMonsterIds.Add(Id);
        // 없는 액션을 가리키는 몬스터는 DataBuilder의 참조 규칙이 막는다.
        if (FDesignAction* Design = Input.DesignActions.Find(ActionId))
            Design->bUsedByMonster = true;
    }
    return bOk;
}

FP1CreatureCombatCheck::FResult FP1CreatureCombatCheck::Run(const FInput& Input)
{
    FResult Result;
    TArray<FString>& Errors = Result.Errors;

    // 몬스터는 몬스터 표와 하나씩 짝이 맞아야 한다. 짝이 없는 몬스터가 있으면 서버가 뜨지 않는다.
    TSet<int32> MonsterIds;
    for (const FCreature& Monster : Input.Monsters)
    {
        const FString Kind = FString::Printf(TEXT("몬스터 %d"), Monster.Id);
        if (MonsterIds.Contains(Monster.Id))
            Errors.Add(FString::Printf(TEXT("%s(%s): 몬스터 에셋 테이블에 같은 번호가 둘이다"), *Kind, *Monster.Source));
        MonsterIds.Add(Monster.Id);

        if (Input.DesignMonsterIds.Contains(Monster.Id) == false)
            Errors.Add(FString::Printf(TEXT("%s(%s): 몬스터 표에 없다"), *Kind, *Monster.Source));
        CheckCreature(Monster, Kind, Errors);
    }

    TArray<int32> DesignMonsterIds = Input.DesignMonsterIds.Array();
    DesignMonsterIds.Sort();
    for (int32 MonsterId : DesignMonsterIds)
    {
        if (MonsterIds.Contains(MonsterId) == false)
            Errors.Add(FString::Printf(TEXT("몬스터 %d: 몬스터 에셋 테이블에 행이 없다"), MonsterId));
    }

    // 직업은 클라이언트에 직업 표가 없어서 서버가 기동할 때 대조한다.
    if (Input.Classes.IsEmpty())
        Errors.Add(TEXT("직업마다 플레이어 블루프린트가 없다. 프로젝트 설정 「P1 Game Data」의 CharacterClassPlayers에 직업을 더한다"));
    for (const FCreature& Class : Input.Classes)
        CheckCreature(Class, FString::Printf(TEXT("직업 %d"), Class.Id), Errors);

    // 액션은 액션 표와 하나씩 짝이 맞아야 한다. 짝이 없는 액션이 있으면 서버가 뜨지 않는다.
    TSet<int32> ActionIds;
    for (const FAction& Action : Input.Actions)
    {
        if (ActionIds.Contains(Action.Id))
            Errors.Add(FString::Printf(TEXT("액션 %d(%s): 액션 에셋 테이블에 같은 번호가 둘이다"), Action.Id, *Action.Source));
        ActionIds.Add(Action.Id);

        const FDesignAction* Design = Input.DesignActions.Find(Action.Id);
        if (Design == nullptr)
        {
            Errors.Add(FString::Printf(TEXT("액션 %d(%s): 액션 표에 없다"), Action.Id, *Action.Source));
            continue;
        }
        CheckAction(Action, *Design, Errors);
    }

    TArray<int32> DesignActionIds;
    Input.DesignActions.GetKeys(DesignActionIds);
    DesignActionIds.Sort();
    for (int32 ActionId : DesignActionIds)
    {
        if (ActionIds.Contains(ActionId) == false)
            Errors.Add(FString::Printf(TEXT("액션 %d: 액션 에셋 테이블에 행이 없다"), ActionId));
    }

    if (Errors.IsEmpty() == false)
        return Result;

    Result.File = FString::Printf(TEXT("{\"RootMotionSampleRate\":%d,\n\"Monsters\":%s,\n\"Classes\":%s,\n\"Actions\":%s}\n"),
        ROOT_MOTION_SAMPLE_RATE, *WriteRows(Input.Monsters, WriteCreature), *WriteRows(Input.Classes, WriteCreature),
        *WriteRows(Input.Actions, WriteAction));
    return Result;
}
