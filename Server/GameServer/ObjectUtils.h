#pragma once

class ObjectUtils
{
public:
    template<typename SubClassType>
    static ObjectRef Create()
    {
        ObjectRef object = make_shared<SubClassType>();
        if (object->Init() == false)
            return nullptr;

        return object;
    }

	static PlayerRef CreatePlayer(GameSessionRef session);
    static MonsterRef CreateMonster(int32 templateId);
    
private:
	static atomic<int64> s_idGenerator;
};

