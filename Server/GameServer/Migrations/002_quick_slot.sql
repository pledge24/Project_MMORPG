-- 002: 퀵슬롯.
-- 이미 적용된 DB에 다시 실행해도 아무것도 바꾸지 않는다. 테이블이 없을 때만 만든다.

-- 캐릭터가 퀵슬롯에 등록한 대상. 아이템을 담지 않고 아이템 정의를 가리킨다
-- 빈 퀵슬롯은 행을 두지 않는다. 접속 종료 때 서버가 등록 전체로 MERGE한다
-- 캐릭터는 DeletedAt으로 삭제를 표시하므로 dbo.Item처럼 연쇄 삭제를 걸지 않는다
IF OBJECT_ID(N'dbo.QuickSlot', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.QuickSlot
    (
        CharacterId BIGINT  NOT NULL,
        SlotIndex   TINYINT NOT NULL, -- 0부터 퀵슬롯 개수 미만
        RefType     TINYINT NOT NULL, -- 등록한 대상의 종류. 1: 아이템 정의. 스킬을 등록하게 되면 2를 쓴다
        RefId       INT     NOT NULL, -- 기획 데이터의 아이템 번호. 스킬이면 스킬 번호
        CONSTRAINT PK_QuickSlot PRIMARY KEY (CharacterId, SlotIndex),
        CONSTRAINT FK_QuickSlot_Character FOREIGN KEY (CharacterId)
            REFERENCES dbo.[Character] (CharacterId),
        CONSTRAINT CK_QuickSlot_RefType CHECK (RefType >= 1)
    );
END;
