#include "CreatureCombat/P1CreatureCombatCheck.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    using FSection = FP1CreatureCombatCheck::FSection;
    using FAttack = FP1CreatureCombatCheck::FAttack;
    using FCreature = FP1CreatureCombatCheck::FCreature;

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

    FString WriteSection(const FSection& Section)
    {
        TArray<FString> HitTimes;
        for (float HitTime : Section.HitTimes)
            HitTimes.Add(FormatNumber(HitTime, TIME_DIGITS));

        TArray<FString> RootMotion;
        for (const FVector& Sample : Section.RootMotion)
        {
            RootMotion.Add(JoinArray({ FormatNumber(Sample.X, DISTANCE_DIGITS), FormatNumber(Sample.Y, DISTANCE_DIGITS),
                FormatNumber(Sample.Z, DISTANCE_DIGITS) }));
        }

        return FString::Printf(TEXT("{\"Name\":\"%s\",\"Length\":%s,\"HitTimes\":%s,\"RootMotion\":%s}"),
            *Section.Name, *FormatNumber(Section.Length, TIME_DIGITS), *JoinArray(HitTimes), *JoinArray(RootMotion));
    }

    /** 크리처 하나가 한 줄이다. 몽타주를 바꾸면 그 몽타주를 쓰는 크리처의 줄만 바뀐다. */
    FString WriteCreature(const FCreature& Creature)
    {
        TArray<FString> Attacks;
        for (const FAttack& Attack : Creature.Attacks)
        {
            TArray<FString> Sections;
            for (const FSection& Section : Attack.Sections)
                Sections.Add(WriteSection(Section));
            Attacks.Add(FString::Printf(TEXT("{\"Montage\":\"%s\",\"Sections\":%s}"), *Attack.Montage, *JoinArray(Sections)));
        }

        return FString::Printf(TEXT("{\"Id\":%d,\"CapsuleRadius\":%s,\"CapsuleHalfHeight\":%s,\"Attacks\":%s}"), Creature.Id,
            *FormatNumber(Creature.CapsuleRadius, DISTANCE_DIGITS), *FormatNumber(Creature.CapsuleHalfHeight, DISTANCE_DIGITS), *JoinArray(Attacks));
    }

    /** 한 행이 한 줄인 배열. 행이 없으면 []. */
    FString WriteRows(const TArray<FCreature>& Creatures)
    {
        if (Creatures.IsEmpty())
            return TEXT("[]");

        TArray<FString> Rows;
        for (const FCreature& Creature : Creatures)
            Rows.Add(WriteCreature(Creature));
        return TEXT("[\n") + FString::Join(Rows, TEXT(",\n")) + TEXT("\n]");
    }

    //~ 검사

    /** 크리처 하나의 캡슐과 공격 몽타주. Kind는 오류 메시지의 주어(「몬스터 5000」, 「직업 1」)다. */
    void CheckCreature(const FCreature& Creature, const FString& Kind, bool bAllowRootMotion, TArray<FString>& Errors)
    {
        const FString Who = FString::Printf(TEXT("%s(%s)"), *Kind, *Creature.Source);
        if (Creature.CapsuleRadius <= 0.f || Creature.CapsuleHalfHeight <= 0.f)
            Errors.Add(FString::Printf(TEXT("%s: 캡슐 반지름과 반높이가 0보다 커야 한다"), *Who));

        if (Creature.Attacks.IsEmpty())
            Errors.Add(FString::Printf(TEXT("%s: 공격 몽타주가 없다"), *Who));

        for (int32 Index = 0; Index < Creature.Attacks.Num(); ++Index)
        {
            const FAttack& Attack = Creature.Attacks[Index];
            if (Attack.Montage.IsEmpty())
            {
                Errors.Add(FString::Printf(TEXT("%s: 공격 몽타주 %d번째 칸이 비었다"), *Who, Index + 1));
                continue;
            }

            // 서버는 몬스터를 루트 모션으로 옮기지 않는다. 클라이언트만 몬스터를 옮기면 두 위치가 어긋난다.
            if (Attack.bHasRootMotion && bAllowRootMotion == false)
                Errors.Add(FString::Printf(TEXT("%s %s: 몬스터의 공격 몽타주에 루트 모션이 있다. 서버는 몬스터를 루트 모션으로 옮기지 않는다"), *Who, *Attack.Montage));

            // 섹션 길이와 타격 시점은 트랙 시간이라, 재생 속도가 1이 아니면 클라이언트가 재생하는 초와 서버의 판정 시점이 어긋난다.
            if (FMath::IsNearlyEqual(Attack.RateScale, 1.f) == false)
                Errors.Add(FString::Printf(TEXT("%s %s: 몽타주의 RateScale이 1이 아니다(%s). 재생 속도 대신 애니메이션 길이를 고친다"),
                    *Who, *Attack.Montage, *FormatNumber(Attack.RateScale, TIME_DIGITS)));

            if (Attack.Sections.IsEmpty())
                Errors.Add(FString::Printf(TEXT("%s %s: 섹션이 없다"), *Who, *Attack.Montage));

            for (const FSection& Section : Attack.Sections)
            {
                const FString Where = FString::Printf(TEXT("%s %s 섹션 %s"), *Who, *Attack.Montage, *Section.Name);
                if (Section.Length <= 0.f)
                    Errors.Add(FString::Printf(TEXT("%s: 길이가 0보다 커야 한다"), *Where));
                if (Section.HitTimes.IsEmpty())
                    Errors.Add(FString::Printf(TEXT("%s: 타격 노티파이(P1 Attack Hit)가 없다"), *Where));
                for (float HitTime : Section.HitTimes)
                {
                    if (HitTime < 0.f || HitTime > Section.Length)
                        Errors.Add(FString::Printf(TEXT("%s: 타격 시점 %s초가 섹션 밖이다"), *Where, *FormatNumber(HitTime, TIME_DIGITS)));
                }
            }
        }
    }
}

