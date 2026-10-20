-- 001: 초기 스키마 (계정, 캐릭터, 캐릭터 상태, 아이템, 재화).
-- 이미 적용된 DB에 다시 실행해도 아무것도 바꾸지 않는다. 테이블과 인덱스가 없을 때만 만든다.
-- 스키마를 바꿀 때는 이 파일을 고치지 않고 다음 번호의 파일(002_<이름>.sql)을 더한다.

-- 필터 인덱스(UX_Character_Name_Active)는 이 두 설정이 켜져 있어야 만들어진다. sqlcmd의 기본값은 QUOTED_IDENTIFIER OFF다.
SET ANSI_NULLS ON;
SET QUOTED_IDENTIFIER ON;

-- 게임 계정: 이 월드에 처음 접속할 때 만든다.
-- 인증 정보와 개인정보는 AccountDB가 맡고, 여기에는 게임 데이터만 둔다.
IF OBJECT_ID(N'dbo.Account', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Account
    (
        AccountId          BIGINT       NOT NULL, -- AccountDB가 발급한 계정 번호
        CharacterSlotCount TINYINT      NOT NULL, -- 처음 값은 서버가 기획 데이터(캐릭터 생성 표)에서 읽어 넣는다
        CreatedAt          DATETIME2(3) NOT NULL
            CONSTRAINT DF_Account_CreatedAt DEFAULT SYSUTCDATETIME(),
        LastLoginAt        DATETIME2(3) NULL,     -- UTC
        CONSTRAINT PK_Account PRIMARY KEY (AccountId)
    );
END;

-- 핵심 정보: 캐릭터 목록 화면에서 조회하거나 바뀌는 일이 드문 값
IF OBJECT_ID(N'dbo.Character', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.[Character]
    (
        CharacterId BIGINT       NOT NULL, -- 서버의 ID 발급기가 발급한다
        AccountId   BIGINT       NOT NULL,
        Name        NVARCHAR(32) NOT NULL,
        ClassId     INT          NOT NULL, -- 기획 데이터의 직업. 1: 전사
        Level       INT          NOT NULL  -- 누적 경험치로 다시 계산하는 값의 목록용 사본
            CONSTRAINT DF_Character_Level DEFAULT 1,
        CreatedAt   DATETIME2(3) NOT NULL
            CONSTRAINT DF_Character_CreatedAt DEFAULT SYSUTCDATETIME(),
        LastLoginAt DATETIME2(3) NULL,     -- UTC
        DeletedAt   DATETIME2(3) NULL,     -- NULL이면 삭제되지 않은 캐릭터다
        CONSTRAINT PK_Character PRIMARY KEY (CharacterId),
        CONSTRAINT FK_Character_Account FOREIGN KEY (AccountId)
            REFERENCES dbo.Account (AccountId),
        CONSTRAINT CK_Character_Level CHECK (Level >= 1)
    );
END;

-- 삭제되지 않은 캐릭터 사이에서만 이름 중복을 검사한다
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID(N'dbo.Character') AND name = N'UX_Character_Name_Active')
BEGIN
    CREATE UNIQUE NONCLUSTERED INDEX UX_Character_Name_Active
        ON dbo.[Character] (Name)
        WHERE DeletedAt IS NULL;
END;

-- 캐릭터 목록 화면 조회용 커버링 인덱스
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID(N'dbo.Character') AND name = N'IX_Character_AccountId')
BEGIN
    CREATE NONCLUSTERED INDEX IX_Character_AccountId
        ON dbo.[Character] (AccountId)
        INCLUDE (Name, ClassId, Level, LastLoginAt, DeletedAt);
END;

