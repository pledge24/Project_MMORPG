-- GameDB에서 사용하는 모든 테이블

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


-- 2. CharacterState: 캐릭터의 마지막 상태
DROP TABLE IF EXISTS CharacterLastState;
CREATE TABLE CharacterLastState(
    character_id        BIGINT PRIMARY KEY,
    exp                 BIGINT NOT NULL DEFAULT 0,
    cur_hp              INT NOT NULL DEFAULT 0,
    cur_mp              INT NOT NULL DEFAULT 0,
    cur_attack          INT NOT NULL DEFAULT 0,
    cur_magic           INT NOT NULL DEFAULT 0,
    map_id              INT NOT NULL,
    pos_x               FLOAT NOT NULL DEFAULT 0.0,
    pos_y               FLOAT NOT NULL DEFAULT 0.0,
    pos_z               FLOAT NOT NULL DEFAULT 0.0,
    rot_yaw             FLOAT NOT NULL DEFAULT 0.0,
    gold                BIGINT NOT NULL DEFAULT 1000,

    FOREIGN KEY (character_id) REFERENCES Characters(character_id)
    ON DELETE CASCADE
);
GO


-- 3. CharacterInventory: 캐릭터 인벤토리 정보
DROP TABLE IF EXISTS CharacterInventory;
CREATE TABLE CharacterInventory(
    character_id        BIGINT NOT NULL,
    item_id             INT NOT NULL,
    slot_id             INT NOT NULL,
    quantity            INT NOT NULL DEFAULT 1,

    FOREIGN KEY (character_id) REFERENCES Characters(character_id)
    ON DELETE CASCADE,

    PRIMARY KEY (character_id, slot_id)
);
GO


-- 4. CharacterEquipment: 캐릭터 장착 정보
DROP TABLE IF EXISTS CharacterEquipment;
CREATE TABLE CharacterEquipment(
    character_id        BIGINT NOT NULL,
    item_id             INT NOT NULL,
    slot_id             INT NOT NULL, -- 장착 부위 ID(ex. 0=투구, 1=상의, 2=하의, 3=무기)

    FOREIGN KEY (character_id) REFERENCES Characters(character_id)
    ON DELETE CASCADE,

    PRIMARY KEY (character_id, slot_id)
);
GO