#pragma once
#include "Protocol/Entity.pb.h"
#include "Utils/Utils.h"

/*--------------------------------------------------------------
    크리처 판정 데이터

    몬스터와 직업, 액션의 에셋에서 추출한 판정 정보다. 몬스터 번호, 직업 번호, 액션 번호로 기획 데이터와 이어진다.
    캡슐 크기와 액션 몽타주 섹션의 타이밍, 루트 모션이 있고, 수치와 규칙은 기획 데이터 표에 있다.
---------------------------------------------------------------*/

/** 크리처 판정 데이터 파일. 배치 데이터와 같은 추출 산출물 폴더에 있다. */
constexpr string_view CREATURE_COMBAT_FILE = "CreatureCombat.json";

/** 몬스터 하나나 직업 하나의 판정 데이터. */
struct CreatureCombat
{
    /** cm. */
    float capsuleRadius = 0.f;
    /** cm. */
    float capsuleHalfHeight = 0.f;
};

/** 콤보의 다음 단계 입력을 받는 구간. 섹션 시작에서 잰 초이고, 섹션 안에서 시작해 섹션 끝 전에 끝난다(추출이 검사한다). */
struct ComboWindow
{
    float start = 0.f;
    float end = 0.f;
};

/** 액션 하나가 재생하는 몽타주 섹션의 타이밍. 시간은 섹션 시작에서 잰 초다. */
struct ActionTiming
{
    /** 몽타주의 에셋 경로와 섹션 이름. 로그에만 쓴다. */
    string montage;
    string section;
    float length = 0.f;
    /** 타격 순번 순서의 타격 시점. 수가 액션의 타격 수와 같다(기동할 때 대조한다). */
    vector<float> hitTimes;
    /** 다음 액션이 있는 액션에만 있다. */
    optional<ComboWindow> comboWindow;
    /** 1/GamedataTables::rootMotionSampleRate초 간격의 누적 루트 모션(캐릭터 로컬, cm). 처음은 0, 끝은 섹션 끝이고 없으면 비어 있다. */
    vector<vector3D> rootMotion;
};
