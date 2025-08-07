SELECT character_id, class_id, character_name, level            
FROM [dbo].[Characters]                                         
WHERE deleted_at IS NULL AND user_id = 2002                      
ORDER BY created_at;
GO