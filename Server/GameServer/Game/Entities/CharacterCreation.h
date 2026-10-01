#pragma once

/*--------------------------------------------------------------
    CharacterCreation

    클라이언트가 보낸 캐릭터 생성 요청을 검증한다. 세션과 DB를 모른다.
---------------------------------------------------------------*/

namespace CharacterCreation
{
    // DB 열 character_name이 NVARCHAR(50)이다. 글자 수는 UTF-16 기준으로 센다.
    constexpr int32 MAX_NAME_LENGTH = 50;

    // 통과하면 nullopt, 거절하면 화면에 보일 사유를 돌려준다.
    // 직업은 레벨 표가 있고 그 표가 비어 있지 않아야 만들 수 있다.
    optional<string> Validate(const Protocol::CharacterOverview& character);
}
