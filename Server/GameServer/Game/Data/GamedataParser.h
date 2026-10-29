#pragma once
#include "Game/Data/Templates.h"

/** 파싱만 마친 기획표 문서. DataBuilder가 생성한 파일 하나가 문서 하나다. 표는 행 배열이고 설정은 객체 하나다. */
struct GamedataDocuments
{
    Json maps;
    /** 포털은 지역 아래 중첩되어 있다. */
    Json areas;
    Json monsters;
    Json dropGroups;
    /** 스폰 항목은 그룹 아래 중첩되어 있다. */
    Json spawnGroups;
    Json currencies;
    Json items;
    Json itemEquipStats;
    Json itemConsumeEffects;
    Json levels;
    Json classLevelStats;
    /** 설정 워크북의 초기 진행 시트(Progress.json). */
    Json initialProgress;
    /** 파일 이름 → 추출한 맵 하나의 배치 데이터. 오류 메시지가 파일 이름을 쓴다. */
    map<string, Json> placements;
    /** 추출한 크리처 판정 데이터(CreatureCombat.json). */
    Json creatureCombat;
};

/**
 * 기획표 문서를 검증해 템플릿으로 바꾸는 자유 함수.
 * 필드가 없거나 타입이 틀린 행, 표끼리 맞지 않는 값을 부팅 단계에서 걸러 낸다.
 */
namespace GamedataParser
{
    /**
     * 통과하면 nullopt, 실패하면 처음 걸린 오류를 "파일 N번째 행: 사유" 형식으로 돌려준다.
     * 실패하면 tables에 일부만 채워졌을 수 있으므로 쓰지 않는다.
     */
    optional<string> Parse(const GamedataDocuments& documents, OUT GamedataTables& tables);
}
