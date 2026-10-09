#include "Core/pch.h"
#include <gtest/gtest.h>
#include "DB/DAOCommon.h"

/*--------------------------------------------------------------
    DB 연결 가드 테스트

    연결 풀이 비어 있으면 Pop은 nullptr를 돌려준다. 가드가 그 값을 그대로 넘기면 DAO가 널 연결을
    역참조해 게임 서버가 죽는다(TD-004). 빌리지 못하면 DBError로 알려 잡이 실패 응답을 보내게 한다.

    픽스처 결합도: 전역 연결 풀(GDBConnectionPool)을 쓴다. 테스트 실행 파일은 DB에 연결하지 않으므로
    풀은 늘 비어 있다.
---------------------------------------------------------------*/

TEST(DBConnectionGuardTest, EmptyPoolIsDBError)
{
    ASSERT_EQ(GDBConnectionPool->Pop(), nullptr) << "이 테스트는 비어 있는 풀을 전제로 한다";

    EXPECT_THROW(DBConnectionGuard guard, DBError) << "널 연결을 넘기면 DAO가 역참조해 서버가 죽는다";
}
