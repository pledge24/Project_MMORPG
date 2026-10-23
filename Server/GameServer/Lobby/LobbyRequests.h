#pragma once

/**
 * 로비 요청의 처리.
 *
 * 로비 접속, 로비 복귀, 캐릭터 생성, 삭제, 선택을 DB 접근 창구에 넘기고, 그 결과를 로비 결과 값을 실은 응답으로 바꾼다.
 * 모든 함수가 블로킹 쿼리를 부르므로 DB 큐의 잡 안에서만 부른다. DB 작업이 실패하면 DBError를 던진다.
 */
namespace LobbyRequests
{
    /** 인증 티켓을 소비한 계정의 행을 만들거나 갱신하고, 그 계정의 캐릭터 목록을 돌려준다. */
    Protocol::S_CHARACTER_LIST ConnectLobby(DBConnection& conn, int64 accountId);

    /** 로비 복귀한 계정의 캐릭터 목록을 돌려준다. 계정 행은 바꾸지 않는다. */
    Protocol::S_CHARACTER_LIST ReturnToLobby(DBConnection& conn, int64 accountId);

    /**
     * characterId로 캐릭터를 만든다. 이름과 직업은 CharacterCreation::Validate를 통과한 값만 넘긴다.
     * 빈 슬롯이 없거나 이름이 겹치면 그 결과 값을 담는다.
     */
    Protocol::S_CREATE_CHARACTER CreateCharacter(DBConnection& conn, int64 accountId, int64 characterId, const Protocol::C_CREATE_CHARACTER& pkt);

    /** characterId는 클라이언트 값이어도 된다. */
    Protocol::S_DELETE_CHARACTER DeleteCharacter(DBConnection& conn, int64 accountId, int64 characterId);

    /** 캐릭터 선택의 소유 확인. characterId는 클라이언트 값이어도 된다. 이 계정의 캐릭터면 SUCCESS. */
    Protocol::LobbyResult SelectCharacter(DBConnection& conn, int64 accountId, int64 characterId);
}
