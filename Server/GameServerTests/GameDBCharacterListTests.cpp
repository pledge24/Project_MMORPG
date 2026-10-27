#include "Core/pch.h"
#include <gtest/gtest.h>
#include "GameDBTestFixture.h"
#include "Lobby/AccountDAO.h"
#include "Lobby/CharacterListDAO.h"
#include "DB/DAOCommon.h"
#include "Lobby/Data/LobbyData.h"

/*--------------------------------------------------------------
    계정과 캐릭터 목록의 SQL 확인 (실제 DB)

    로그인의 계정 행, 캐릭터 목록, 생성, 삭제를 GameDB_Test에서 실제 SQL로 돌린다. 생성은 캐릭터 행만 넣는다.
    가짜 연결은 바인딩만 본다. 슬롯 한도의 COUNT, 이름 중복의 고유 인덱스 위반(2601), 삭제한 캐릭터의
    이름 재사용은 스키마와 SQL이 함께 정하므로 여기서만 확인된다.

    테스트마다 다른 계정 번호를 쓰고, 그 계정의 행만 넣고 지운다. 이름도 테스트마다 다르게 짓는다.
    이름의 고유 인덱스는 삭제되지 않은 모든 캐릭터에 걸리므로, 이름이 겹치면 다른 테스트의 행에 결과가 흔들린다.

    기본으로 꺼져 있다. 접속 문자열과 실행 필터는 GameDBTestFixture에 있다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 DEFAULT_SLOT_COUNT = 2;
}

class DISABLED_GameDBCharacterListTest : public GameDBTestFixture
{
protected:
    void SetUp() override
    {
        GameDBTestFixture::SetUp();
        if (HasFatalFailure())
            return;

        LobbyDesignData::LobbyConfig config;
        config.DefaultCharacterSlotCount = DEFAULT_SLOT_COUNT;
        config.CreatableClasses = { Protocol::CHARACTER_CLASS_WARRIOR };
        LobbyData::Install(std::move(config));
    }

    void TearDown() override
    {
        LobbyData::Install(LobbyDesignData::LobbyConfig());
        GameDBTestFixture::TearDown();
    }

    // 계정과 그 계정의 캐릭터 행을 테스트가 끝날 때 지운다. 외래 키 때문에 자식 행부터 지운다(정리는 넣은 역순으로 돈다).
    void CleanupAccount(int64 accountId)
    {
        const wstring id = to_wstring(accountId);
        const wstring characters = L"(SELECT CharacterId FROM dbo.[Character] WHERE AccountId = " + id + L")";
        AddCleanup(L"DELETE FROM dbo.Account WHERE AccountId = " + id);
        AddCleanup(L"DELETE FROM dbo.[Character] WHERE AccountId = " + id);
        AddCleanup(L"DELETE FROM dbo.CharacterState WHERE CharacterId IN " + characters);
        AddCleanup(L"DELETE FROM dbo.CharacterCurrency WHERE CharacterId IN " + characters);
    }

    // 캐릭터를 하나 만든다. 계정 행은 테스트가 RecordLogin으로 먼저 만든다.
    CreateCharacterResult Create(int64 accountId, int64 characterId, const string& name)
    {
        return CharacterListDAO::CreateCharacter(Connection(), accountId, characterId, name, Protocol::CHARACTER_CLASS_WARRIOR);
    }

    std::optional<int64> CountRows(const wstring& table, const wstring& where)
    {
        return QueryInt64(L"SELECT COUNT(*) FROM " + table + L" WHERE " + where);
    }
};

TEST_F(DISABLED_GameDBCharacterListTest, LoginCreatesAccountRowWithDefaultSlotCount)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'101;
    CleanupAccount(ACCOUNT_ID);

    EXPECT_EQ(AccountDAO::RecordLogin(Connection(), ACCOUNT_ID), DEFAULT_SLOT_COUNT);

    const wstring where = L"AccountId = " + to_wstring(ACCOUNT_ID);
    EXPECT_EQ(QueryInt64(L"SELECT CharacterSlotCount FROM dbo.Account WHERE " + where), DEFAULT_SLOT_COUNT);
    EXPECT_EQ(CountRows(L"dbo.Account", where + L" AND LastLoginAt IS NOT NULL"), 1);
}

TEST_F(DISABLED_GameDBCharacterListTest, LoginUpdatesLastLoginAndKeepsSlotCount)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'102;
    CleanupAccount(ACCOUNT_ID);
    const wstring where = L"AccountId = " + to_wstring(ACCOUNT_ID);

    AccountDAO::RecordLogin(Connection(), ACCOUNT_ID);
    ASSERT_TRUE(Execute(L"UPDATE dbo.Account SET CharacterSlotCount = 6, LastLoginAt = '2000-01-01' WHERE " + where));

    EXPECT_EQ(AccountDAO::RecordLogin(Connection(), ACCOUNT_ID), 6) << "이미 있던 계정은 기획 데이터의 기본값이 아니라 그 행의 슬롯 수를 쓴다";
    EXPECT_EQ(CountRows(L"dbo.Account", where), 1) << "다시 로그인해도 행을 하나 더 만들지 않는다";
    EXPECT_EQ(CountRows(L"dbo.Account", where + L" AND LastLoginAt > '2001-01-01'"), 1) << "다시 로그인하면 마지막 로그인 시각을 갱신한다";
}

// 로비 복귀는 로그인이 아니다. 슬롯 수만 읽고 마지막 로그인 시각은 그대로 둔다.
TEST_F(DISABLED_GameDBCharacterListTest, LoadSlotCountKeepsLastLogin)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'115;
    CleanupAccount(ACCOUNT_ID);
    const wstring where = L"AccountId = " + to_wstring(ACCOUNT_ID);

    AccountDAO::RecordLogin(Connection(), ACCOUNT_ID);
    ASSERT_TRUE(Execute(L"UPDATE dbo.Account SET CharacterSlotCount = 5, LastLoginAt = '2000-01-01' WHERE " + where));

    EXPECT_EQ(AccountDAO::LoadCharacterSlotCount(Connection(), ACCOUNT_ID), 5);
    EXPECT_EQ(CountRows(L"dbo.Account", where + L" AND LastLoginAt = '2000-01-01'"), 1);
}

TEST_F(DISABLED_GameDBCharacterListTest, CreateInsertsOnlyCharacterRow)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'103;
    constexpr int64 CHARACTER_ID = 9'000'000'103'001;
    CleanupAccount(ACCOUNT_ID);
    AccountDAO::RecordLogin(Connection(), ACCOUNT_ID);

    const CreateCharacterResult result = Create(ACCOUNT_ID, CHARACTER_ID, "DB생성_캐릭터행");

    ASSERT_FALSE(result.rejection.has_value());
    EXPECT_EQ(result.characterId, CHARACTER_ID);

    const wstring where = L"CharacterId = " + to_wstring(CHARACTER_ID);
    EXPECT_EQ(CountRows(L"dbo.[Character]", where + L" AND AccountId = " + to_wstring(ACCOUNT_ID)
        + L" AND ClassId = 1 AND Level = 1 AND DeletedAt IS NULL"), 1);
    EXPECT_EQ(CountRows(L"dbo.CharacterState", where), 0) << "진행 행이 없는 캐릭터는 아직 게임에 입장한 적이 없는 캐릭터다";
    EXPECT_EQ(CountRows(L"dbo.CharacterCurrency", where), 0) << "초기 진행 행은 첫 게임 입장이 만든다";
}

TEST_F(DISABLED_GameDBCharacterListTest, DuplicateNameIsRejectionAndLeavesNoRows)
{
    constexpr int64 OWNER_ID = 9'000'000'000'104;
    constexpr int64 OTHER_ID = 9'000'000'000'105;
    constexpr int64 OTHER_CHARACTER_ID = 9'000'000'105'001;
    CleanupAccount(OWNER_ID);
    CleanupAccount(OTHER_ID);
    AccountDAO::RecordLogin(Connection(), OWNER_ID);
    AccountDAO::RecordLogin(Connection(), OTHER_ID);
    ASSERT_FALSE(Create(OWNER_ID, 9'000'000'104'001, "DB생성_중복").rejection.has_value());

    const CreateCharacterResult result = Create(OTHER_ID, OTHER_CHARACTER_ID, "DB생성_중복");

    ASSERT_TRUE(result.rejection.has_value()) << "이름 중복은 다른 계정의 캐릭터와도 판정한다";
    EXPECT_EQ(result.rejection.value(), CharacterRejection::DUPLICATE_NAME);
    const wstring where = L"CharacterId = " + to_wstring(OTHER_CHARACTER_ID);
    EXPECT_EQ(CountRows(L"dbo.[Character]", where), 0) << "거절된 생성은 캐릭터 행을 남기지 않는다";
}

TEST_F(DISABLED_GameDBCharacterListTest, SlotLimitCountsOnlyActiveCharacters)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'106;
    CleanupAccount(ACCOUNT_ID);
    AccountDAO::RecordLogin(Connection(), ACCOUNT_ID);
    ASSERT_FALSE(Create(ACCOUNT_ID, 9'000'000'106'001, "DB슬롯_하나").rejection.has_value());
    ASSERT_FALSE(Create(ACCOUNT_ID, 9'000'000'106'002, "DB슬롯_둘").rejection.has_value());

    const CreateCharacterResult full = Create(ACCOUNT_ID, 9'000'000'106'003, "DB슬롯_셋");
    ASSERT_TRUE(full.rejection.has_value()) << "슬롯 수(2)만큼 만든 계정은 더 만들 수 없다";
    EXPECT_EQ(full.rejection.value(), CharacterRejection::NO_EMPTY_SLOT);

    ASSERT_TRUE(CharacterListDAO::DeleteCharacter(Connection(), ACCOUNT_ID, 9'000'000'106'001));

    EXPECT_FALSE(Create(ACCOUNT_ID, 9'000'000'106'004, "DB슬롯_셋").rejection.has_value())
        << "삭제한 캐릭터는 슬롯 한도에 세지 않는다";
}

TEST_F(DISABLED_GameDBCharacterListTest, DeletedCharacterNameCanBeReused)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'107;
    constexpr int64 DELETED_ID = 9'000'000'107'001;
    CleanupAccount(ACCOUNT_ID);
    AccountDAO::RecordLogin(Connection(), ACCOUNT_ID);
    ASSERT_FALSE(Create(ACCOUNT_ID, DELETED_ID, "DB재사용").rejection.has_value());
    ASSERT_TRUE(CharacterListDAO::DeleteCharacter(Connection(), ACCOUNT_ID, DELETED_ID));

    EXPECT_FALSE(Create(ACCOUNT_ID, 9'000'000'107'002, "DB재사용").rejection.has_value())
        << "이름 중복은 삭제되지 않은 캐릭터 사이에서만 판정한다";
}

TEST_F(DISABLED_GameDBCharacterListTest, DeleteIsSoftAndKeepsStateAndCurrency)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'108;
    constexpr int64 CHARACTER_ID = 9'000'000'108'001;
    CleanupAccount(ACCOUNT_ID);
    AccountDAO::RecordLogin(Connection(), ACCOUNT_ID);
    ASSERT_FALSE(Create(ACCOUNT_ID, CHARACTER_ID, "DB삭제_소프트").rejection.has_value());
    // 게임에 입장한 적이 있는 캐릭터처럼 진행 행을 넣는다.
    const wstring id = to_wstring(CHARACTER_ID);
    ASSERT_TRUE(Execute(L"INSERT INTO dbo.CharacterState (CharacterId, MapId, PosX, PosY, PosZ, CurrentHp, CurrentMp) VALUES ("
        + id + L", 1111, 0, 0, 0, 1, 1)"));
    ASSERT_TRUE(Execute(L"INSERT INTO dbo.CharacterCurrency (CharacterId, CurrencyId, Amount) VALUES (" + id + L", 1, 10)"));

    EXPECT_TRUE(CharacterListDAO::DeleteCharacter(Connection(), ACCOUNT_ID, CHARACTER_ID));
    EXPECT_FALSE(CharacterListDAO::DeleteCharacter(Connection(), ACCOUNT_ID, CHARACTER_ID)) << "이미 지운 캐릭터는 다시 지우지 않는다";

    const wstring where = L"CharacterId = " + to_wstring(CHARACTER_ID);
    EXPECT_EQ(CountRows(L"dbo.[Character]", where + L" AND DeletedAt IS NOT NULL"), 1) << "삭제는 행을 지우지 않고 삭제 시각만 적는다";
    EXPECT_EQ(CountRows(L"dbo.CharacterState", where), 1);
    EXPECT_EQ(CountRows(L"dbo.CharacterCurrency", where), 1) << "재화 행은 남긴다";
}

TEST_F(DISABLED_GameDBCharacterListTest, DeleteOfOtherAccountsCharacterChangesNothing)
{
    constexpr int64 OWNER_ID = 9'000'000'000'109;
    constexpr int64 OTHER_ID = 9'000'000'000'110;
    constexpr int64 CHARACTER_ID = 9'000'000'109'001;
    CleanupAccount(OWNER_ID);
    AccountDAO::RecordLogin(Connection(), OWNER_ID);
    ASSERT_FALSE(Create(OWNER_ID, CHARACTER_ID, "DB삭제_남의것").rejection.has_value());

    EXPECT_FALSE(CharacterListDAO::DeleteCharacter(Connection(), OTHER_ID, CHARACTER_ID));
    EXPECT_EQ(CountRows(L"dbo.[Character]", L"CharacterId = " + to_wstring(CHARACTER_ID) + L" AND DeletedAt IS NULL"), 1);
}

TEST_F(DISABLED_GameDBCharacterListTest, ListExcludesDeletedCharacters)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'111;
    constexpr int64 KEPT_ID = 9'000'000'111'001;
    constexpr int64 DELETED_ID = 9'000'000'111'002;
    CleanupAccount(ACCOUNT_ID);
    AccountDAO::RecordLogin(Connection(), ACCOUNT_ID);
    ASSERT_FALSE(Create(ACCOUNT_ID, KEPT_ID, "DB목록_남김").rejection.has_value());
    ASSERT_FALSE(Create(ACCOUNT_ID, DELETED_ID, "DB목록_지움").rejection.has_value());
    ASSERT_TRUE(CharacterListDAO::DeleteCharacter(Connection(), ACCOUNT_ID, DELETED_ID));
    ASSERT_TRUE(Execute(L"UPDATE dbo.[Character] SET Level = 7 WHERE CharacterId = " + to_wstring(KEPT_ID)));

    vector<CharacterListRow> characters;
    CharacterListDAO::LoadCharacterList(Connection(), ACCOUNT_ID, OUT characters);

    ASSERT_EQ(characters.size(), 1u);
    EXPECT_EQ(characters[0].characterId, KEPT_ID);
    EXPECT_EQ(characters[0].name, "DB목록_남김");
    EXPECT_EQ(characters[0].classId, Protocol::CHARACTER_CLASS_WARRIOR);
    EXPECT_EQ(characters[0].level, 7) << "목록의 레벨은 DB에 저장된 사본이다";
}

TEST_F(DISABLED_GameDBCharacterListTest, CreateWithoutAccountRowIsDBError)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'112;
    CleanupAccount(ACCOUNT_ID);

    EXPECT_THROW(Create(ACCOUNT_ID, 9'000'000'112'001, "DB계정없음"), DBError)
        << "슬롯 수의 원천인 계정 행이 없으면 거절이 아니라 서버 오류다";
}

TEST_F(DISABLED_GameDBCharacterListTest, OwnershipIsOnlyForAccountsActiveCharacter)
{
    constexpr int64 OWNER_ID = 9'000'000'000'113;
    constexpr int64 OTHER_ID = 9'000'000'000'114;
    constexpr int64 ACTIVE_ID = 9'000'000'113'001;
    constexpr int64 DELETED_ID = 9'000'000'113'002;
    CleanupAccount(OWNER_ID);
    AccountDAO::RecordLogin(Connection(), OWNER_ID);
    ASSERT_FALSE(Create(OWNER_ID, ACTIVE_ID, "DB소유_남김").rejection.has_value());
    ASSERT_FALSE(Create(OWNER_ID, DELETED_ID, "DB소유_지움").rejection.has_value());
    ASSERT_TRUE(CharacterListDAO::DeleteCharacter(Connection(), OWNER_ID, DELETED_ID));

    EXPECT_TRUE(CharacterListDAO::OwnsCharacter(Connection(), OWNER_ID, ACTIVE_ID));
    EXPECT_FALSE(CharacterListDAO::OwnsCharacter(Connection(), OTHER_ID, ACTIVE_ID)) << "다른 계정의 캐릭터로는 입장하지 못한다";
    EXPECT_FALSE(CharacterListDAO::OwnsCharacter(Connection(), OWNER_ID, DELETED_ID)) << "삭제한 캐릭터로는 입장하지 못한다";
}
