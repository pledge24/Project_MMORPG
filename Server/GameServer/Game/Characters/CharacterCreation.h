#pragma once

/**
 * 클라이언트가 보낸 캐릭터 생성 요청을 검증한다.
 * 세션과 DB는 모른다.
 */
namespace CharacterCreation
{
    /** 캐릭터 이름 최대 길이(UTF-16 기준). */
    constexpr int32 MAX_NAME_LENGTH = 50;
    /** 캐릭터 슬롯 기본 개수*/
    constexpr int32 DEFAULT_CHARACTER_SLOT_COUNT = 4;

    /**
     * 통과하면 nullopt, 거절하면 화면에 보일 사유를 돌려준다.
     * 직업은 레벨 표가 있고 그 표가 비어 있지 않아야 만들 수 있다.
     */
    optional<string> Validate(const Protocol::CharacterOverview& character);
}
