--SELECT character_id, class_id, character_name, level            
--FROM [dbo].[Characters]                                         
--WHERE deleted_at IS NULL AND user_id = 2002                      
--ORDER BY created_at;
--GO

BEGIN TRANSACTION;
            
DECLARE @existing_character_id BIGINT;
DECLARE @character_id BIGINT;
DECLARE @user_id BIGINT = 2002;
DECLARE @class_id INT = 1;
DECLARE @character_name NVARCHAR (50) = N'안녕하세ㄴㄴ요1';
                     
SELECT @existing_character_id = character_id
FROM [dbo].[Characters]
WITH(UPDLOCK, HOLDLOCK)
WHERE character_name = @character_name;
            
IF @existing_character_id IS NULL
BEGIN
    -- 1. 캐릭터 기본 정보 삽입
    INSERT INTO [dbo].[Characters]([user_id], [class_id], [character_name])
    VALUES(@user_id, @class_id, @character_name);
                
    -- 마지막에 삽입된 ID 가져오기(identity로)
    SET @character_id = SCOPE_IDENTITY()
                
    -- 2. 캐릭터 마지막 상태 저장(기본값)
    INSERT INTO [dbo].[CharacterLastState]([character_id], [cur_hp], [cur_mp], [cur_attack], [cur_magic])
    VALUES(@character_id, 10, 10, 10, 10);
                
    -- 3. 결과셋으로 반환
    SELECT @character_id as character_id;
                
    COMMIT TRANSACTION;
END
ELSE
BEGIN
    SELECT -1 as character_id;
    ROLLBACK TRANSACTION;
END