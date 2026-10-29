#pragma once
#include "Protocol/Entity.pb.h"
#include "Utils/Utils.h"

/*--------------------------------------------------------------
    크리처 판정 데이터

    몬스터와 직업의 에셋에서 추출한 판정 정보다. 몬스터 번호와 직업 번호로 기획 데이터와 이어진다.
    캡슐 크기와 공격 몽타주의 타이밍, 루트 모션이 있고, 수치와 규칙은 기획 데이터 표에 있다.
---------------------------------------------------------------*/

/** 크리처 판정 데이터 파일. 배치 데이터와 같은 추출 산출물 폴더에 있다. */
constexpr string_view CREATURE_COMBAT_FILE = "CreatureCombat.json";

/** 공격 몽타주의 섹션 하나. 시간은 섹션 시작에서 잰 초다. */
struct AttackSectionCombat
{
    string name;
    float length = 0.f;
    /** 공격이 맞는 시점. 시간 순서이고 하나 이상이다(기동할 때 대조한다). */
    vector<float> hitTimes;
    /** 1/GamedataTables::rootMotionSampleRate초 간격의 누적 루트 모션(캐릭터 로컬, cm). 처음은 0, 끝은 섹션 끝이고 없으면 비어 있다. */
    vector<vector3D> rootMotion;
};

/** 콤보 순번 하나의 공격 몽타주. */
struct AttackCombat
{
    /** 몽타주의 에셋 경로. 로그에만 쓴다. */
    string montage;
    /** 하나 이상이다(기동할 때 대조한다). */
    vector<AttackSectionCombat> sections;
};

/** 몬스터 하나나 직업 하나의 판정 데이터. */
struct CreatureCombat
{
    /** cm. */
    float capsuleRadius = 0.f;
    /** cm. */
    float capsuleHalfHeight = 0.f;
    /** 콤보 순번 1부터의 공격 몽타주. 순번 0인 공격은 첫 몽타주다. 하나 이상이다(기동할 때 대조한다). */
    vector<AttackCombat> attacks;
};
