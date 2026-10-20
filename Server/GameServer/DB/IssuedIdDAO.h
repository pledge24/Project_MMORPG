#pragma once

/**
 * 서버가 발급한 ID의 DB 조회 창구.
 *
 * 캐릭터 ID와 아이템 UID는 한 Snowflake 발급기가 발급한다. 부팅할 때 이 노드가 이미 발급한 ID를 읽어
 * 발급기가 그 뒤에서 이어 가게 한다. DB 작업이 실패하면 DBError를 던진다.
 */
class IssuedIdDAO
{
public:
    /** 캐릭터와 아이템 테이블에서 nodeId가 발급한 ID의 최댓값. 발급한 적이 없으면 0. */
    static int64 GetLastIssuedId(DBConnection& conn, int32 nodeId);
};
