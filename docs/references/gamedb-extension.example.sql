-- 게임 계정: 이 월드에 처음 접속할 때 생성
-- 인증 정보와 개인정보는 AccountDB에서 관리하고, 여기에는 게임 데이터만 저장
CREATE TABLE Account
(
    AccountId          BIGINT       NOT NULL, -- AccountDB가 발급한 계정 ID
    CharacterSlotCount TINYINT      NOT NULL
        CONSTRAINT DF_Account_CharacterSlotCount DEFAULT 4,
    CreatedAt          DATETIME2(3) NOT NULL
        CONSTRAINT DF_Account_CreatedAt DEFAULT SYSUTCDATETIME(),
    LastLoginAt        DATETIME2(3) NULL,
    CONSTRAINT PK_Account PRIMARY KEY (AccountId)
);

-- 핵심 정보: 캐릭터 목록 화면에서 조회하거나 변경 빈도가 낮은 값
CREATE TABLE [Character]
(
    CharacterId BIGINT       NOT NULL,
    AccountId   BIGINT       NOT NULL,
    Name        NVARCHAR(32) NOT NULL,
    ClassId     INT          NOT NULL,
    Level       INT          NOT NULL
        CONSTRAINT DF_Character_Level DEFAULT 1,
    CreatedAt   DATETIME2(3) NOT NULL
        CONSTRAINT DF_Character_CreatedAt DEFAULT SYSUTCDATETIME(),
    LastLoginAt DATETIME2(3) NULL,
    DeletedAt   DATETIME2(3) NULL,
    CONSTRAINT PK_Character PRIMARY KEY (CharacterId),
    CONSTRAINT FK_Character_Account FOREIGN KEY (AccountId)
        REFERENCES Account (AccountId),
    CONSTRAINT CK_Character_Level CHECK (Level >= 1)
);

-- 삭제되지 않은 캐릭터 사이에서만 이름 중복을 검사
CREATE UNIQUE NONCLUSTERED INDEX UX_Character_Name_Active
    ON [Character] (Name)
    WHERE DeletedAt IS NULL;

-- 캐릭터 목록 화면 조회용 커버링 인덱스
CREATE NONCLUSTERED INDEX IX_Character_AccountId
    ON [Character] (AccountId)
    INCLUDE (Name, ClassId, Level, LastLoginAt, DeletedAt);

-- 자주 갱신되는 상태 값: 경험치, 위치, 현재 HP/MP
CREATE TABLE CharacterState
(
    CharacterId BIGINT       NOT NULL,
    Exp         BIGINT       NOT NULL
        CONSTRAINT DF_CharacterState_Exp DEFAULT 0,
    MapId       INT          NOT NULL,
    PosX        REAL         NOT NULL,
    PosY        REAL         NOT NULL,
    PosZ        REAL         NOT NULL,
    Direction   REAL         NOT NULL
        CONSTRAINT DF_CharacterState_Direction DEFAULT 0,
    CurrentHp   INT          NOT NULL,
    CurrentMp   INT          NOT NULL,
    UpdatedAt   DATETIME2(3) NOT NULL
        CONSTRAINT DF_CharacterState_UpdatedAt DEFAULT SYSUTCDATETIME(), -- INSERT 시에만 적용, UPDATE 시에는 서버가 지정
    CONSTRAINT PK_CharacterState PRIMARY KEY (CharacterId),
    CONSTRAINT FK_CharacterState_Character FOREIGN KEY (CharacterId)
        REFERENCES [Character] (CharacterId),
    CONSTRAINT CK_CharacterState_Exp CHECK (Exp >= 0),
    CONSTRAINT CK_CharacterState_CurrentHp CHECK (CurrentHp >= 0),
    CONSTRAINT CK_CharacterState_CurrentMp CHECK (CurrentMp >= 0)
);

-- 확장·해금 정보: 결제나 퀘스트로 늘어난 수치만 저장 (기본 크기는 기획 데이터에 정의)
-- 행이 없으면 확장하지 않은 상태로 간주
CREATE TABLE CharacterExpansion
(
    CharacterId           BIGINT   NOT NULL,
    ExtraInventorySlots   SMALLINT NOT NULL
        CONSTRAINT DF_CharacterExpansion_ExtraInventorySlots DEFAULT 0,
    ExtraWarehouseSlots   SMALLINT NOT NULL
        CONSTRAINT DF_CharacterExpansion_ExtraWarehouseSlots DEFAULT 0,
    ExtraSkillPresetSlots TINYINT  NOT NULL
        CONSTRAINT DF_CharacterExpansion_ExtraSkillPresetSlots DEFAULT 0,
    CONSTRAINT PK_CharacterExpansion PRIMARY KEY (CharacterId),
    CONSTRAINT FK_CharacterExpansion_Character FOREIGN KEY (CharacterId)
        REFERENCES [Character] (CharacterId),
    CONSTRAINT CK_CharacterExpansion_NonNegative CHECK (
        ExtraInventorySlots >= 0 AND ExtraWarehouseSlots >= 0)
);

