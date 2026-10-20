#include "Core/pch.h"
#include <gtest/gtest.h>
#include "GameDBTestFixture.h"
#include "DB/IssuedIdDAO.h"
#include "Utils/SnowflakeIdGenerator.h"

/*--------------------------------------------------------------
    발급한 ID 조회의 SQL 확인 (실제 DB)

    부팅할 때 이 노드가 발급한 ID의 최댓값을 캐릭터와 아이템 두 테이블에서 읽는다. 다른 노드의 ID까지 세면
    발급기가 다른 노드의 시각을 따라가 쓸데없이 앞서고, 한 테이블만 보면 새 ID가 다른 테이블의 행과 겹친다.

    이 테스트의 ID는 노드 1022와 1023, 기준 시각에서 2^40밀리초(약 35년) 뒤의 시각으로 만든다.
    개발 중에 생기는 다른 행보다 늘 커서, 같은 노드의 다른 행이 있어도 최댓값이 이 테스트의 값으로 정해진다.

    기본으로 꺼져 있다. 접속 문자열과 실행 필터는 GameDBTestFixture에 있다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'201;
    constexpr int64 FAR_FUTURE_UNIX_MS = SnowflakeIdGenerator::EPOCH_UNIX_MS + (int64{ 1 } << 40);

    // 정한 시각에서 count개를 발급하고 마지막 ID를 돌려준다.
    int64 IssueAt(int32 nodeId, int64 unixMs, int32 count)
    {
        SnowflakeIdGenerator generator(nodeId, [unixMs]() { return unixMs; });
        int64 id = 0;
        for (int32 i = 0; i < count; i++)
            id = generator.Next();
        return id;
    }
}

class DISABLED_GameDBIssuedIdTest : public GameDBTestFixture
{
protected:
    void InsertCharacter(int64 characterId)
    {
        const wstring id = to_wstring(characterId);
        ASSERT_TRUE(Execute(L"INSERT INTO dbo.[Character] (CharacterId, AccountId, Name, ClassId) VALUES ("
            + id + L", " + to_wstring(ACCOUNT_ID) + L", N'DB발급_" + id + L"', 1)"));
    }

    void InsertItem(int64 itemUid, int64 ownerId, int32 slotIndex)
    {
        ASSERT_TRUE(Execute(L"INSERT INTO dbo.Item (ItemUid, OwnerId, TemplateId, ItemType, Container, SlotIndex) VALUES ("
            + to_wstring(itemUid) + L", " + to_wstring(ownerId) + L", 1, 2, 1, " + to_wstring(slotIndex) + L")"));
    }
};

TEST_F(DISABLED_GameDBIssuedIdTest, LastIssuedIdIsMaxOfThisNodeAcrossCharacterAndItem)
{
    const wstring account = to_wstring(ACCOUNT_ID);
    AddCleanup(L"DELETE FROM dbo.Account WHERE AccountId = " + account);
    AddCleanup(L"DELETE FROM dbo.[Character] WHERE AccountId = " + account);
    AddCleanup(L"DELETE FROM dbo.Item WHERE OwnerId IN (SELECT CharacterId FROM dbo.[Character] WHERE AccountId = " + account + L")");
    ASSERT_TRUE(Execute(L"INSERT INTO dbo.Account (AccountId, CharacterSlotCount) VALUES (" + account + L", 4)"));

    // 노드 1023: 캐릭터 ID보다 아이템 UID가 나중에 발급됐다. 노드 1022: 그보다 더 나중에 발급됐다.
    const int64 characterId = IssueAt(1023, FAR_FUTURE_UNIX_MS, 1);
    const int64 lastItemUid = IssueAt(1023, FAR_FUTURE_UNIX_MS, 3);
    const int64 otherNodeId = IssueAt(1022, FAR_FUTURE_UNIX_MS + 1000, 1);
    InsertCharacter(characterId);
    InsertCharacter(otherNodeId);
    InsertItem(IssueAt(1023, FAR_FUTURE_UNIX_MS, 2), characterId, 0);
    InsertItem(lastItemUid, characterId, 1);
    InsertItem(IssueAt(1022, FAR_FUTURE_UNIX_MS + 2000, 1), otherNodeId, 0);
    ASSERT_FALSE(HasFatalFailure());

    EXPECT_EQ(IssuedIdDAO::GetLastIssuedId(Connection(), 1023), lastItemUid) << "아이템 테이블까지 보고, 다른 노드의 ID는 세지 않는다";
    EXPECT_EQ(IssuedIdDAO::GetLastIssuedId(Connection(), 1022), IssueAt(1022, FAR_FUTURE_UNIX_MS + 2000, 1));
}

TEST_F(DISABLED_GameDBIssuedIdTest, NodeWithoutIssuedIdReturnsZero)
{
    // 노드 1021은 이 테스트도 개발 서버(Debug 기본 노드 0)도 쓰지 않는다.
    EXPECT_EQ(IssuedIdDAO::GetLastIssuedId(Connection(), 1021), 0);
}
