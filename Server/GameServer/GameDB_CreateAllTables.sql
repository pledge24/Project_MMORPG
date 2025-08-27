-- GameDB에서 사용하는 모든 테이블
DROP TABLE IF EXISTS CharactersLastState;
DROP TABLE IF EXISTS CharactersItems;
DROP TABLE IF EXISTS CharactersEquipments;

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
    character_id        BIGINT PRIMARY KEY,
    cur_hp              INT NOT NULL DEFAULT 0,
    cur_mp              INT NOT NULL DEFAULT 0,
    cur_attack          INT NOT NULL DEFAULT 0,
    cur_magic           INT NOT NULL DEFAULT 0,
    map_id              INT NOT NULL DEFAULT 0,
    pos_x               FLOAT NOT NULL DEFAULT 0.0,
    pos_y               FLOAT NOT NULL DEFAULT 0.0,
    pos_z               FLOAT NOT NULL DEFAULT 0.0,
    rot_yaw             FLOAT NOT NULL DEFAULT 0.0,
    exp                 BIGINT NOT NULL DEFAULT 0,
    gold                BIGINT NOT NULL DEFAULT 1000,

    FOREIGN KEY (character_id) REFERENCES Characters(character_id)
    ON DELETE CASCADE
);
GO


-- 3. CharactersItems: 캐릭터 아이템 정보(장비는 제외)
DROP TABLE IF EXISTS CharactersItems;
CREATE TABLE CharactersItems(
    character_id        BIGINT NOT NULL,
    template_id         INT NOT NULL,
    slot_id             INT NOT NULL,
    count               INT NOT NULL DEFAULT 1,

    FOREIGN KEY (character_id) REFERENCES Characters(character_id)
    ON DELETE CASCADE,

    PRIMARY KEY (character_id, slot_id)
);
GO


-- 4. CharactersEquipments: 캐릭터 장착 아이템 정보(무기, 갑옷)
DROP TABLE IF EXISTS CharactersEquipments;
CREATE TABLE CharactersEquipments(
    item_uid            BIGINT IDENTITY(1000, 1) PRIMARY KEY,
    character_id        BIGINT NOT NULL,
    template_id         INT NOT NULL,
    slot_id             INT NOT NULL, -- 장착 중인 무기이면 100,000부터 시작
    enhance             INT NOT NULL DEFAULT 0,
    durability          INT NOT NULL DEFAULT 0,
    bonus_attack        INT NOT NULL DEFAULT 0,
    bonus_magic         INT NOT NULL DEFAULT 0,

    FOREIGN KEY (character_id) REFERENCES Characters(character_id)
    ON DELETE CASCADE,
);
GO