-- 플레이어가 분배한 스탯 포인트
CREATE TABLE CharacterStatAlloc
(
    CharacterId       BIGINT   NOT NULL,
    AllocStr          SMALLINT NOT NULL
        CONSTRAINT DF_CharacterStatAlloc_AllocStr DEFAULT 0,
    AllocDex          SMALLINT NOT NULL
        CONSTRAINT DF_CharacterStatAlloc_AllocDex DEFAULT 0,
    AllocInt          SMALLINT NOT NULL
        CONSTRAINT DF_CharacterStatAlloc_AllocInt DEFAULT 0,
    AllocVit          SMALLINT NOT NULL
        CONSTRAINT DF_CharacterStatAlloc_AllocVit DEFAULT 0,
    EarnedBonusPoints SMALLINT NOT NULL
        CONSTRAINT DF_CharacterStatAlloc_EarnedBonusPoints DEFAULT 0, -- 레벨업 이외의 경로(퀘스트, 아이템)로 누적 획득한 포인트
    CONSTRAINT PK_CharacterStatAlloc PRIMARY KEY (CharacterId),
    CONSTRAINT FK_CharacterStatAlloc_Character FOREIGN KEY (CharacterId)
        REFERENCES [Character] (CharacterId),
    CONSTRAINT CK_CharacterStatAlloc_NonNegative CHECK (
        AllocStr >= 0 AND AllocDex >= 0 AND AllocInt >= 0
            AND AllocVit >= 0 AND EarnedBonusPoints >= 0)
);

-- 로그아웃 후에도 유지되어야 하는 버프 (1:N)
CREATE TABLE CharacterBuff
(
    CharacterId BIGINT       NOT NULL,
    BuffId      INT          NOT NULL,
    Stack       SMALLINT     NOT NULL
        CONSTRAINT DF_CharacterBuff_Stack DEFAULT 1,
    ExpireAt    DATETIME2(3) NOT NULL,
    CONSTRAINT PK_CharacterBuff PRIMARY KEY (CharacterId, BuffId),
    CONSTRAINT FK_CharacterBuff_Character FOREIGN KEY (CharacterId)
        REFERENCES [Character] (CharacterId),
    CONSTRAINT CK_CharacterBuff_Stack CHECK (Stack >= 1)
);

-- 모든 아이템이 공유하는 공통 속성
-- 착용 중인 장비도 Container를 착용 슬롯으로 지정하여 이 테이블에서 관리
-- 소비 아이템과 퀘스트 아이템은 세부 테이블 없이 이 테이블만 사용
CREATE TABLE Item
(
    ItemUid    BIGINT       NOT NULL,                                                      -- 서버가 발급하는 전역 고유 ID
    OwnerId    BIGINT       NOT NULL,                                                      -- 캐릭터 ID
    TemplateId INT          NOT NULL,                                                      -- 기획 데이터의 아이템 ID
    ItemType   TINYINT      NOT NULL,                                                      -- 1:장비, 2:소비, 3:퀘스트 ...
    Container  TINYINT      NOT NULL,                                                      -- 인벤토리, 창고, 착용 슬롯 등
    SlotIndex  SMALLINT     NOT NULL,                                                      -- 착용 슬롯에서는 장착 부위 번호
    StackCount INT          NOT NULL
        CONSTRAINT DF_Item_StackCount DEFAULT 1,
    IsBound    BIT          NOT NULL
        CONSTRAINT DF_Item_IsBound DEFAULT 0,
    CreatedAt  DATETIME2(3) NOT NULL
        CONSTRAINT DF_Item_CreatedAt DEFAULT SYSUTCDATETIME(),
    CONSTRAINT PK_Item PRIMARY KEY (ItemUid),
    CONSTRAINT UQ_Item_OwnerId_Container_SlotIndex UNIQUE (OwnerId, Container, SlotIndex), -- 슬롯(착용 부위 포함) 하나에 아이템 하나
    CONSTRAINT UQ_Item_ItemUid_ItemType UNIQUE (ItemUid, ItemType),                        -- 서브타입 테이블의 복합 외래 키 대상
    CONSTRAINT FK_Item_Character FOREIGN KEY (OwnerId)
        REFERENCES [Character] (CharacterId),
    CONSTRAINT CK_Item_StackCount CHECK (StackCount >= 1),
    CONSTRAINT CK_Item_SlotIndex CHECK (SlotIndex >= 0)
);

-- 장비 아이템에만 존재하는 속성
CREATE TABLE ItemGear
(
    ItemUid      BIGINT  NOT NULL,
    ItemType     TINYINT NOT NULL
        CONSTRAINT DF_ItemGear_ItemType DEFAULT 1,
    EnhanceLevel TINYINT NOT NULL
        CONSTRAINT DF_ItemGear_EnhanceLevel DEFAULT 0,
    Durability   INT     NOT NULL,
    CONSTRAINT PK_ItemGear PRIMARY KEY (ItemUid),
    CONSTRAINT FK_ItemGear_Item FOREIGN KEY (ItemUid, ItemType)
        REFERENCES Item (ItemUid, ItemType) ON DELETE CASCADE,
    CONSTRAINT CK_ItemGear_ItemType CHECK (ItemType = 1),
    CONSTRAINT CK_ItemGear_Durability CHECK (Durability >= 0)
);

