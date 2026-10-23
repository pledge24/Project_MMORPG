#pragma once

/**
 * GameDB 계정 행의 DB 접근 창구.
 *
 * 이 월드에 들어온 계정의 게임 데이터(캐릭터 슬롯 수, 마지막 로그인 시각)를 기록한다.
 * 쿼리가 블로킹이므로 DB 큐의 잡 안에서만 부른다. DB 작업이 실패하면 DBError를 던진다.
 */
class AccountDAO
{
public:
    /**
     * 인증 티켓을 소비한 계정의 행을 만들거나 마지막 로그인 시각을 갱신하고, 그 계정의 캐릭터 슬롯 수를 돌려준다.
     * 새 행의 슬롯 수는 로비 표의 기본값이다.
     */
    static int32 RecordLogin(DBConnection& conn, int64 accountId);

    /** 계정 행의 캐릭터 슬롯 수를 읽는다. 행을 만들거나 로그인 시각을 바꾸지 않는다. 행이 없으면 DBError. */
    static int32 LoadCharacterSlotCount(DBConnection& conn, int64 accountId);
};
