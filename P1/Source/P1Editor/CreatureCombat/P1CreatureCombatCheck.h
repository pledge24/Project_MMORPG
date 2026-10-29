#pragma once

#include "CoreMinimal.h"

/**
 * 에셋에서 모은 크리처 판정 데이터의 검사와 서버가 읽는 파일의 작성.
 *
 * 몬스터와 직업마다 캡슐 크기와 공격 몽타주의 타격 시점이 있는지, 몬스터 표와 짝이 맞는지 검사하고, 통과하면
 * 크리처 판정 데이터 JSON을 만든다. 에셋과 UObject를 모르므로 에셋을 연 쪽이 값을 넘긴다.
 */
class P1EDITOR_API FP1CreatureCombatCheck
{
public:
    /** 루트 모션 곡선의 표본 수(초당). 서버가 같은 간격으로 읽는다. */
    static constexpr int32 ROOT_MOTION_SAMPLE_RATE = 30;

    /** 몽타주 섹션 하나. 시간은 섹션 시작에서 잰 초다. */
    struct FSection
    {
        FString Name;
        float Length = 0.f;
        /** 타격 노티파이의 시점. 시간 순서다. */
        TArray<float> HitTimes;
        /** 1/ROOT_MOTION_SAMPLE_RATE초 간격으로 잰 누적 루트 모션 이동량(캐릭터 로컬, cm). 첫 표본은 0이고 마지막 표본은 섹션 끝이다. 루트 모션이 없으면 비어 있다. */
        TArray<FVector> RootMotion;
    };

    /** 콤보 순번 하나의 공격 몽타주. Montage가 비어 있으면 몽타주 칸이 비었다. */
    struct FAttack
    {
        /** 몽타주의 오브젝트 경로. 오류 메시지와 산출물에 쓴다. */
        FString Montage;
        bool bHasRootMotion = false;
        TArray<FSection> Sections;
    };

    /** 몬스터 하나나 직업 하나. Id는 몬스터 번호나 직업 번호다. */
    struct FCreature
    {
        int32 Id = 0;
        /** 오류 메시지에 쓰는 출처(에셋 테이블의 행이나 플레이어 블루프린트). */
        FString Source;
        float CapsuleRadius = 0.f;
        float CapsuleHalfHeight = 0.f;
        TArray<FAttack> Attacks;
    };

    struct FInput
    {
        TArray<FCreature> Monsters;
        TArray<FCreature> Classes;
        /** 클라이언트 기획 데이터의 몬스터 번호. */
        TSet<int32> DesignMonsterIds;
    };

    struct FResult
    {
        /** 오류 한 줄에 몬스터 번호나 직업 번호가 들어 있다. */
        TArray<FString> Errors;
        /** 크리처 판정 데이터 JSON. 오류가 하나라도 있으면 비어 있다. */
        FString File;

        int32 GetExitCode() const { return Errors.IsEmpty() ? 0 : 1; }
    };

    /** 클라이언트 기획 데이터의 몬스터 표 JSON에서 번호를 읽는다. 형식이 틀리면 OutErrors에 사유를 더하고 false. */
    static bool ParseMonsterIds(const FString& MonsterJson, TSet<int32>& OutIds, TArray<FString>& OutErrors);

    /** 몬스터와 직업을 함께 검사하고, 통과하면 산출물을 만든다. 몬스터 표와의 짝을 보려고 모든 몬스터를 한 번에 받는다. */
    static FResult Run(const FInput& Input);
};
