ALTER TABLE Characters
ADD CONSTRAINT UQ_Characters_character_name UNIQUE (character_name);
GO

ALTER TABLE Characters DROP COLUMN deleted_at