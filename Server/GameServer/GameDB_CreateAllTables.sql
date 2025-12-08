-- GameDB에서 사용하는 모든 테이블
DROP TABLE IF EXISTS CharactersLastState;
DROP TABLE IF EXISTS CharactersGearItems;
DROP TABLE IF EXISTS CharactersConsumableItems;
DROP TABLE IF EXISTS CharactersMiscItems;

-- 1. Characters: 캐릭터 기본 정보
DROP TABLE IF EXISTS Characters;
CREATE TABLE Characters(
    character_id        BIGINT IDENTITY(1, 1) PRIMARY KEY,
    user_id             BIGINT NOT NULL,
    class_id            INT NOT NULL,
    character_name      NVARCHAR(50) NOT NULL UNIQUE,
    level               SMALLINT NOT NULL DEFAULT 1,
    last_login          DATETIME2 NULL,
    created_at          DATETIME2 NOT NULL DEFAULT GETDATE()
);
GO


-- 2. CharactersState: 캐릭터의 마지막 상태
DROP TABLE IF EXISTS CharactersLastState;
CREATE TABLE CharactersLastState(
    character_id            BIGINT PRIMARY KEY,
    cur_hp                  BIGINT NOT NULL DEFAULT 0,
    cur_mp                  BIGINT NOT NULL DEFAULT 0,
    cur_physical_attack     BIGINT NOT NULL DEFAULT 0,
    cur_magical_attack      BIGINT NOT NULL DEFAULT 0,
    room_id                 INT NOT NULL DEFAULT 10,
    pos_x                   FLOAT NOT NULL DEFAULT 0.0,
    pos_y                   FLOAT NOT NULL DEFAULT 0.0,
    pos_z                   FLOAT NOT NULL DEFAULT 0.0,
    rot_yaw                 FLOAT NOT NULL DEFAULT 0.0,
    exp                     BIGINT NOT NULL DEFAULT 0,
    gold                    BIGINT NOT NULL DEFAULT 10000000,

    FOREIGN KEY (character_id) REFERENCES Characters(character_id)
    ON DELETE CASCADE
);
GO


-- 3. CharactersGearItems: 캐릭터의 장비 아이템
DROP TABLE IF EXISTS CharactersGearItems;
CREATE TABLE CharactersGearItems(
    character_id                    BIGINT NOT NULL,
    slot_id                         INT NOT NULL,
    item_uid                        BIGINT NOT NULL,
    template_id                     INT NOT NULL,
    is_equipped                     BIT NOT NULL,
    enhance                         INT NOT NULL DEFAULT 0,
    durability                      INT NOT NULL DEFAULT 0,
    additional_physical_attack      BIGINT NOT NULL DEFAULT 0,
    additional_magical_attack       BIGINT NOT NULL DEFAULT 0,

    FOREIGN KEY (character_id) REFERENCES Characters(character_id)
    ON DELETE CASCADE,

    PRIMARY KEY (character_id, slot_id, is_equipped)
);
GO


-- 4. CharactersConsumableItems: 캐릭터의 소비 아이템
DROP TABLE IF EXISTS CharactersConsumableItems;
CREATE TABLE CharactersConsumableItems(
    character_id        BIGINT NOT NULL,
    slot_id             INT NOT NULL,
    template_id         INT NOT NULL,
    count               INT NOT NULL DEFAULT 1,

    FOREIGN KEY (character_id) REFERENCES Characters(character_id)
    ON DELETE CASCADE,

    PRIMARY KEY (character_id, slot_id)
);
GO


-- 5. CharactersMiscItems: 캐릭터의 기타 아이템
DROP TABLE IF EXISTS CharactersMiscItems;
CREATE TABLE CharactersMiscItems(
    character_id        BIGINT NOT NULL,
    slot_id             INT NOT NULL,
    template_id         INT NOT NULL,
    count               INT NOT NULL DEFAULT 1,

    FOREIGN KEY (character_id) REFERENCES Characters(character_id)
    ON DELETE CASCADE,

    PRIMARY KEY (character_id, slot_id)
);
GO