bool FP1CreatureCombatCheck::ParseMonsterIds(const FString& MonsterJson, TSet<int32>& OutIds, TArray<FString>& OutErrors)
{
    TArray<TSharedPtr<FJsonValue>> Rows;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(MonsterJson);
    if (FJsonSerializer::Deserialize(Reader, Rows) == false)
    {
        OutErrors.Add(FString::Printf(TEXT("Monster.json: 행 배열로 읽지 못했다: %s"), *Reader->GetErrorMessage()));
        return false;
    }

    bool bOk = true;
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        const TSharedPtr<FJsonObject> Row = Rows[Index]->AsObject();
        int32 Id = 0;
        if (Row.IsValid() && Row->TryGetNumberField(TEXT("Id"), Id))
        {
            OutIds.Add(Id);
            continue;
        }

        OutErrors.Add(FString::Printf(TEXT("Monster.json %d번째 행: 'Id'가 없다"), Index + 1));
        bOk = false;
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
        CheckCreature(Monster, Kind, false, Errors);
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
        CheckCreature(Class, FString::Printf(TEXT("직업 %d"), Class.Id), true, Errors);

    if (Errors.IsEmpty() == false)
        return Result;

    // 행은 번호 순서다.
    TArray<FCreature> Monsters = Input.Monsters;
    Monsters.Sort([](const FCreature& A, const FCreature& B) { return A.Id < B.Id; });
    TArray<FCreature> Classes = Input.Classes;
    Classes.Sort([](const FCreature& A, const FCreature& B) { return A.Id < B.Id; });

    Result.File = FString::Printf(TEXT("{\"RootMotionSampleRate\":%d,\n\"Monsters\":%s,\n\"Classes\":%s}\n"),
        ROOT_MOTION_SAMPLE_RATE, *WriteRows(Monsters), *WriteRows(Classes));
    return Result;
}
