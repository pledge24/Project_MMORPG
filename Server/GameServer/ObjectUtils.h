#pragma once

class ObjectUtils
{
public:
	static PlayerRef CreatePlayer(GameSessionRef session);
    static MonsterRef CreateMonster(int32 templateId);
    
private:
	static atomic<int64> s_idGenerator;
};