-- 장비의 랜덤 옵션처럼 개수가 가변적인 속성
CREATE TABLE ItemGearOption
(
    ItemUid     BIGINT  NOT NULL,
    OptionSlot  TINYINT NOT NULL,
    OptionId    INT     NOT NULL,
    OptionValue INT     NOT NULL,
    CONSTRAINT PK_ItemGearOption PRIMARY KEY (ItemUid, OptionSlot),
    CONSTRAINT FK_ItemGearOption_ItemGear FOREIGN KEY (ItemUid)
        REFERENCES ItemGear (ItemUid) ON DELETE CASCADE
);

-- 캐릭터 단위 재화: 재화 종류마다 행 하나
CREATE TABLE CharacterCurrency
(
    CharacterId BIGINT       NOT NULL,
    CurrencyId  INT          NOT NULL,                                      -- 기획 데이터의 재화 ID (골드, 명예 점수 등)
    Amount      BIGINT       NOT NULL
        CONSTRAINT DF_CharacterCurrency_Amount DEFAULT 0,
    UpdatedAt   DATETIME2(3) NOT NULL
        CONSTRAINT DF_CharacterCurrency_UpdatedAt DEFAULT SYSUTCDATETIME(), -- INSERT 시에만 적용, UPDATE 시에는 서버가 지정
    CONSTRAINT PK_CharacterCurrency PRIMARY KEY (CharacterId, CurrencyId),
    CONSTRAINT FK_CharacterCurrency_Character FOREIGN KEY (CharacterId)
        REFERENCES [Character] (CharacterId),
    CONSTRAINT CK_CharacterCurrency_Amount CHECK (Amount >= 0)
);

-- 계정 단위 재화: 같은 계정의 모든 캐릭터가 공유
CREATE TABLE AccountCurrency
(
    AccountId  BIGINT       NOT NULL,
    CurrencyId INT          NOT NULL,
    Amount     BIGINT       NOT NULL
        CONSTRAINT DF_AccountCurrency_Amount DEFAULT 0,
    UpdatedAt  DATETIME2(3) NOT NULL
        CONSTRAINT DF_AccountCurrency_UpdatedAt DEFAULT SYSUTCDATETIME(), -- INSERT 시에만 적용, UPDATE 시에는 서버가 지정
    CONSTRAINT PK_AccountCurrency PRIMARY KEY (AccountId, CurrencyId),
    CONSTRAINT FK_AccountCurrency_Account FOREIGN KEY (AccountId)
        REFERENCES Account (AccountId),
    CONSTRAINT CK_AccountCurrency_Amount CHECK (Amount >= 0)
);

-- 재화 변동 이력: 잔액 테이블과 같은 트랜잭션에서 기록
-- 고빈도 이벤트(몬스터 처치, 일반 상점 구매 등)는 정기 저장 행의 순변동에 합산
CREATE TABLE CurrencyLog
(
    LogId        BIGINT IDENTITY (1, 1) NOT NULL,
    OwnerType    TINYINT                NOT NULL, -- 1:캐릭터, 2:계정
    OwnerId      BIGINT                 NOT NULL, -- OwnerType에 따라 CharacterId 또는 AccountId
    CurrencyId   INT                    NOT NULL,
    Delta        BIGINT                 NOT NULL, -- 획득은 양수, 소모는 음수
    BalanceAfter BIGINT                 NOT NULL,
    Reason       SMALLINT               NOT NULL, -- 0:정기 저장, 1:거래, 2:우편, 3:경매장, 4:캐시, 5:한정 상품 ...
    RefId        BIGINT                 NULL,     -- Reason에 따라 거래 ID, 우편 ID, 경매 ID 등
    CreatedAt    DATETIME2(3)           NOT NULL
        CONSTRAINT DF_CurrencyLog_CreatedAt DEFAULT SYSUTCDATETIME(),
    CONSTRAINT PK_CurrencyLog PRIMARY KEY (LogId),
    CONSTRAINT CK_CurrencyLog_OwnerType CHECK (OwnerType IN (1, 2)),
    CONSTRAINT CK_CurrencyLog_Delta CHECK (Delta <> 0),
    CONSTRAINT CK_CurrencyLog_BalanceAfter CHECK (BalanceAfter >= 0)
);

-- 잔액 연속성 검증과 소유자별 이력 조회용
CREATE NONCLUSTERED INDEX IX_CurrencyLog_Owner_Currency
    ON CurrencyLog (OwnerType, OwnerId, CurrencyId, LogId)
    INCLUDE (Delta, BalanceAfter);

-- 거래 ID 등으로 관련 기록을 찾는 조회용
CREATE NONCLUSTERED INDEX IX_CurrencyLog_Reason_RefId
    ON CurrencyLog (Reason, RefId)
    WHERE RefId IS NOT NULL;