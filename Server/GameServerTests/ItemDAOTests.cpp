#include "Core/pch.h"
#include <gtest/gtest.h>
#include "FakeDBConnection.h"
#include "DB/ItemDAO.h"

/*--------------------------------------------------------------
    아이템 DAO 테스트

    item_uid는 BIGINT이고 GNextItemUID도 int64다. 최댓값을 int32로 읽으면 범위를 넘는 순간 값이
    잘려, 새로 발급한 UID가 기존 아이템과 겹친다(TD-034).

    픽스처 결합도: FakeDBConnection과 전역 GNextItemUID를 쓴다. 테스트가 끝나면 GNextItemUID를 되돌린다.
---------------------------------------------------------------*/

class ItemDAOTest : public ::testing::Test
{
protected:
    void SetUp() override { savedNextItemUID = GNextItemUID.load(); }
    void TearDown() override { GNextItemUID = savedNextItemUID; }

    int64 savedNextItemUID = 0;
};

TEST_F(ItemDAOTest, MaxItemUIDBeyondInt32IsReadWhole)
{
    constexpr int64 MAX_ITEM_UID = 5'000'000'000;
    FakeDBConnection conn;
    conn.QueueResult({ { MAX_ITEM_UID } });

    ItemDAO::GetMaxItemUID(conn);

    EXPECT_EQ(GNextItemUID.load(), MAX_ITEM_UID + 1) << "최댓값이 잘리면 새 UID가 기존 아이템과 겹친다";
}
