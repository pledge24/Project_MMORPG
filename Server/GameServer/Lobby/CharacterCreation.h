#pragma once

/**
 * 캐릭터 생성 규칙.
 *
 * 클라이언트가 보낸 캐릭터 생성 요청이 규칙에 맞는지 판정한다.
 */
namespace CharacterCreation
{
    /** 캐릭터 이름 최대 길이(UTF-16 기준). DB 열(Character.Name, NVARCHAR(32))의 크기와 같아야 한다. */
    constexpr int32 MAX_NAME_LENGTH = 32;

    /** 클라이언트가 보낸 이름과 직업을 검사한다. 통과하면 nullopt, 거절하면 그 결과 값. */
    optional<Protocol::LobbyResult> Validate(const string& name, Protocol::CharacterClass characterClass);
}
