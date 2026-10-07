#pragma once

/**
 * 엔티티를 만드는 정적 팩토리. 언리얼의 SpawnActorDeferred에 대응한다.
 * id 발급, 생성, Init(params)까지만 하고 룸에는 넣지 않는다. 룸에 넣고 Start하는 일은 Room::AddEntity가 맡는다.
 * 엔티티 id를 서버 전체에서 겹치지 않게 발급한다. id 발급이 atomic이라 여러 스레드에서 불러도 된다.
 */
class EntityFactory
{
public:
    /** T의 정의가 보이는 곳에서 부른다. Init에 실패하면 nullptr. */
    template<typename T>
    static shared_ptr<T> Create(const typename T::SpawnParams& params)
    {
        shared_ptr<T> entity = make_shared<T>();

        // Init이 id를 읽을 수 있도록 먼저 쓴다.
        const int64 newId = s_idGenerator.fetch_add(1);
        entity->_entityInfo->set_entity_id(newId);
        entity->_posInfo->set_entity_id(newId);

        if (entity->Init(params) == false)
            return nullptr;

        return entity;
    }

private:
    /** 1부터 발급한다. 플레이어와 몬스터가 같은 번호 공간을 쓴다. */
    static atomic<int64> s_idGenerator;
};
