#pragma once

/**
 * 엔티티를 만드는 정적 팩토리.
 *
 * 서버 전체에서 겹치지 않는 엔티티 id를 발급하고, 룸에 넣기 전까지의 생성을 맡는다.
 * 언리얼의 SpawnActorDeferred에 대응하며, 여러 스레드에서 불러도 된다.
 */
class EntityFactory
{
public:
    /**
     * 엔티티를 만들어 Init까지 부른다. 룸에는 넣지 않으며, Init에 실패하면 nullptr.
     * T의 정의가 보이는 곳에서 부른다.
     */
    template<typename T>
    static shared_ptr<T> Create(const typename T::SpawnParams& params)
    {
        shared_ptr<T> entity = make_shared<T>();

        // Init이 id를 읽을 수 있도록 먼저 쓴다.
        const int64 newId = s_idGenerator.fetch_add(1);
        entity->SetEntityId(newId);

        if (entity->Init(params) == false)
            return nullptr;

        return entity;
    }

private:
    /** Entity 고유번호. 1부터 발급하며, 플레이어와 몬스터가 같은 번호 공간을 쓴다. */
    static atomic<int64> s_idGenerator;
};