-- 자주 갱신되는 상태 값: 경험치, 위치, 현재 HP/MP
IF OBJECT_ID(N'dbo.CharacterState', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.CharacterState
    (
        CharacterId BIGINT       NOT NULL,
        Exp         BIGINT       NOT NULL  -- 총 누적 경험치
            CONSTRAINT DF_CharacterState_Exp DEFAULT 0,
        MapId       INT          NOT NULL, -- 기획 데이터의 맵 번호. 룸은 서버가 맵과 좌표로 고른다
        PosX        REAL         NOT NULL,
        PosY        REAL         NOT NULL,
        PosZ        REAL         NOT NULL,
        Direction   REAL         NOT NULL
            CONSTRAINT DF_CharacterState_Direction DEFAULT 0,
        CurrentHp   INT          NOT NULL,
        CurrentMp   INT          NOT NULL,
        UpdatedAt   DATETIME2(3) NOT NULL  -- 기본값은 INSERT에만 쓰인다. UPDATE에서는 서버가 넣는다
            CONSTRAINT DF_CharacterState_UpdatedAt DEFAULT SYSUTCDATETIME(),
        CONSTRAINT PK_CharacterState PRIMARY KEY (CharacterId),
        CONSTRAINT FK_CharacterState_Character FOREIGN KEY (CharacterId)
            REFERENCES dbo.[Character] (CharacterId),
        CONSTRAINT CK_CharacterState_Exp CHECK (Exp >= 0),
        CONSTRAINT CK_CharacterState_CurrentHp CHECK (CurrentHp >= 0),
        CONSTRAINT CK_CharacterState_CurrentMp CHECK (CurrentMp >= 0)
    );
END;

-- 모든 아이템이 함께 가지는 속성
-- 착용 중인 장비도 Container를 착용 장비로 두고 이 테이블에 둔다
-- 소모품과 기타 아이템은 세부 테이블 없이 이 테이블만 쓴다
IF OBJECT_ID(N'dbo.Item', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Item
    (
        ItemUid    BIGINT       NOT NULL, -- 서버의 ID 발급기가 발급하는 전역 고유 번호
        OwnerId    BIGINT       NOT NULL, -- 캐릭터 ID
        TemplateId INT          NOT NULL, -- 기획 데이터의 아이템 번호
        ItemType   TINYINT      NOT NULL, -- 1: 장비, 2: 소모품, 3: 기타. 인벤토리에서는 구역을 가른다
        Container  TINYINT      NOT NULL, -- 아이템을 담는 곳. 1: 인벤토리, 2: 착용 장비
        SlotIndex  SMALLINT     NOT NULL, -- 구역 안의 칸 번호. 착용 장비에서는 장착 부위 번호
        StackCount INT          NOT NULL
            CONSTRAINT DF_Item_StackCount DEFAULT 1,
        CreatedAt  DATETIME2(3) NOT NULL
            CONSTRAINT DF_Item_CreatedAt DEFAULT SYSUTCDATETIME(),
        CONSTRAINT PK_Item PRIMARY KEY (ItemUid),
        CONSTRAINT UQ_Item_OwnerId_Container_ItemType_SlotIndex UNIQUE (OwnerId, Container, ItemType, SlotIndex), -- 칸 하나에 아이템 하나
        CONSTRAINT UQ_Item_ItemUid_ItemType UNIQUE (ItemUid, ItemType), -- 세부 테이블의 복합 외래 키 대상
        CONSTRAINT FK_Item_Character FOREIGN KEY (OwnerId)
            REFERENCES dbo.[Character] (CharacterId),
        CONSTRAINT CK_Item_StackCount CHECK (StackCount >= 1),
        CONSTRAINT CK_Item_SlotIndex CHECK (SlotIndex >= 0)
    );
END;

-- 장비 아이템에만 있는 속성
IF OBJECT_ID(N'dbo.ItemGear', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.ItemGear
    (
        ItemUid      BIGINT  NOT NULL,
        ItemType     TINYINT NOT NULL
            CONSTRAINT DF_ItemGear_ItemType DEFAULT 1,
        EnhanceLevel TINYINT NOT NULL
            CONSTRAINT DF_ItemGear_EnhanceLevel DEFAULT 0,
        Durability   INT     NOT NULL,
        CONSTRAINT PK_ItemGear PRIMARY KEY (ItemUid),
        CONSTRAINT FK_ItemGear_Item FOREIGN KEY (ItemUid, ItemType)
            REFERENCES dbo.Item (ItemUid, ItemType) ON DELETE CASCADE,
        CONSTRAINT CK_ItemGear_ItemType CHECK (ItemType = 1),
        CONSTRAINT CK_ItemGear_Durability CHECK (Durability >= 0)
    );
END;

-- 장비의 랜덤 옵션처럼 개수가 정해지지 않은 속성
IF OBJECT_ID(N'dbo.ItemGearOption', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.ItemGearOption
    (
        ItemUid     BIGINT  NOT NULL,
        OptionSlot  TINYINT NOT NULL, -- 옵션 칸 번호
        OptionId    INT     NOT NULL, -- 기획 데이터의 옵션 번호
        OptionValue INT     NOT NULL,
        CONSTRAINT PK_ItemGearOption PRIMARY KEY (ItemUid, OptionSlot),
        CONSTRAINT FK_ItemGearOption_ItemGear FOREIGN KEY (ItemUid)
            REFERENCES dbo.ItemGear (ItemUid) ON DELETE CASCADE
    );
END;

-- 캐릭터 단위 재화: 재화 종류마다 행 하나
IF OBJECT_ID(N'dbo.CharacterCurrency', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.CharacterCurrency
    (
        CharacterId BIGINT       NOT NULL,
        CurrencyId  INT          NOT NULL, -- 기획 데이터(재화 표)의 재화 번호
        Amount      BIGINT       NOT NULL
            CONSTRAINT DF_CharacterCurrency_Amount DEFAULT 0,
        UpdatedAt   DATETIME2(3) NOT NULL  -- 기본값은 INSERT에만 쓰인다. UPDATE에서는 서버가 넣는다
            CONSTRAINT DF_CharacterCurrency_UpdatedAt DEFAULT SYSUTCDATETIME(),
        CONSTRAINT PK_CharacterCurrency PRIMARY KEY (CharacterId, CurrencyId),
        CONSTRAINT FK_CharacterCurrency_Character FOREIGN KEY (CharacterId)
            REFERENCES dbo.[Character] (CharacterId),
        CONSTRAINT CK_CharacterCurrency_Amount CHECK (Amount >= 0)
    );
END;
