#pragma once

/**
 * 엔티티 하나의 기능 하나를 맡는 컴포넌트의 베이스. 소유자와 Start/Tick만 공유한다.
 * 엔티티는 컴포넌트를 타입이 정해진 멤버로 직접 소유하고, 컴포넌트를 타입으로 찾는 범용 조회는 두지 않는다(ADR-0013).
 * 소유자가 자기 Start와 Tick에서 컴포넌트의 Start와 Tick을 부른다. 스레드는 소유자를 따른다.
 * 소유자는 컴포넌트를 shared_ptr로 들고, 컴포넌트는 소유자를 weak_ptr로 가리키므로 순환 참조가 없다.
 */
class EntityComponent
{
public:
    explicit EntityComponent(EntityRef owner) : _owner(owner) {}
    virtual ~EntityComponent() = default;

    /** 소유자의 Start에서 한 번 부른다. */
    virtual void Start() {}
    /** 소유자의 Tick에서 부른다. 틱을 돌리지 않는 소유자는 부르지 않는다. */
    virtual void Tick(float deltaTime) {}

protected:
    weak_ptr<Entity> _owner;
};
