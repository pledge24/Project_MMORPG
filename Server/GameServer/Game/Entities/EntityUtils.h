#pragma once

/**
 * 엔티티를 만드는 정적 팩토리. 엔티티 id를 서버 전체에서 겹치지 않게 발급한다.
 * id 발급이 atomic이라 여러 스레드에서 불러도 된다.
 */
class EntityUtils
{
public:
    /**
     * 할당만 담당한다. Init()은 각 팩토리가 필요한 값을 모두 채운 뒤에 호출한다.
     * (Monster::Init()은 template_id와 posInfo가 이미 세팅돼 있어야 성공한다)
     */
    template<typename SubClassType>
    static EntityRef CreateEntity()
    {
        return make_shared<SubClassType>();
    }

    /**
     * 플레이어를 만들어 세션에 연결하고 Init까지 한다. 게임 입장 때 DB 스레드에서 부른다.
     * Init에 실패하면 nullptr를 돌려주지만, 세션의 _player에는 이미 들어가 있다.
     */
	static PlayerRef CreatePlayer(GameSessionRef session);
    /** 몬스터를 만들어 Init까지 한다. 룸에 넣고 Start하는 일은 호출자가 한다. Init에 실패하면 nullptr. */
    static MonsterRef CreateMonster(int32 templateId, const Protocol::PosInfo& spawnPos);

private:
    /** 1부터 발급한다. 플레이어와 몬스터가 같은 번호 공간을 쓴다. */
	static atomic<int64> s_idGenerator;
};

