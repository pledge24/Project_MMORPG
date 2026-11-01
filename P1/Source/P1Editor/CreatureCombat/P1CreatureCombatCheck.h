#pragma once

#include "CoreMinimal.h"

/**
 * 에셋에서 모은 크리처 판정 데이터의 검사와 서버가 읽는 파일의 작성.
 *
 * 몬스터와 직업마다 캡슐 크기가, 기획 데이터의 액션마다 몽타주 섹션의 타이밍이 있는지 기획 데이터와 대조하고, 통과하면
 * 크리처 판정 데이터 JSON을 만든다. 에셋과 UObject를 모르므로 에셋을 연 쪽이 값을 넘긴다.
 */
class P1EDITOR_API FP1CreatureCombatCheck
{
public:
    /** 루트 모션 곡선의 표본 수(초당). 서버가 같은 간격으로 읽는다. */
    static constexpr int32 ROOT_MOTION_SAMPLE_RATE = 30;

    /** 몬스터 하나나 직업 하나. Id는 몬스터 번호나 직업 번호다. */
    struct FCreature
    {
        int32 Id = 0;
        /** 오류 메시지에 쓰는 출처(에셋 테이블의 행이나 플레이어 블루프린트). */
        FString Source;
        float CapsuleRadius = 0.f;
        float CapsuleHalfHeight = 0.f;
    };

    /** 콤보 입력 창 노티파이 스테이트 하나. 시간은 섹션 시작에서 잰 초다. */
    struct FComboWindow
    {
        float Start = 0.f;
        float End = 0.f;
    };

    /** 액션 에셋 테이블의 행 하나가 가리키는 몽타주 섹션. 시간은 섹션 시작에서 잰 트랙 시간(초)이다. Montage가 비어 있으면 몽타주 칸이 비었다. */
    struct FAction
    {
        int32 Id = 0;
        /** 오류 메시지에 쓰는 출처(에셋 테이블의 행). */
        FString Source;
        /** 몽타주의 오브젝트 경로. 오류 메시지와 산출물에 쓴다. */
        FString Montage;
        FString Section;
        bool bHasRootMotion = false;
        float RateScale = 1.f;
        float Length = 0.f;
        /** 타격 노티파이의 시점. 시간 순서다. */
        TArray<float> HitTimes;
        /** 섹션 안에서 시작하는 콤보 입력 창. 시작 순서다. */
        TArray<FComboWindow> ComboWindows;
        /** 1/ROOT_MOTION_SAMPLE_RATE초 간격으로 잰 누적 루트 모션 이동량(캐릭터 로컬, cm). 첫 표본은 0이고 마지막 표본은 섹션 끝이다. 루트 모션이 없으면 비어 있다. */
        TArray<FVector> RootMotion;
    };

    /** 기획 데이터의 액션 하나에서 추출이 대조하는 값. */
    struct FDesignAction
    {
        int32 HitCount = 0;
        bool bHasNextAction = false;
        /** 몬스터의 일반 공격 액션이다. */
        bool bUsedByMonster = false;
    };

    struct FInput
    {
        TArray<FCreature> Monsters;
        TArray<FCreature> Classes;
        TArray<FAction> Actions;
        /** 기획 데이터의 몬스터 번호. */
        TSet<int32> DesignMonsterIds;
        /** 액션 번호 → 기획 데이터의 액션. */
        TMap<int32, FDesignAction> DesignActions;
    };

    struct FResult
    {
        /** 오류 한 줄에 몬스터 번호, 직업 번호, 액션 번호 중 하나가 들어 있다. */
        TArray<FString> Errors;
        /** 크리처 판정 데이터 JSON. 오류가 하나라도 있으면 비어 있다. */
        FString File;

        int32 GetExitCode() const { return Errors.IsEmpty() ? 0 : 1; }
    };

    /**
     * 서버 기획 데이터의 액션 표와 몬스터 표 JSON에서 몬스터 번호와 액션의 대조 값을 읽어 Input에 채운다.
     * 형식이 틀리면 OutErrors에 사유를 더하고 false.
     */
    static bool ParseDesignData(const FString& ActionJson, const FString& MonsterJson, FInput& Input, TArray<FString>& OutErrors);

    /** 크리처와 액션을 함께 검사하고, 통과하면 산출물을 만든다. 기획 데이터와의 짝을 보려고 모든 행을 한 번에 받는다. */
    static FResult Run(const FInput& Input);
};
