#pragma once

class ObjectUtils
{
public:
    // 할당만 담당한다. Init()은 각 팩토리가 필요한 값을 모두 채운 뒤에 호출한다.
    // (Monster::Init()은 template_id와 posInfo가 이미 세팅돼 있어야 성공한다)
    template<typename SubClassType>
    static ObjectRef Create()
    {
        return make_shared<SubClassType>();
    }

	static PlayerRef CreatePlayer(GameSessionRef session);
    static MonsterRef CreateMonster(int32 templateId, const Protocol::PosInfo& spawnPos);
    
private:
	static atomic<int64> s_idGenerator;
};

